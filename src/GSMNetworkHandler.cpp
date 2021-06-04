#define _XOPEN_SOURCE
#include <time.h>
#include "GSMNetworkHandler.h"

GSMNetworkHandler::GSMNetworkHandler(BaseGSMHandler *gsmHandler):GSMCallHandler(gsmHandler)
{
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
    regState = GSM_REG_STATE_UNKNOWN;
    initState = GSM_STATE_CHECK_SIM;
    TriggerCommand();
}

bool GSMNetworkHandler::OnGSMEvent(char * data)
{
    // Handle +CREG?
    
    if (strncmp(data, GSM_CMD_NETWORK_REG, strlen(GSM_CMD_NETWORK_REG)) == 0) {

        char *creg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
		char *cregArgs[2];
        cregArgs[1] = 0;

		SplitString(creg, ',', cregArgs, 2, false);
        if (cregArgs[1] == 0) {
            regState = (GSM_REG_STATE)atoi(cregArgs[0]);
        } else {
		    regState = (GSM_REG_STATE)atoi(cregArgs[1]);
        }

        Serial.print("regState: ");
        Serial.println((int)regState);
        
        switch (regState) {
            case GSM_REG_STATE_CONNECTING_HOME:
            case GSM_REG_CONNECTED_HOME:
            case GSM_REG_STATE_CONNECTING_OTHER:
            case GSM_REG_STATE_CONNECTED_ROAMING:
            case GSM_REG_STATE_CONNECTED_SMS_ONLY_HOME:
            case GSM_REG_STATE_CONNECTED_SMS_ONLY_ROAMING:
            case GSM_REG_STATE_CONNECTED_EMERGENSY_ONLY:
            case GSM_REG_STATE_CONNECTED_CSFB_NOT_HOME:
            case GSM_REG_STATE_CONNECTED_CSFB_NOT_ROAMING:
                if (initState == GSM_STATE_WAIT_REG_NETWORK) {
                    IncreaseState();
                    TriggerCommand();
                }

                if (listener != NULL) {
                    listener->OnGSMStatus(regState);
                }
                break;
            case GSM_REG_STATE_DENIED:
            case GSM_REG_STATE_UNKNOWN:
                if (listener != NULL) {
                    listener->OnGSMFailed(false);
                }
                break;
        }
        return true;
    }
    if (strncmp(data, GSM_NETWORK_TYPE_STATUS, strlen(GSM_NETWORK_TYPE_STATUS)) == 0) {
        char *ureg = data + strlen(GSM_CMD_NETWORK_REG) + 2;
        networkType = (GSM_NETWORK_TYPE)atoi(ureg);
        if (listener != NULL) {
            listener->OnGSMNetworkType(networkType);
        }
    }
    
    return GSMCallHandler::OnGSMEvent(data);
}

bool GSMNetworkHandler::OnGSMResponse(char *request, char *response, MODEM_RESPONSE_TYPE type)
{

    //Serial.print("GSMNetworkHandler::OnGSMResponse [");
    //Serial.print(response);
    //Serial.print("] ");
    //Serial.println(result);

    // Reponse contains only OK/ERROR/... status
    if (type >= MODEM_RESPONSE_OK)
    {
        // Length must be greater than AT.. command
        if (strlen(request) > 2) {

            request+=2; // Skip "AT"

            if (strncmp(request, GSM_SIM_PIN_CMD, strlen(GSM_SIM_PIN_CMD)) == 0) {

                if (request[strlen(GSM_SIM_PIN_CMD)] == '?') {
                    // AT+CPIN? confirmation
                    // State should be changed on data read
                    TriggerCommand();
                    return true;
                } else {
                    // AT+CPIN="" confirmation on pin insert
                    if (type == MODEM_RESPONSE_OK) {
                        isSimUnlocked = true;
                        initState = GSM_STATE_AUTO_NETWOR_STATE;
                        TriggerCommand();
                        if (listener != NULL) {
                            listener->OnGSMSimUnlocked();
                        }
                    } else {
                        initState = GSM_STATE_ERROR;
                        if (listener != NULL) {
                            listener->OnGSMFailed(true);
                        }
                    }
                }

                
                return true;
            }

            if (strncmp(request, GSM_CMD_NETWORK_REG, strlen(GSM_CMD_NETWORK_REG)) == 0 ||
                strncmp(request, GSM_NETWORK_TYPE_STATUS, strlen(GSM_NETWORK_TYPE_STATUS)) == 0 ||
                strncmp(request, GSM_CMD_MSG_FROMAT, strlen(GSM_CMD_MSG_FROMAT)) == 0 ||
                strncmp(request, GSM_CMD_HEX_MODE, strlen(GSM_CMD_HEX_MODE)) == 0 ||
                strncmp(request, GSM_CMD_TIME_ZONE, strlen(GSM_CMD_TIME_ZONE)) == 0 ||
                strncmp(request, GSM_CMD_DTFM, strlen(GSM_CMD_DTFM)) == 0 ||
                strncmp(request, GSM_CMD_CALL_STATUS, strlen(GSM_CMD_CALL_STATUS)) == 0)
            {
                if (type == MODEM_RESPONSE_OK) {
                    IncreaseState();
                    TriggerCommand();
                } else {
                    Timer::Stop(delayedRequest);
                    delayedRequest = Timer::Start(this, REQUEST_DELAY_DURATION);
                }
                return true;
            }
        }
        return false;
    }

    if (strncmp(response, GSM_SIM_PIN_CMD, strlen(GSM_SIM_PIN_CMD)) == 0) {

        // SIM PIN
        char *simState = response + strlen(GSM_SIM_PIN_CMD) + 2;

		if (strncmp(GSM_SIM_PIN_READY, simState, strlen(GSM_SIM_PIN_READY)) == 0) {
			// NO PIN REQUIRED
            initState = GSM_STATE_AUTO_NETWOR_STATE;
            //TriggerCommand();
		}
		else if (strncmp(GSM_SIM_PIN_REQUIRE, simState, strlen(GSM_SIM_PIN_REQUIRE)) == 0) {
			// PIN REQUIRE
            initState = GSM_STATE_ENTER_SIM;
            //TriggerCommand();
		}
		else {
            if (initState == GSM_STATE_ENTER_SIM) {
                initState = GSM_STATE_ERROR;
                if (listener != NULL) {
                    listener->OnGSMFailed(true);
                }
            }
            // else {
            //    TriggerCommand();
            //}
		}

        return true;
    }
    if (strncmp(GSM_CMD_NETWORK_QUALITY, response, strlen(GSM_CMD_NETWORK_QUALITY)) == 0) {
        char* csq = response + strlen(GSM_CMD_NETWORK_QUALITY) + 2;
        char* csqArgs[2];

        SplitString(csq, ',', csqArgs, 2, false);

        signalStrength = atoi(csqArgs[0]); // 0 - 31, 99
        if (signalStrength > 31) {
            signalStrength = 0;
        }
        signalStrength = ((double)signalStrength / 31.0) * 100.0;

        signalQuality = atoi(csqArgs[1]); // 0 - 7, 99
        if (signalQuality > 7) {
            signalQuality = 0;
        }
        signalQuality = ((7.0 - (double)signalQuality) / 7.0) * 100.0;

        if (listener != NULL) {
            listener->OnGSMQuality(signalStrength, signalQuality);
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
    return GSMCallHandler::OnGSMResponse(request, response, type);
}

void GSMNetworkHandler::IncreaseState()
{
    int val = (int)initState;
    if (initState < GSM_STATE_DONE) {
        val++;
        initState = (GSM_INIT_STATE)val;
    }
}

void GSMNetworkHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == delayedRequest) {
        delayedRequest = 0;
        if (data == 0) {
            TriggerCommand();
        } else if (data == 1u) {
            FetchTime();
        }
    }
}
void GSMNetworkHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == delayedRequest) {
        delayedRequest = 0;
    }
}

