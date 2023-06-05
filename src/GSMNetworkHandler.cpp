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
    Timer::Stop(gsmReconnectTimer);
    gsmReconnectTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME, 0);
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
        SplitString(cmt, ',', cmtArgs, 3, false);

        ShiftQuotations(cmtArgs, 3);
        incomingSms = new IncomingSMSInfo();
        strcpy(incomingSms->sender, cmtArgs[0]);

        tmz tmzStruct;
        incomingSms->timeStamp.utcStamp = ExtractTime(cmtArgs[2], &tmzStruct);
        if (incomingSms->timeStamp.utcStamp > 0 ) {
            incomingSms->timeStamp.quaterZone = tmzStruct.quaterZone;
            incomingSms->timeStamp.utcStamp -= ZoneInSeconds(tmzStruct.quaterZone);
        }
        return true;
    }
    if (IsEvent(GSM_CMD_NETWORK_REG, data, dataLen)) {
        if (gsmReconnectTimer != 0) {
            // Stop timer if it is for modem reboot
            Timer::Stop(gsmReconnectTimer);
        }
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
        if (request->GetIsCheck()) {
            PinStatusModemCMD *pinStatusCMD = (PinStatusModemCMD *)request;
            // AT+CPIN? confirmation
            // State should be changed on data read
            if (type == MODEM_RESPONSE_OK) {
                if (pinStatusCMD->pinState == GSM_PIN_STATE_SIM_PIN) {
                    gsmHandler->ForceCommand(new CharModemCMD(simPin, GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
                } else if (pinStatusCMD->pinState == GSM_PIN_STATE_READY) {
                    HandleSimUnlocked();
                } else {
                    HandleGSMFail(GSM_FAIL_OTHER_PIN);
                }
            } else if (type == MODEM_RESPONSE_DATA) {
                pinStatusCMD->HandleDataContent(response, respLen);
            } else if (type >= MODEM_RESPONSE_ERROR && type <= MODEM_RESPONSE_CANCELED) {
                // Resend cmd, pin module not ready
                //if (pinStatusCMD->cme_error == CME_ERROR_NO_SIM) {
                //    HandleGSMFail(GSM_FAIL_NO_SIM);
                //    return true;
                //}
                // TODO: Start delayed timer?

                gsmSimTimer = Timer::Start(this, GSM_MODEM_SIM_PIN_DELAY, 0);
                //gsmHandler->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
            } else if (type > MODEM_RESPONSE_TIMEOUT) {
                HandleGSMFail(GSM_FAIL_OTHER_PIN);
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
                HandleGSMFail(GSM_FAIL_UNKNOWN);
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_NETWORK_REG) == 0) {
        if (request->GetIsCheck()) {
            if (type== MODEM_RESPONSE_DATA) {
                gsmStats.regState = GetCregState(response, respLen);
            } else if (type== MODEM_RESPONSE_OK) {
                switch (gsmStats.regState) {
                    default:
                        UpdateCregResult();
                        break;
                }
            } else if (type >= MODEM_RESPONSE_ERROR) {
                gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
            }
        } else {
            if (type == MODEM_RESPONSE_OK) {
                gsmHandler->ForceCommand(new ByteModemCMD(2, GSM_TEMP_THRESHOLD_CMD));
            } else if (type >= MODEM_RESPONSE_ERROR) {
                gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
            }
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_TEMP_THRESHOLD_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_CMD_UTEMP));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_UTEMP) == 0) {
        if (request->GetIsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                char* temp = response + strlen(GSM_CMD_UTEMP) + 2;
                gsmStats.temperature = atoi(temp);
                gsmStats.temperature /= 10;
            } else if (type == MODEM_RESPONSE_TIMEOUT) {
                gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // TODO: ?
            }
        } else {
            if (type == MODEM_RESPONSE_OK) {
                gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_SMS_FROMAT));
            } else if (type >= MODEM_RESPONSE_ERROR) {
                gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_SMS_FROMAT) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new CharModemCMD("2,2,0,0,0", GSM_CMD_SMS_INDICATION));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_SMS_INDICATION) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ULong2ModemCMD(1,1, GSM_CMD_HEX_MODE));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_HEX_MODE) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_TIME_ZONE));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_TIME_ZONE) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ULong2ModemCMD(1,2, GSM_CMD_DTFM));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_DTFM) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_NETWORK_TYPE_STATUS));
        }  else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_NETWORK_TYPE_STATUS) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmHandler->ForceCommand(new ByteModemCMD(1, GSM_CMD_CALL_STATUS));
        }  else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_CALL_STATUS) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            initState = GSM_STATE_WAIT_CREG;
            gsmHandler->ForceCommand(new BaseModemCMD(GSM_CMD_NETWORK_REG, MODEM_COMMAND_TIMEOUT, true));
        } else if (type >= MODEM_RESPONSE_ERROR) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_NETWORK_QUALITY) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
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
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        } else if (type >= MODEM_RESPONSE_ERROR) {
            // TODO: ?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_TIME) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
            size_t cclkShift = strlen(GSM_CMD_TIME) + 2;
            if (cclkShift < respLen) {
                char *stamp = ShiftQuotations(response + cclkShift);
                tmz tmzStruct;
                currentTime.utcStamp = ExtractTime(stamp, &tmzStruct);
                currentTime.quaterZone = tmzStruct.quaterZone;
                currentTime.utcStamp -= ZoneInSeconds(tmzStruct.quaterZone);
                if (currentTime.utcStamp != 0) {
                    syncTS = millis();
                }
            }
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        } else if (type >= MODEM_RESPONSE_ERROR) {
            // TODO: ?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_SMS_SEND) == 0) {
        SMSSendModemCMD *sendSMSCmd = (SMSSendModemCMD *)request;
        if (type == MODEM_RESPONSE_DATA) {
            if (strcmp(request->ExtraTriggerValue(), response) == 0) {
                request->SetHasExtraTrigger(false);
                Stream *modemStream = gsmHandler->GetModemStream();
                if (listener != NULL && listener->OnSMSSendStream(modemStream, sendSMSCmd->phoneNumber, sendSMSCmd->customData)) {
                    //Send message end
                    modemStream->write(CRTLZ_ASCII_SYMBOL);
                } else {
                    // Send message cancel callback
                    modemStream->write(ESC_ASCII_SYMBOL);
                }
            } else if (strcmp(GSM_CMD_SMS_SEND, response) == 0) {
                // Got result message reference
                // +CMGS: 1
                // Have nothing to do with that
            }
        } else {
            // Do nothing?
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
            HandleGSMFail(GSM_FAIL_REG_NETWORK);
            break;
    }
}

void GSMNetworkHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == gsmReconnectTimer) {
        gsmReconnectTimer = 0;
        gsmHandler->StartModem(true, gsmHandler->GetBaudRate());
        return;
    }
    if (timerId == gsmSimTimer) {
        gsmSimTimer = 0;
        gsmHandler->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
        return;
    }
    if (timerId == gsmNetStatsTimer) {
        gsmNetStatsTimer = 0;
        FetchModemStats();
        return;
    }
}
void GSMNetworkHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == gsmReconnectTimer) {
        gsmReconnectTimer = 0;
        return;
    }
    if (timerId == gsmSimTimer) {
        gsmSimTimer = 0;
        return;
    }
    if (timerId == gsmNetStatsTimer) {
        gsmNetStatsTimer = 0;
        return;
    }
}

void GSMNetworkHandler::FetchModemStats() {
    Timer::Stop(gsmNetStatsTimer);
    gsmNetStatsTimer = Timer::Start(this, QUALITY_CHECK_DURATION, 0);
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
    if (currentTime.utcStamp == 0) return 0;
    unsigned long syncDelta = (millis() - syncTS) / 1000ul;
    return currentTime.utcStamp + syncDelta;
}
time_t GSMNetworkHandler::GetLocalTime()
{
    if (currentTime.utcStamp == 0) return 0;
    return GetUTCTime() + ZoneInSeconds(currentTime.quaterZone);
}

GSMNetworkStats *GSMNetworkHandler::GetGSMStats() {
    return &gsmStats;
}

bool GSMNetworkHandler::InitSendSMS(char *phone, uint8_t customData)
{
    return gsmHandler->AddCommand(new SMSSendModemCMD(phone, GSM_CMD_SMS_SEND, customData));
}

void GSMNetworkHandler::OnModemReboot()
{
    initState = GSM_STATE_NONE;
    gsmStats.regState = GSM_REG_STATE_UNKNOWN;
    gsmStats.networkType = GSM_NETWORK_UNKNOWN;
    gsmStats.thresholdState = GSM_THRESHOLD_T;
    gsmStats.signalStrength = 0;
    gsmStats.signalQuality = 0;
    StopTimers();

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

void GSMNetworkHandler::StopTimers()
{
    Timer::Stop(gsmNetStatsTimer);
    Timer::Stop(gsmReconnectTimer);
    Timer::Stop(gsmSimTimer);
}

void GSMNetworkHandler::HandleGSMFail(GSM_FAIL_STATE failState)
{
    StopTimers();
    if (listener != NULL) {
        listener->OnGSMFailed(GSM_FAIL_OTHER_PIN);
    }
}