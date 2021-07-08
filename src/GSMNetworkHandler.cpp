#define _XOPEN_SOURCE
#include <time.h>
#include "GSMNetworkHandler.h"
#include "command/CharModemCMD.h"
#include "command/ByteModemCMD.h"
#include "command/ULong2ModemCMD.h"
#include "command/SMSSendModemCMD.h"
#include "command/PinModemCMD.h"
#include "common/GSMUtils.h"

GSMNetworkHandler::GSMNetworkHandler(BaseGSMHandler *gsmHandler):GSMCallHandler(gsmHandler)
{
    gsmStats.regState = GSM_REG_STATE_UNKNOWN;
    gsmStats.networkType = GSM_NETWORK_UNKNOWN;
    gsmStats.thresholdState = GSM_THRESHOLD_T;
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
    Timer::Stop(gsmTimer);
    gsmTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME, 0);
    gsmHandler->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
}

bool GSMNetworkHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (incomingSms != NULL) {
        if (listener != NULL) {
            listener->OnSMSReceive(incomingSms, data, dataLen);
        }
        FlushIncomingSMS();
        // We got sms data?
        return true;
    }
    // Handle +CREG?

    if (IsEvent(GSM_SMS_EVENT, data, dataLen)) {
        // +CMT: "+393475234652",,"14/11/21,11:58:23+01"
        char *cmt = data + strlen(GSM_SMS_EVENT) + 2;
		char *cmtArgs[3];
        SplitString(cmt, ',', cmtArgs, 2, false);
        ShiftQuotations(cmtArgs, 3);

        incomingSms = new IncomingSMSInfo();
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
    if (IsEvent(GSM_CMD_NETWORK_REG, data, dataLen)) {
        gsmStats.regState = GetCregState(data, dataLen);
        UpdateCregResult();
        return true;
    }
    if (IsEvent(GSM_NETWORK_TYPE_STATUS, data, dataLen)) {
        char *ureg = data + strlen(GSM_NETWORK_TYPE_STATUS) + 2;
        gsmStats.networkType = (GSM_NETWORK_TYPE)atoi(ureg);
        if (listener != NULL) {
            listener->OnGSMNetworkType(gsmStats.networkType);
        }
        return true;
    }
    if (IsEvent(GSM_TEMP_THRESHOLD_EVENT, data, dataLen)) {
        // GSM_TEMP_THRESHOLD_EVENT
        char *uusts = data + strlen(GSM_TEMP_THRESHOLD_EVENT) + 2;
		char *uustsArgs[2];
        SplitString(uusts, ',', uustsArgs, 2, false);
        gsmStats.thresholdState = (GSM_THRESHOLD_STATE)atoi(uustsArgs[1]);

        //Serial.print("Threshold state: ");
        //Serial.println((int)gsmStats.thresholdState);

        if (listener != NULL) {
            listener->OnGSMThreshold(gsmStats.thresholdState);
        }
        return true;
    }
    
    return GSMCallHandler::OnGSMEvent(data, dataLen);
}