void GSMNetworkHandler::TriggerCommand()
{
    Timer::Stop(delayedRequest);
    const size_t cmdSize = 128;
    char cmd[cmdSize];
    switch (initState)
    {
    case GSM_STATE_CHECK_SIM:
        snprintf(cmd, cmdSize, "%s%s?", GSM_PREFIX_CMD, GSM_SIM_PIN_CMD);
        break;
    case GSM_STATE_ENTER_SIM:
        snprintf(cmd, cmdSize, "%s%s=\"%s\"", GSM_PREFIX_CMD, GSM_SIM_PIN_CMD, simPin);
        break;
    case GSM_STATE_AUTO_NETWOR_STATE:
        snprintf(cmd, cmdSize, "%s%s=1", GSM_PREFIX_CMD, GSM_CMD_NETWORK_REG);
        break;
    case GSM_STATE_PREFERRED_MESSAGE_FORMAT:
        snprintf(cmd, cmdSize, "%s%s=1", GSM_PREFIX_CMD, GSM_CMD_MSG_FROMAT);
        break;
    case GSM_STATE_HEX_MODE:
        snprintf(cmd, cmdSize, "%s%s=1,1", GSM_PREFIX_CMD, GSM_CMD_HEX_MODE);
        break;
    case GSM_STATE_AUTOMATIC_TIME_ZONE:
        snprintf(cmd, cmdSize, "%s%s=1", GSM_PREFIX_CMD, GSM_CMD_TIME_ZONE);
        break;
    case GSM_STATE_DTMF_DETECTION:
        snprintf(cmd, cmdSize, "%s%s=1,2", GSM_PREFIX_CMD, GSM_CMD_DTFM);
        break;
    case GSM_STATE_WAIT_REG_NETWORK:
        // Nothing to do here, wait for connection
        return;
    case GSM_STATE_NETWORK_TYPE:
        snprintf(cmd, cmdSize, "%s%s=1", GSM_PREFIX_CMD, GSM_NETWORK_TYPE_STATUS);
        break;
    case GSM_STATE_CALL_STATUS:
        snprintf(cmd, cmdSize, "%s%s=1", GSM_PREFIX_CMD, GSM_CMD_CALL_STATUS);
        break;
    case GSM_STATE_DONE:
        if (currentTime == 0) {
            FetchTime();
        }
        if (listener != NULL) {
            listener->OnGSMConnected();
        }
        return;
    case GSM_STATE_ERROR:
        return;
    }
    gsmHandler->ForceCommand(cmd);
}

void GSMNetworkHandler::FetchTime() {
    Timer::Stop(delayedRequest);
    delayedRequest = Timer::Start(this, QUALITY_CHECK_DURATION, 1u);
    gsmHandler->AddCommand("AT+CCLK?");
    gsmHandler->AddCommand("AT+CSQ");
}

bool GSMNetworkHandler::IsInitialized()
{
    return initState == GSM_STATE_DONE;
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
    unsigned long syncDelta = (millis() - syncTS) / 1000ul;
    return currentTime + syncDelta;
}

uint8_t GSMNetworkHandler::GetSignalStrength()
{
    return signalStrength;
}
uint8_t GSMNetworkHandler::GetSignalQuality()
{
    return signalQuality;
}