#include <time.h>
#include "GSMNetworkManager.h"
#include "command/CharModemCMD.h"
#include "command/ByteModemCMD.h"
#include "command/ULong2ModemCMD.h"
#include "command/SMSSendModemCMD.h"
#include "command/PinModemCMD.h"
#include "common/GSMUtils.h"

GSMNetworkManager::GSMNetworkManager(GSMModemManager *modemManager):
    gsmReconnectTimer(this),
    gsmSimTimer(this),
    gsmNetStatsTimer(this),
    gsmCREGTimer(this)
{
    this->modemManager = modemManager;
    gsmStats.Reset();
}
GSMNetworkManager::~GSMNetworkManager()
{
    
}

void GSMNetworkManager::SetGSMListener(IGSMNetworkManager *listener)
{
    this->listener = listener;
}

void GSMNetworkManager::Connect(const char *simPin)
{
    this->simPin = simPin;
    gsmStats.regState = GSM_REG_STATE_IDLE;
    initState = GSM_STATE_PIN;
    gsmReconnectTimer.Stop();

    modemManager->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
}

bool GSMNetworkManager::OnGSMEvent(char * data, size_t dataLen)
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
        UpdateCregResult(GetCregState(data, dataLen));
        return true;
    }
    
    return false;
}

bool GSMNetworkManager::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    // Network registration commands
    if (strcmp(request->cmd, GSM_SIM_PIN_CMD) == 0) {
        if (request->GetIsCheck()) {
            PinStatusModemCMD *pinStatusCMD = (PinStatusModemCMD *)request;
            // AT+CPIN? confirmation
            // State should be changed on data read
            if (type == MODEM_RESPONSE_OK) {
                if (pinStatusCMD->pinState == GSM_PIN_STATE_SIM_PIN) {
                    modemManager->ForceCommand(new CharModemCMD(simPin, GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT, true));
                } else if (pinStatusCMD->pinState == GSM_PIN_STATE_READY) {
                    HandleSimUnlocked();
                } else {
                    HandleGSMFail(GSM_FAIL_OTHER_PIN);
                }
            } else if (type == MODEM_RESPONSE_DATA) {
                pinStatusCMD->HandleDataContent(response, respLen);
            } else if (type >= MODEM_RESPONSE_ERROR && type <= MODEM_RESPONSE_CANCELED) {
                // Resend cmd, pin module not ready
                //gsmSimTimer = Timer::Start(this, GSM_MODEM_SIM_PIN_DELAY, 0);
                gsmSimTimer.StartMicros(GSM_MODEM_SIM_PIN_DELAY);
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
            if (type == MODEM_RESPONSE_DATA) {
                UpdateCregResult(GetCregState(response, respLen));
            } else if (type== MODEM_RESPONSE_OK) {
                gsmCREGTimer.StartMicros(GSM_NETWORG_CREG_INTERVAL);
            } else if (type >= MODEM_RESPONSE_ERROR) {
                modemManager->StartModem();
            }
        } else {
            if (type >= MODEM_RESPONSE_ERROR) {
                modemManager->StartModem();
            }
        }
        return true;
    }

    // Modem output configuration commands:
    if (strcmp(request->cmd, GSM_CMD_SMS_FROMAT) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            NextConfigurationStep();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            modemManager->StartModem();
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_SMS_INDICATION) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            NextConfigurationStep();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            modemManager->StartModem();
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_TIME_ZONE) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            NextConfigurationStep();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            modemManager->StartModem();
        }
        return true;
    }

    // Functionality commands:
    if (strcmp(request->cmd, GSM_CMD_NETWORK_QUALITY) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
            char* csq = response + strlen(GSM_CMD_NETWORK_QUALITY) + 2;
            char* csqArgs[2];
            SplitString(csq, ',', csqArgs, 2, false);
            UpdateSignalQuality(atoi(csqArgs[0]), atoi(csqArgs[1]));
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            modemManager->StartModem();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            // TODO: ?
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_SMS_SEND) == 0) {
        SMSSendModemCMD *sendSMSCmd = (SMSSendModemCMD *)request;
        if (type == MODEM_RESPONSE_EXTRA_TRIGGER) {
            Stream *modemStream = modemManager->GetModemStream();
            bool success = (listener != NULL && listener->OnSMSSendStream(modemStream, sendSMSCmd->phoneNumber, sendSMSCmd->customData));
            sendSMSCmd->EndStream(modemStream, !success);
        } else if (type == MODEM_RESPONSE_DATA) {
            if (strncmp(GSM_CMD_SMS_SEND, response, strlen(GSM_CMD_SMS_SEND)) == 0) {
                // Got result message reference
                // +CMGS: 1
                // Have nothing to do with that
            }
        } else {
            // Do nothing?
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
            modemManager->StartModem();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            // TODO: ?
        }
        return true;
    }


    return false;
}


