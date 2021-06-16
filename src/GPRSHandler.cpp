#include "GPRSHandler.h"

GPRSHandler::GPRSHandler(BaseGSMHandler *gsmHandler)
{
    deviceAddr.dword = 0;
    this->gsmHandler = gsmHandler;
}
GPRSHandler::~GPRSHandler()
{
    FlushAuthData();
}


void GPRSHandler::SetAPNHandler(IGPRSHandler *apnHandler)
{
    this->apnHandler = apnHandler;
}


void GPRSHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == checkTimer) {
        checkTimer = 0;
        gsmHandler->ForceCommand("AT+UPSND=0,8");
    }
}
void GPRSHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == checkTimer) {
        checkTimer = 0;
    }
}

void GPRSHandler::FlushAuthData()
{
    deviceAddr.dword = 0;

    if (this->apn != NULL) {
        free(this->apn);
        this->apn = NULL;
    }
    if (this->login != NULL) {
        free(this->login);
        this->login = NULL;
    }
    if (this->password != NULL) {
        free(this->password);
        this->password = NULL;
    }
    Timer::Stop(checkTimer);
}

bool GPRSHandler::Connect(char* apn, char* login, char* password)
{
    if (apnState != GSM_APN_DEACTIVATED) {
        return false;
    }
    FlushAuthData();
    
    if (apn != NULL) {
        this->apn = (char*)malloc(strlen(apn) + 1);
        strcpy(this->apn, apn);
    }
    if (login != NULL) {
        this->login = (char*)malloc(strlen(login) + 1);
        strcpy(this->login, login);
    }
    if (password != NULL) {
        this->password = (char*)malloc(strlen(password) + 1);
        strcpy(this->password, password);
    }
    Connect();
    return true;
}

bool GPRSHandler::Connect()
{
    if (apnState == GSM_APN_DEACTIVATED) {
        apnState = GSM_APN_ACTIVATING;
        SendSetting(UPSD_SETTING_APN);
        return true;
    }
    return false;
}

bool GPRSHandler::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    if (strlen(request) > 2) {
        request+=2;
        char reqData[128];

        if (IsRequestCMD(request, GSM_GPRS_CMD, strlen(GSM_GPRS_CMD))) {
            int state = atoi(request + strlen(GSM_GPRS_CMD) + 1);
            if (state == 0) {
                if (type >= MODEM_RESPONSE_OK) {
                    // No need to deactivate Profile
                    apnState = GSM_APN_DEACTIVATED;
                    if (apnHandler != NULL) {
                        apnHandler->OnGPRSStopped(type == MODEM_RESPONSE_OK);
                    }
                }
            }
            return true;
        }
        else if (IsRequestCMD(request, GSM_APN_CONTROL_CMD, strlen(GSM_APN_CONTROL_CMD))) {
            // "AT+UPSDA=0,3"
            strcpy(reqData, request + strlen(GSM_APN_CONTROL_CMD) + 1);
            char *upsdArgs[2];
            SplitString(reqData, ',', upsdArgs, 2, false);

            int state = atoi(upsdArgs[1]);
            if (state == 3) { // Connect command
                if (type == MODEM_RESPONSE_OK) {
                    Timer::Stop(checkTimer);
                    checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
                } else if (type > MODEM_RESPONSE_OK) {
                    apnState = GSM_APN_DEACTIVATING;
                    gsmHandler->ForceCommand("AT+CGATT=0", APN_CONNECT_CMD_TIMEOUT);
                }
            }
            // No check for deactivation AT+CGATT=0 will handle that
            return true;
        }
        else if (IsRequestCMD(request, GSM_APN_FETCH_DATA_CMD, strlen(GSM_APN_FETCH_DATA_CMD))) {
            strcpy(reqData, request + strlen(GSM_APN_FETCH_DATA_CMD) + 1);
            char *upsndArgs[2];
            SplitString(reqData, ',', upsndArgs, 2, false);
            int state = atoi(upsndArgs[1]);

            if (type == MODEM_RESPONSE_DATA) {
                char *upsndRespArgs[3];
                SplitString(response, ',', upsndRespArgs, 3, false);

                if (state == UPSDN_SETTING_STATUS) { // Connection
                    if (strncmp(upsndRespArgs[2], "1", 1) == 0) {
                        gsmHandler->ForceCommand("AT+UPSND=0,0");
                    } else {
                        Timer::Stop(checkTimer);
                        checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
                    }
                } else if (state == UPSDN_SETTING_IP) { // ADDR
                    GetAddr(upsndRespArgs[2], &deviceAddr);
                }
            }
            else if (type > MODEM_RESPONSE_OK) {
                if (state == UPSDN_SETTING_STATUS) { // Connection
                    Timer::Stop(checkTimer);
                    checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
                } else if (state == UPSDN_SETTING_IP) {
                    // TODO: error on ip fetch?
                    gsmHandler->ForceCommand("AT+CGATT=0");
                }
            }
            else if (type == MODEM_RESPONSE_OK) {
                if (state == UPSDN_SETTING_IP) {
                    apnState = GSM_APN_ACTIVE;
                    if (apnHandler != NULL) {
                        apnHandler->OnGPRSStarted(type == MODEM_RESPONSE_OK);
                    }
                }
            }
            return true;
        }
        else if (IsRequestCMD(request, GSM_APN_CONFIG_CMD, strlen(GSM_APN_CONFIG_CMD))) {
            strcpy(reqData, request + strlen(GSM_APN_FETCH_DATA_CMD) + 1);
            char *upsdArgs[3];
            SplitString(reqData, ',', upsdArgs, 3, false);
            int setting = atoi(upsdArgs[1]);
            if (type == MODEM_RESPONSE_OK) {
                UPSD_SETTING_INDEX next = GetNextSetting(setting);
                if (next == setting) {
                    // Enable APN, GATT will enable itself
                    gsmHandler->ForceCommand("AT+UPSDA=0,3", APN_CONNECT_CMD_TIMEOUT);
                } else {
                    SendSetting(next);
                }
            } else if (type > MODEM_RESPONSE_OK) {
                // ERROR
                apnState = GSM_APN_DEACTIVATING;
                gsmHandler->ForceCommand("AT+CGATT=0", APN_CONNECT_CMD_TIMEOUT);
            }
            return true;
        }
    }
    return false;
}
bool GPRSHandler::OnGSMEvent(char * data)
{
    if (strncmp(data, GSM_APN_DEACTIVATED_EVENT, strlen(GSM_APN_DEACTIVATED_EVENT)) == 0) {
        apnState = GSM_APN_DEACTIVATING;
        gsmHandler->ForceCommand("AT+CGATT=0", APN_CONNECT_CMD_TIMEOUT);
        return true;
    }

    return false;
}

