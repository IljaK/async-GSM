#include "GPRSHandler.h"
#include "command/ULong2ModemCMD.h"
#include "command/IPResolveModemCMD.h"

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
        gsmHandler->ForceCommand(new ULong2ModemCMD(0,8,GSM_APN_FETCH_DATA_CMD));
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

bool GPRSHandler::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strcmp(request->cmd, GSM_GPRS_CMD) == 0) {
        ByteModemCMD *cgatt = (ByteModemCMD *)request;
        if (cgatt->byteData == 0) {
            if (type >= MODEM_RESPONSE_OK) {
                // No need to deactivate Profile
                apnState = GSM_APN_DEACTIVATED;
                if (apnHandler != NULL) {
                    apnHandler->OnGPRSDeactivated(type == MODEM_RESPONSE_OK);
                }
            }
        }
        return true;
    }
    else if (strcmp(request->cmd, GSM_APN_CONTROL_CMD) == 0) {
        ULong2ModemCMD *upsda = (ULong2ModemCMD *)request;
        // "AT+UPSDA=0,3"
        if (upsda->valueData2 == 3) { // Connect command
            if (type == MODEM_RESPONSE_OK) {
                Timer::Stop(checkTimer);
                checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
            } else if (type > MODEM_RESPONSE_OK) {
                apnState = GSM_APN_DEACTIVATING;
                gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
            }
        }
        // No check for deactivation AT+CGATT=0 will handle that
        return true;
    }
    else if (strcmp(request->cmd, GSM_APN_FETCH_DATA_CMD) == 0) {
        ULong2ModemCMD *upsnd = (ULong2ModemCMD *)request;
        // "AT+UPSND=0,0"
        if (type == MODEM_RESPONSE_DATA) {
            if (strlen(response) <= strlen(GSM_APN_FETCH_DATA_CMD) + 2) {
                return true;
            }
            response += strlen(GSM_APN_FETCH_DATA_CMD) + 2;
            char *upsndRespArgs[3];
            SplitString(response, ',', upsndRespArgs, 3, false);

            if (upsnd->valueData2 == UPSDN_SETTING_STATUS) { // Connection
                if (strncmp(upsndRespArgs[2], "1", 1) == 0) {
                    // Fetch status
                    gsmHandler->ForceCommand(new ULong2ModemCMD(0,0,GSM_APN_FETCH_DATA_CMD));
                } else {
                    Timer::Stop(checkTimer);
                    checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
                }
            } else if (upsnd->valueData2 == UPSDN_SETTING_IP) { // ADDR
                GetAddr(ShiftQuotations(upsndRespArgs[2]), &deviceAddr);
            }
        }
        else if (type > MODEM_RESPONSE_OK) {
            if (upsnd->valueData2 == UPSDN_SETTING_STATUS) { // Connection
                Timer::Stop(checkTimer);
                checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
            } else if (upsnd->valueData2 == UPSDN_SETTING_IP) {
                // TODO: error on ip fetch?
                gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
            }
        }
        else if (type == MODEM_RESPONSE_OK) {
            if (upsnd->valueData2 == UPSDN_SETTING_IP) {
                apnState = GSM_APN_ACTIVE;
                if (apnHandler != NULL) {
                    apnHandler->OnGPRSActivated(type == MODEM_RESPONSE_OK);
                }
            }
        }
        return true;
    }
    else if (strcmp(request->cmd, GSM_APN_CONFIG_CMD) == 0) {
        ULong2StringModemCMD *upsd = (ULong2StringModemCMD *)request;
        if (type == MODEM_RESPONSE_OK) {
            UPSD_SETTING_INDEX next = GetNextSetting(upsd->valueData2);
            if (next == upsd->valueData2) {
                // Enable APN, GATT will enable itself
                gsmHandler->ForceCommand(new ULong2ModemCMD(0,3, GSM_APN_CONTROL_CMD, APN_CONNECT_CMD_TIMEOUT));
            } else {
                SendSetting(next);
            }
        } else if (type > MODEM_RESPONSE_OK) {
            // ERROR
            apnState = GSM_APN_DEACTIVATING;
            gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
        }
        return true;
    }
    else if (strcmp(request->cmd, GSM_RESOLVE_DNS_CMD) == 0) {
        IPResolveModemCMD *udnsrn = (IPResolveModemCMD *)request;
        if (type == MODEM_RESPONSE_DATA) {
            char *ipString = response + strlen(GSM_RESOLVE_DNS_CMD) + 2;
            GetAddr(ShiftQuotations(ipString), &udnsrn->ipAddr);
        } else if (type == MODEM_RESPONSE_OK) {
            if (apnHandler != NULL) {
                apnHandler->OnHostNameResolve(udnsrn->charData, udnsrn->ipAddr, udnsrn->ipAddr.dword != 0);
            }
        } else {
            if (apnHandler != NULL) {
                apnHandler->OnHostNameResolve(udnsrn->charData, udnsrn->ipAddr, false);
            }
        }
    }
    return false;
}
bool GPRSHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (IsEvent(GSM_APN_DEACTIVATED_EVENT, data, dataLen)) {
        apnState = GSM_APN_DEACTIVATING;
        gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
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
    switch (setting)
    {
    case UPSD_SETTING_APN:
        gsmHandler->ForceCommand(new ULong2StringModemCMD(0, 1, apn, GSM_APN_CONFIG_CMD));
        break;
    case UPSD_SETTING_LOGIN:
        gsmHandler->ForceCommand(new ULong2StringModemCMD(0, 2, login, GSM_APN_CONFIG_CMD));
        break;
    case UPSD_SETTING_PSWD:
        gsmHandler->ForceCommand(new ULong2StringModemCMD(0, 3, password, GSM_APN_CONFIG_CMD));
        break;
    default:
        gsmHandler->ForceCommand(new ULong2StringModemCMD(0, 7, (char *)"0.0.0.0", GSM_APN_CONFIG_CMD));
        break;
    }
}

bool GPRSHandler::Deactivate()
{
    if (apnState == GSM_APN_ACTIVE) {
        apnState = GSM_APN_DEACTIVATING;
        // No need to trigger profile OFF command
        gsmHandler->ForceCommand(new ByteModemCMD(0, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
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
    return deviceAddr;
}
void GPRSHandler::OnModemReboot()
{
    FlushAuthData();
    apnState = GSM_APN_DEACTIVATED;
}


bool GPRSHandler::ResolveHostName(const char *hostName)
{
    if (!IsActive()) {
        return false;
    }
    // ByteCharModemCMD
    return gsmHandler->AddCommand(new IPResolveModemCMD(0, hostName, GSM_RESOLVE_DNS_CMD, APN_CONNECT_CMD_TIMEOUT));
}