//strength(rssi): (0-31,99), quality(ber): (0-7,99)
void GSMNetworkManager::UpdateSignalQuality(int strength, int quality)
{
    gsmStats.signalStrength = strength; // (0 - 31), 99
    if (gsmStats.signalStrength > 31) {
        gsmStats.signalStrength = 0;
    }
    gsmStats.signalStrength = ((double)gsmStats.signalStrength / 31.0) * 100.0;

    gsmStats.signalQuality = quality; // (0 - 7), 99
    if (gsmStats.signalQuality > 7) {
        gsmStats.signalQuality = 0;
    }
    gsmStats.signalQuality = ((7.0 - (double)gsmStats.signalQuality) / 7.0) * 100.0;

    if (listener != NULL) {
        listener->OnGSMQuality(gsmStats.signalStrength, gsmStats.signalQuality);
    }
}

void GSMNetworkManager::HandleSimUnlocked()
{
    isSimUnlocked = true;
    initState = GSM_STATE_WAIT_CREG;
    NextConfigurationStep();
    if (listener != NULL) {
        listener->OnGSMSimUnlocked();
    }
}

void GSMNetworkManager::NextConfigurationStep()
{
    configurationStep++;
    if (configurationStep >= GSM_MODEM_CONFIGURATION_STEP_COMPLETE) {
        ContinueConfigureModem();
        return;
    }

    switch (configurationStep) {
        case GSM_MODEM_CONFIGURATION_STEP_NONE:
            // TODO: ?
            break;
        case GSM_MODEM_CONFIGURATION_STEP_CNMI:
            modemManager->ForceCommand(new CharModemCMD("2,2,0,0,0", GSM_CMD_SMS_INDICATION));
            break;
        case GSM_MODEM_CONFIGURATION_STEP_CMFG:
            modemManager->ForceCommand(new ByteModemCMD(1, GSM_CMD_SMS_FROMAT));
            break;
        case GSM_MODEM_CONFIGURATION_STEP_CTZU:
            modemManager->ForceCommand(new ByteModemCMD(1, GSM_CMD_TIME_ZONE));
            break;
    }
}

// Override to handle modem specific configuration
void GSMNetworkManager::ContinueConfigureModem()
{
    ConfigureModemCompleted();
}

// Trigger when configuration completes
void GSMNetworkManager::ConfigureModemCompleted()
{
    //Timer::Stop(gsmCREGTimer);
    //gsmCREGTimer = Timer::Start(this, GSM_NETWORG_CREG_INTERVAL, 0);
    gsmCREGTimer.StartMicros(GSM_NETWORG_CREG_INTERVAL);
    gsmReconnectTimer.StartMicros(GSM_NETWORG_REG_TIMEOUT);
    modemManager->ForceCommand(new ByteModemCMD(1, GSM_CMD_NETWORK_REG));
}

GSM_REG_STATE GSMNetworkManager::GetCregState(char * data, size_t dataLen)
{
    char *creg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
    char *cregArgs[2];
    cregArgs[1] = 0;

    size_t args = SplitString(creg, ',', cregArgs, 2, false);
    if (args == 2) { // Data from command
        return (GSM_REG_STATE)atoi(cregArgs[1]);
    }
    // Data from event
    return (GSM_REG_STATE)atoi(cregArgs[0]);
}