UPSD_SETTING_INDEX GPRSHandler::GetNextSetting(int index)
{
    index++;
    if (index < UPSD_SETTING_APN) {
        index = UPSD_SETTING_APN;
    } else if (index > UPSD_SETTING_PSWD) {
        index = UPSD_SETTING_ASSIGNED_IP;
    }
    return (UPSD_SETTING_INDEX)index;
}

void GPRSHandler::SendSetting(UPSD_SETTING_INDEX setting)
{
    const size_t cmdSize = 128;
    char cmd[cmdSize];

    switch (setting)
    {
    case UPSD_SETTING_APN:
        snprintf(cmd, cmdSize, "%s%s=0,1,\"%s\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD, apn);
        break;
    case UPSD_SETTING_LOGIN:
        if (login == NULL) {
            snprintf(cmd, cmdSize, "%s%s=0,2,\"\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD);
        } else {
            snprintf(cmd, cmdSize, "%s%s=0,2,\"%s\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD, login);
        }
        break;
    case UPSD_SETTING_PSWD:
        if (password == NULL) {
            snprintf(cmd, cmdSize, "%s%s=0,3,\"\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD);
        } else {
            snprintf(cmd, cmdSize, "%s%s=0,3,\"%s\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD, password);
        }
        break;
    default:
        snprintf(cmd, cmdSize, "%s%s=0,7,\"0.0.0.0\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD);
        break;
    }
    gsmHandler->ForceCommand(cmd);
}

bool GPRSHandler::Deactivate()
{
    if (apnState == GSM_APN_ACTIVE) {
        apnState = GSM_APN_DEACTIVATING;
        // No need to trigger profile OFF command
        gsmHandler->ForceCommand("AT+CGATT=0", APN_CONNECT_CMD_TIMEOUT);
        return true;
    }
    return false;
}

bool GPRSHandler::IsActive()
{
    return apnState == GSM_APN_ACTIVE;
}

IPAddr GPRSHandler::GetDeviceAddr()
{
    return IPAddr(deviceAddr);
}
void GPRSHandler::OnModemReboot()
{
    FlushAuthData();
    apnState = GSM_APN_DEACTIVATED;
}