bool GSMNetworkHandler::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strcmp(request->cmd, GSM_SIM_PIN_CMD) == 0) {
        if (request->IsCheck()) {
            PinStatusModemCMD *pinStatusCMD = (PinStatusModemCMD *)request;
            // AT+CPIN? confirmation
            // State should be changed on data read
            if (type == MODEM_RESPONSE_OK) {
                if (pinStatusCMD->pinState == GSM_PIN_STATE_SIM_PIN) {
                    gsmHandler->ForceCommand(new CharModemCMD(simPin, GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
                } else if (pinStatusCMD->pinState == GSM_PIN_STATE_READY) {
                    HandleSimUnlocked();
                } else if (listener != NULL) {
                    listener->OnGSMFailed(pinStatusCMD->pinState);
                }
            } else if (type == MODEM_RESPONSE_DATA) {
                pinStatusCMD->HandleDataContent(response, respLen);
            } else if (type > MODEM_RESPONSE_OK && type < MODEM_RESPONSE_TIMEOUT) {
                // Resend cmd, pin module not ready
                gsmHandler->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
            } else if (type > MODEM_RESPONSE_TIMEOUT) {
                if (listener != NULL) {
                    listener->OnGSMFailed(pinStatusCMD->pinState);
                }
            }
            return true;
        } else {
            // AT+CPIN="" confirmation on pin insert
            if (type == MODEM_RESPONSE_OK) {
                HandleSimUnlocked();
            } else if (type == MODEM_RESPONSE_ERROR) {
                isSimUnlocked = false;
                if (listener != NULL) {
                    isSimUnlocked = false;
                }
            } else if (type > MODEM_RESPONSE_ERROR) {
                if (listener != NULL) {
                    listener->OnGSMFailed(GSM_PIN_STATE_UNKNOWN);
                }
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_NETWORK_REG) == 0) {
        if (request->IsCheck()) {
            // TODO:
            if (type== MODEM_RESPONSE_DATA) {
                gsmStats.regState = GetCregState(response, respLen);
            } else if (type== MODEM_RESPONSE_OK) {
                switch (gsmStats.regState) {
                    case GSM_REG_STATE_CONNECTING_HOME:
                    case GSM_REG_STATE_CONNECTING_OTHER:
                        // Connection in progress, do nothing
                        break;
                    default:
                        UpdateCregResult();
                        break;
                }
            }
        } else {
            if (type == MODEM_RESPONSE_OK) {
                gsmHandler->ForceCommand(new ByteModemCMD(2, GSM_TEMP_THRESHOLD_CMD));
            } else {
                // Check again or return failed to load?
                //gsmHandler->ForceCommand(new BaseModemCMD(GSM_CMD_NETWORK_REG, MODEM_COMMAND_TIMEOUT, true));
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_TEMP_THRESHOLD_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_CMD_UTEMP));
        } else {
            // TODO: resend?
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_UTEMP) == 0) {
        if (request->IsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                char* temp = response + strlen(GSM_CMD_UTEMP) + 2;
                gsmStats.temperature = atoi(temp);
                gsmStats.temperature /= 10;
                //Serial.print("Temperature: ");
                //Serial.print(gsmStats.temperature);
                //Serial.println("C");
            }
        } else {
            if (type == MODEM_RESPONSE_OK) {
                gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_SMS_FROMAT));
            } else {
                // TODO: resend?
            }
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
            gsmHandler->ForceCommand(new BaseModemCMD(GSM_CMD_NETWORK_REG, MODEM_COMMAND_TIMEOUT, true));
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

            char *zPointer = strchr(response + strlen(GSM_CMD_TIME) + 2, '+');
            if (zPointer == NULL) {
                zPointer = strchr(response + strlen(GSM_CMD_TIME) + 2, '-');
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


void GSMNetworkHandler::HandleSimUnlocked()
{
    isSimUnlocked = true;
    initState = GSM_STATE_CONFIG;
    gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_NETWORK_REG));
    if (listener != NULL) {
        listener->OnGSMSimUnlocked();
    }
}


GSM_REG_STATE GSMNetworkHandler::GetCregState(char * data, size_t dataLen)
{
    char *creg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
    char *cregArgs[2];
    cregArgs[1] = 0;

    size_t args = SplitString(creg, ',', cregArgs, 2, false);
    if (args == 2) { // Data from event
        return (GSM_REG_STATE)atoi(cregArgs[1]);
    }
    return (GSM_REG_STATE)atoi(cregArgs[0]);
}

void GSMNetworkHandler::UpdateCregResult()
{
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
                FetchModemStats();
                initState = GSM_STATE_READY;
                if (listener != NULL) {
                    listener->OnGSMConnected();
                }
            }
            if (listener != NULL) {
                listener->OnGSMStatus(gsmStats.regState);
            }
            break;
        case GSM_REG_STATE_DENIED:
        case GSM_REG_STATE_UNKNOWN:
            if (listener != NULL) {
                listener->OnGSMFailed(GSM_PIN_STATE_READY);
            }
            break;
    }
}

void GSMNetworkHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == gsmTimer) {
        gsmTimer = 0;
        if (data == 0) {
            if (listener != NULL) {
                listener->OnGSMFailed(GSM_PIN_STATE_UNKNOWN);
            }
        } else {
            FetchModemStats();
        }
    }
}
void GSMNetworkHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == gsmTimer) {
        gsmTimer = 0;
    }
}

void GSMNetworkHandler::FetchModemStats() {
    Timer::Stop(gsmTimer);
    gsmTimer = Timer::Start(this, QUALITY_CHECK_DURATION, 1u);
    gsmHandler->AddCommand(new BaseModemCMD(GSM_CMD_TIME, MODEM_COMMAND_TIMEOUT, true));
    gsmHandler->AddCommand(new BaseModemCMD(GSM_CMD_UTEMP, MODEM_COMMAND_TIMEOUT, true));
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

time_t GSMNetworkHandler::GetUTCTime()
{
    return GetLocalTime() - timeZone;
}
time_t GSMNetworkHandler::GetLocalTime()
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
    gsmStats.thresholdState = GSM_THRESHOLD_T;
    gsmStats.signalStrength = 0;
    gsmStats.signalQuality = 0;
    Timer::Stop(gsmTimer);

    FlushIncomingSMS();
    GSMCallHandler::OnModemReboot();
}

void GSMNetworkHandler::FlushIncomingSMS()
{
    if (incomingSms != NULL) {
        delete incomingSms;
        incomingSms = NULL;
    }
}