#define _XOPEN_SOURCE
#include <time.h>
#include "GSMNetworkHandler.h"
#include "command/CharModemCMD.h"
#include "command/ByteModemCMD.h"
#include "command/ULong2ModemCMD.h"
#include "command/SMSSendModemCMD.h"

GSMNetworkHandler::GSMNetworkHandler(BaseGSMHandler *gsmHandler):GSMCallHandler(gsmHandler)
{
    gsmStats.regState = GSM_REG_STATE_UNKNOWN;
    gsmStats.networkType = GSM_NETWORK_UNKNOWN;
    gsmStats.thresoldState = GSM_THRESOLD_T;
    gsmStats.signalStrength = 0;
    gsmStats.signalQuality = 0;
}
GSMNetworkHandler::~GSMNetworkHandler()
{
    
}

void GSMNetworkHandler::SetGSMListener(IGSMNetworkHandler *listener)
{
    this->listener = listener;
}

void GSMNetworkHandler::Connect(const char *simPin)
{
    this->simPin = simPin;
    gsmStats.regState = GSM_REG_STATE_UNKNOWN;
    initState = GSM_STATE_PIN;
    //gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_NETWORK_REG));
    gsmHandler->ForceCommand(new BaseModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
}

bool GSMNetworkHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (incomingSms != NULL) {
        incomingSms->message = data;
        if (listener != NULL) {
            listener->OnSMSReceive(incomingSms);
        }
        if (incomingSms->sender != NULL) {
            free(incomingSms->sender);
            incomingSms->sender = NULL;
        }
        delete incomingSms;
        incomingSms = NULL;
        // We got sms data?
        return true;
    }
    // Handle +CREG?

    if (strncmp(data, GSM_SMS_EVENT, strlen(GSM_SMS_EVENT)) == 0 && data[strlen(GSM_SMS_EVENT)] == ':') {
        // +CMT: "+393475234652",,"14/11/21,11:58:23+01"
        char *cmt = data + strlen(GSM_SMS_EVENT) + 2;
		char *cmtArgs[3];
        SplitString(cmt, ',', cmtArgs, 2, false);
        ShiftQuotations(cmtArgs, 3);

        incomingSms = new IncomingSMSData();
        incomingSms->sender = (char *)malloc(strlen(cmtArgs[0]) + 1);
        strcpy(incomingSms->sender, cmtArgs[0]);
        tm smsTime;
        if (strptime(cmtArgs[2], "%y/%m/%d,%H:%M:%S", &smsTime) != NULL) {
            incomingSms->utcTime = mktime(&smsTime);
            char *zPointer = strchr(cmtArgs[2], '+');
            if (zPointer == NULL) {
                zPointer = strchr(cmtArgs[2], '-');
            }
            if (zPointer != NULL) {
                incomingSms->timeZone = atoi(zPointer) * (15 * 60);
            }
            incomingSms->utcTime -= incomingSms->timeZone;
        }
        return true;
    }
    if (strncmp(data, GSM_CMD_NETWORK_REG, strlen(GSM_CMD_NETWORK_REG)) == 0) {

        char *creg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
		char *cregArgs[2];
        cregArgs[1] = 0;

		SplitString(creg, ',', cregArgs, 2, false);
        if (cregArgs[1] == 0) {
            gsmStats.regState = (GSM_REG_STATE)atoi(cregArgs[0]);
        } else {
		    gsmStats.regState = (GSM_REG_STATE)atoi(cregArgs[1]);
        }

        //Serial.print("regState: ");
        //Serial.println((int)gsmStats.regState);
        
        switch (gsmStats.regState) {
            case GSM_REG_STATE_CONNECTING_HOME:
            case GSM_REG_CONNECTED_HOME:
            case GSM_REG_STATE_CONNECTING_OTHER:
            case GSM_REG_STATE_CONNECTED_ROAMING:
            case GSM_REG_STATE_CONNECTED_SMS_ONLY_HOME:
            case GSM_REG_STATE_CONNECTED_SMS_ONLY_ROAMING:
            case GSM_REG_STATE_CONNECTED_EMERGENSY_ONLY:
            case GSM_REG_STATE_CONNECTED_CSFB_NOT_HOME:
            case GSM_REG_STATE_CONNECTED_CSFB_NOT_ROAMING:
                if (initState < GSM_STATE_READY && initState > GSM_STATE_NONE) {
                    FetchTime();
                    initState = GSM_STATE_READY;
                    listener->OnGSMConnected();
                }
                if (listener != NULL) {
                    listener->OnGSMStatus(gsmStats.regState);
                }
                break;
            case GSM_REG_STATE_DENIED:
            case GSM_REG_STATE_UNKNOWN:
                if (listener != NULL) {
                    listener->OnGSMFailed(GSM_PIN_STATE_SIM_PIN);
                }
                break;
        }
        return true;
    }
    if (strncmp(data, GSM_NETWORK_TYPE_STATUS, strlen(GSM_NETWORK_TYPE_STATUS)) == 0) {
        char *ureg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
        gsmStats.networkType = (GSM_NETWORK_TYPE)atoi(ureg);
        if (listener != NULL) {
            listener->OnGSMNetworkType(gsmStats.networkType);
        }
        return true;
    }
    if (strncmp(data, GSM_TEMP_THRESOLD_EVENT, strlen(GSM_TEMP_THRESOLD_EVENT)) == 0) {
        // GSM_TEMP_THRESOLD_EVENT
        char *uusts = data + strlen(GSM_TEMP_THRESOLD_EVENT) + 2;
		char *uustsArgs[2];
        SplitString(uusts, ',', uustsArgs, 2, false);
        gsmStats.thresoldState = (GSM_THRESOLD_STATE)atoi(uustsArgs[1]);
        if (listener != NULL) {
            listener->OnGSMThresold(gsmStats.thresoldState);
        }
        return true;
    }
    
    return GSMCallHandler::OnGSMEvent(data, dataLen);
}