void GSMNetworkManager::UpdateCregResult(GSM_REG_STATE state)
{
    if (state == gsmStats.regState) {
        return;
    }

    Serial.print("CREG: ");
    Serial.print((int)gsmStats.regState);
    Serial.print("->");
    Serial.println((int)state);

    gsmStats.regState = state;

    switch (gsmStats.regState) {
        case GSM_REG_STATE_IDLE:
        case GSM_REG_STATE_CONNECTING:
            gsmReconnectTimer.StartMicros(GSM_NETWORG_REG_TIMEOUT);
            break;
        case GSM_REG_STATE_CONNECTED_HOME:
        case GSM_REG_STATE_CONNECTED_ROAMING:
        case GSM_REG_STATE_CONNECTED_SMS_ONLY_HOME:
        case GSM_REG_STATE_CONNECTED_SMS_ONLY_ROAMING:
        case GSM_REG_STATE_CONNECTED_EMERGENSY_ONLY:
        case GSM_REG_STATE_CONNECTED_CSFB_NOT_HOME:
        case GSM_REG_STATE_CONNECTED_CSFB_NOT_ROAMING:
            gsmReconnectTimer.Stop();
            if (initState > GSM_STATE_NONE && initState < GSM_STATE_READY) {
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
            gsmReconnectTimer.Stop();
            // TODO: or give modem chance to connect?
            HandleGSMFail(GSM_FAIL_REG_NETWORK);
            break;
    }
}

void GSMNetworkManager::OnTimerComplete(Timer * timer)
{
    if (timer == &gsmReconnectTimer) {
        modemManager->StartModem();
        return;
    }
    if (timer == &gsmSimTimer) {
        modemManager->ForceCommand(new PinStatusModemCMD(GSM_SIM_PIN_CMD, MODEM_COMMAND_TIMEOUT));
        return;
    }
    if (timer == &gsmNetStatsTimer) {
        FetchModemStats();
        return;
    }
    if (timer == &gsmCREGTimer) {
        modemManager->ForceCommand(new BaseModemCMD(GSM_CMD_NETWORK_REG, MODEM_COMMAND_TIMEOUT, true));
        return;
    }
}

void GSMNetworkManager::FetchModemStats() {
    gsmNetStatsTimer.StartMicros(QUALITY_CHECK_DURATION);
    modemManager->AddCommand(new BaseModemCMD(GSM_CMD_TIME, MODEM_COMMAND_TIMEOUT, true));
    modemManager->AddCommand(new BaseModemCMD(GSM_CMD_NETWORK_QUALITY));
}

bool GSMNetworkManager::IsInitialized()
{
    return initState == GSM_STATE_READY;
}

bool GSMNetworkManager::IsSimUnlocked()
{
    return isSimUnlocked;
}

time_t GSMNetworkManager::GetUTCTime()
{
    if (currentTime.utcStamp == 0) return 0;
    unsigned long syncDelta = (millis() - syncTS) / 1000ul;
    return currentTime.utcStamp + syncDelta;
}
time_t GSMNetworkManager::GetLocalTime()
{
    if (currentTime.utcStamp == 0) return 0;
    return GetUTCTime() + ZoneInSeconds(currentTime.quaterZone);
}

GSMNetworkStats GSMNetworkManager::GetGSMStats() {
    return gsmStats;
}

bool GSMNetworkManager::InitSendSMS(char *phone, uint8_t customData)
{
    return modemManager->AddCommand(new SMSSendModemCMD(phone, GSM_CMD_SMS_SEND, customData));
}

void GSMNetworkManager::OnModemReboot()
{
    configurationStep = GSM_MODEM_CONFIGURATION_STEP_NONE;
    initState = GSM_STATE_NONE;
    gsmStats.Reset();
    StopTimers();

    FlushIncomingSMS();
}

void GSMNetworkManager::FlushIncomingSMS()
{
    if (incomingSms != NULL) {
        delete incomingSms;
        incomingSms = NULL;
    }
}

void GSMNetworkManager::StopTimers()
{
    gsmNetStatsTimer.Stop();
    gsmReconnectTimer.Stop();
    gsmSimTimer.Stop();
    gsmCREGTimer.Stop();
}

void GSMNetworkManager::HandleGSMFail(GSM_FAIL_STATE failState)
{
    StopTimers();
    if (listener != NULL) {
        listener->OnGSMFailed(failState);
    }
}


void GSMNetworkManager::UpdateThresoldState(GSM_THRESHOLD_STATE state)
{
    gsmStats.thresholdState = state;

    if (listener != NULL) {
        listener->OnGSMThreshold(gsmStats.thresholdState);
    }
}
void GSMNetworkManager::UpdateNetworkType(GSM_NETWORK_TYPE type)
{
    gsmStats.networkType = type;
    if (listener != NULL) {
        listener->OnGSMNetworkType(gsmStats.networkType);
    }
}
void GSMNetworkManager::UpdateTemperature(float temp)
{
    gsmStats.temperature = temp;
}

GSM_INIT_STATE GSMNetworkManager::GetInitState()
{
    return initState;
}
GSM_MODEM_CONFIGURATION_STEP GSMNetworkManager::GetConfigurationStep()
{
    return (GSM_MODEM_CONFIGURATION_STEP)configurationStep;
}