#include "GSMApn.h"

GSMApn::GSMApn(BaseGSMHandler *gsmHandler)
{
    apnAddr.dword = 0;
    deviceAddr.dword = 0;
    this->gsmHandler = gsmHandler;
}
GSMApn::~GSMApn()
{
    FlushAuthData();
}


void GSMApn::SetAPNHandler(IGSMApnHandler *apnHandler)
{
    this->apnHandler = apnHandler;
}

void GSMApn::FlushAuthData()
{
    apnAddr.dword = 0;
    deviceAddr.dword = 0;

    if (this->login != NULL) {
        free(this->login);
        this->login = NULL;
    }
    if (this->password != NULL) {
        free(this->password);
        this->password = NULL;
    }
}

bool GSMApn::Connect(char* apn, char* login, char* password)
{
    if (apnState != GSM_APN_DEACTIVATED) {
        return false;
    }
    FlushAuthData();
    
    GetAddr(apn, &apnAddr);

    if (login != NULL) {
        this->login = (char*)malloc(strlen(apn) + 1);
        strcpy(this->login, login);
    }
    if (password != NULL) {
        this->password = (char*)malloc(strlen(apn) + 1);
        strcpy(this->password, password);
    }
    Connect();
    return true;
}

bool GSMApn::Connect()
{
    if (apnState == GSM_APN_DEACTIVATED) {
        apnState = GSM_APN_ACTIVATING;
        SendSetting(UPSD_SETTING_APN);
        return true;
    }
    return false;
}

bool GSMApn::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    if (strlen(request) > 2) {
        request+=2;
        if (strncmp(request, GSM_APN_CONFIG_CMD, strlen(GSM_APN_CONFIG_CMD)) == 0) {
            char *upsd = request + strlen(GSM_APN_CONFIG_CMD) + 1;
            char *upsdArgs[3];
            SplitString(upsd, ',', upsdArgs, 3, false);
            int setting = atoi(upsdArgs[1]);
            if (setting == 3) { // Activate command
                if (apnHandler != NULL) {
                    apnHandler->OnApnActivated(type == MODEM_RESPONSE_OK);
                }
            } else if (setting == 4) { // Deactivate command
                if (apnHandler != NULL) {
                    apnHandler->OnApnDeactivated(type == MODEM_RESPONSE_OK);
                }
            } else {
                if (type == MODEM_RESPONSE_OK) {
                    UPSD_SETTING_INDEX next = GetNextSetting(setting);
                    if (next == setting) {
                        // Send connect
                        gsmHandler->ForceCommand("AT+UPSDA=0,3");
                    } else {
                        SendSetting(next);
                    }
                } else if (type > MODEM_RESPONSE_OK) {
                    // ERROR
                    if (apnHandler != NULL) {
                        apnHandler->OnApnActivated(false);
                    }
                }
            }
            return true;
        }
        else if (strncmp(request, GSM_APN_CONTROL_CMD, strlen(GSM_APN_CONTROL_CMD)) == 0) {
            // TODO success/unsuccess send activate
        }
    }
    return false;
}
bool GSMApn::OnGSMEvent(char * data)
{
    if (strncmp(data, GSM_APN_ACTIVATED_EVENT, strlen(GSM_APN_ACTIVATED_EVENT)) == 0) {
        char *uupsda = data + 1;
        char *uupsdaArgs[2];
        SplitString(uupsda, ',', uupsdaArgs, 2, false);
        GetAddr(uupsdaArgs[1], &deviceAddr);
        apnState = GSM_APN_ACTIVE;
        if (apnHandler != NULL) {
            apnHandler->OnApnActivated(true);
        }

    } else if (strncmp(data, GSM_APN_DEACTIVATED_EVENT, strlen(GSM_APN_DEACTIVATED_EVENT)) == 0) {
        apnState = GSM_APN_DEACTIVATED;
        if (apnHandler != NULL) {
            apnHandler->OnApnDeactivated(true);
        }
    }

    return false;
}

UPSD_SETTING_INDEX GSMApn::GetNextSetting(int index)
{
    index++;
    if (index < UPSD_SETTING_APN) {
        index = UPSD_SETTING_APN;
    } else if (index > UPSD_SETTING_PSWD) {
        index = UPSD_SETTING_ASSIGNED_IP;
    }
    return (UPSD_SETTING_INDEX)index;
}

void GSMApn::SendSetting(UPSD_SETTING_INDEX setting)
{
    const size_t cmdSize = 128;
    char cmd[cmdSize];

    switch (setting)
    {
    case UPSD_SETTING_APN:
        snprintf(cmd, cmdSize, "%s%s=0,1,\"%u.%u.%u.%u\"", GSM_PREFIX_CMD, GSM_APN_CONFIG_CMD, apnAddr.octet[0], apnAddr.octet[1], apnAddr.octet[2], apnAddr.octet[3]);
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

bool GSMApn::Deactivate()
{
    if (apnState == GSM_APN_ACTIVE) {
        gsmHandler->ForceCommand("AT+UPSDA=0,4");
        return true;
    }
    return false;
}

bool GSMApn::IsActive()
{
    return apnState == GSM_APN_ACTIVE;
}


IPAddr GSMApn::GetDeviceAddr()
{
    return IPAddr(deviceAddr);
}