bool GSMNetworkHandler::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{

    //Serial.print("GSMNetworkHandler::OnGSMResponse [");
    //Serial.print(response);
    //Serial.print("] ");
    //Serial.println(result);

    if (strcmp(request->cmd, GSM_SIM_PIN_CMD) == 0) {
        if (request->IsCheck()) {
            PinModemCMD *pinCMD = (PinModemCMD *)request;
            // AT+CPIN? confirmation
            // State should be changed on data read
            if (type == MODEM_RESPONSE_OK) {
                if (pinCMD->pinState == GSM_PIN_STATE_SIM_PIN) {
                    gsmHandler->ForceCommand(new CharModemCMD(simPin, GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
                } else if (listener != NULL) {
                    listener->OnGSMFailed(pinCMD->pinState);
                }
            } else if (type == MODEM_RESPONSE_DATA) {
                pinCMD->HandleDataContent(response, respLen);
            } else if (type > MODEM_RESPONSE_OK && type < MODEM_RESPONSE_TIMEOUT) {
                // Resend cmd, pin module not ready
                gsmHandler->ForceCommand(new BaseModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
            } else if (type > MODEM_RESPONSE_TIMEOUT) {
                if (listener != NULL) {
                    listener->OnGSMFailed(pinCMD->pinState);
                }
            }
            return true;
        } else {
            // AT+CPIN="" confirmation on pin insert
            if (type == MODEM_RESPONSE_OK) {
                isSimUnlocked = true;
                initState = GSM_STATE_CONFIG;
                //gsmHandler->ForceCommand(new ByteModemCMD(2, GSM_TEMP_THRESOLD_CMD));
                gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_NETWORK_REG));
                if (listener != NULL) {
                    listener->OnGSMSimUnlocked();
                }
            } else {
                isSimUnlocked = false;
                if (listener != NULL) {
                    listener->OnGSMFailed(GSM_PIN_STATE_SIM_PIN);
                }
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_NETWORK_REG) == 0) {
        if (request->IsCheck()) {
            // TODO:
        } else {
            if (type == MODEM_RESPONSE_OK) {
                //gsmHandler->ForceCommand(new BaseModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
                gsmHandler->ForceCommand(new ByteModemCMD(2, GSM_TEMP_THRESOLD_CMD));
            } else {
                // Check again or return failed to load?
                //gsmHandler->ForceCommand(new BaseModemCMD(GSM_CMD_NETWORK_REG, MODEM_COMMAND_TIMEOUT, true));
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_TEMP_THRESOLD_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_SMS_FROMAT));
        } else {
            // TODO: resend?
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_SMS_FROMAT) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new CharModemCMD("2,2,0,0,0", GSM_CMD_SMS_INDICATION));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_SMS_INDICATION) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ULong2ModemCMD(1,1, GSM_CMD_HEX_MODE));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_HEX_MODE) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_TIME_ZONE));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_TIME_ZONE) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ULong2ModemCMD(1,2, GSM_CMD_DTFM));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_DTFM) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_NETWORK_TYPE_STATUS));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_NETWORK_TYPE_STATUS) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_CALL_STATUS));
        } else {
            // TODO: resend?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_CALL_STATUS) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            initState = GSM_STATE_WAIT_CREG;
            // TODO: Wait network redistration
        } else {
            // TODO: resend?
        }
        return true;
    }

    if (strncmp(GSM_CMD_NETWORK_QUALITY, response, strlen(GSM_CMD_NETWORK_QUALITY)) == 0) {
        char* csq = response + strlen(GSM_CMD_NETWORK_QUALITY) + 2;
        char* csqArgs[2];

        SplitString(csq, ',', csqArgs, 2, false);

        gsmStats.signalStrength = atoi(csqArgs[0]); // 0 - 31, 99
        if (gsmStats.signalStrength > 31) {
            gsmStats.signalStrength = 0;
        }
        gsmStats.signalStrength = ((double)gsmStats.signalStrength / 31.0) * 100.0;

        gsmStats.signalQuality = atoi(csqArgs[1]); // 0 - 7, 99
        if (gsmStats.signalQuality > 7) {
            gsmStats.signalQuality = 0;
        }
        gsmStats.signalQuality = ((7.0 - (double)gsmStats.signalQuality) / 7.0) * 100.0;

        if (listener != NULL) {
            listener->OnGSMQuality(gsmStats.signalStrength, gsmStats.signalQuality);
        }
        return true;
    }
    if (strncmp(GSM_CMD_TIME, response, strlen(GSM_CMD_TIME)) == 0) {

        tm now;

        if (strptime(response, "+CCLK: \"%y/%m/%d,%H:%M:%S", &now) != NULL) {
            // adjust for timezone offset which is +/- in 15 minute increments

            currentTime = mktime(&now);
            syncTS = millis();

            char *zPointer = strchr(response, '+');
            if (zPointer == NULL) {
                zPointer = strchr(response, '-');
            }
            if (zPointer != NULL) {
                timeZone = atoi(zPointer) * (15 * 60);
            }
        }
        return true;
    }
    // TODO: 
    if (strcmp(request->cmd, GSM_CMD_SMS_SEND) == 0) {
        // TODO: Filter out sended sms
        if (type == MODEM_RESPONSE_DATA) {
            if (strcmp("> ", response) == 0) {
                request->ExtraTrigger(false);
                Stream *modemStream = gsmHandler->GetModemStream();
                if (listener != NULL && listener->OnSMSSendStream(modemStream)) {
                    //Send message end
                    modemStream->write(CRTLZ_ASCII_SYMBOL);
                } else {
                    // Send message cancel callback
                    modemStream->write(ESC_ASCII_SYMBOL);
                }
            }
        }
        return true;
    }
    return GSMCallHandler::OnGSMResponse(request, response, respLen, type);
}

void GSMNetworkHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == delayedRequest) {
        delayedRequest = 0;
        FetchTime();
    }
}
void GSMNetworkHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == delayedRequest) {
        delayedRequest = 0;
    }
}

void GSMNetworkHandler::FetchTime() {
    Timer::Stop(delayedRequest);
    delayedRequest = Timer::Start(this, QUALITY_CHECK_DURATION, 1u);
    //gsmHandler->AddCommand("AT+CCLK?");  // Sync time
    //gsmHandler->AddCommand("AT+CSQ");    // Get signal quality
    //gsmHandler->AddCommand("AT+UTEMP?", 5000000ul);  // Get temperature;
    gsmHandler->AddCommand(new BaseModemCMD(GSM_CMD_TIME, MODEM_COMMAND_TIMEOUT, true));
    gsmHandler->AddCommand(new BaseModemCMD(GSM_CMD_NETWORK_QUALITY));
}

bool GSMNetworkHandler::IsInitialized()
{
    return initState == GSM_STATE_READY;
}

bool GSMNetworkHandler::IsSimUnlocked()
{
    return isSimUnlocked;
}

unsigned long GSMNetworkHandler::GetUTCTime()
{
    return GetLocalTime() - timeZone;
}
unsigned long GSMNetworkHandler::GetLocalTime()
{
    if (currentTime == 0) return 0;
    unsigned long syncDelta = (millis() - syncTS) / 1000ul;
    return currentTime + syncDelta;
}

GSMNetworkStats *GSMNetworkHandler::GetGSMStats() {
    return &gsmStats;
}

bool GSMNetworkHandler::SendSMS(char *phone)
{
    return gsmHandler->AddCommand(new SMSSendModemCMD(phone, GSM_CMD_SMS_SEND));
}

void GSMNetworkHandler::OnModemReboot()
{
    initState = GSM_STATE_NONE;
    gsmStats.regState = GSM_REG_STATE_UNKNOWN;
    gsmStats.networkType = GSM_NETWORK_UNKNOWN;
    gsmStats.thresoldState = GSM_THRESOLD_T;
    gsmStats.signalStrength = 0;
    gsmStats.signalQuality = 0;
    Timer::Stop(delayedRequest);

    if (incomingSms != NULL) {
        if (incomingSms->sender != NULL) {
            free(incomingSms->sender);
        }
        delete incomingSms;
        incomingSms = NULL;
    }
    GSMCallHandler::OnModemReboot();
}