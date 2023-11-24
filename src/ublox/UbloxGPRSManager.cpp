#include "UbloxGPRSManager.h"
#include "command/ULong2ModemCMD.h"
#include "command/IPResolveExtraModemCMD.h"

UbloxGPRSManager::UbloxGPRSManager(GSMModemManager *gsmManager):GPRSManager(gsmManager)
{

}
UbloxGPRSManager::~UbloxGPRSManager()
{

}

bool UbloxGPRSManager::ConnectInternal()
{
    if (GetApnState() == GSM_APN_DEACTIVATED) {
        HandleAPNUpdate(GSM_APN_ACTIVATING);
        SendSetting(UPSD_SETTING_APN);
        return true;
    }
    return false;
}

bool UbloxGPRSManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{

    if (strcmp(request->cmd, GSM_APN_CONTROL_CMD) == 0) {
        ULong2ModemCMD *upsda = (ULong2ModemCMD *)request;
        // "AT+UPSDA=0,3"
        if (upsda->valueData2 == 3) { // Connect command
            if (type == MODEM_RESPONSE_OK) {
                Timer::Stop(checkTimer);
                checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
            } else if (type >= MODEM_RESPONSE_ERROR) {
                Deactivate();
            }
        }
        // No check for deactivation AT+CGATT=0 will handle that
        return true;
    }
    if (strcmp(request->cmd, GSM_APN_FETCH_DATA_CMD) == 0) {
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
                    gsmManager->ForceCommand(new ULong2ModemCMD(0,0,GSM_APN_FETCH_DATA_CMD));
                } else {
                    Timer::Stop(checkTimer);
                    checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
                }
            } else if (upsnd->valueData2 == UPSDN_SETTING_IP) { // ADDR
                GetAddr(ShiftQuotations(upsndRespArgs[2]), &deviceAddr);
            }
        }
        else if (type >= MODEM_RESPONSE_ERROR) {
            if (upsnd->valueData2 == UPSDN_SETTING_STATUS) { // Connection
                Timer::Stop(checkTimer);
                checkTimer = Timer::Start(this, APN_STATUS_CHECK_DELAY);
            } else if (upsnd->valueData2 == UPSDN_SETTING_IP) {
                // TODO: error on ip fetch?
                Deactivate();
            }
        }
        else if (type == MODEM_RESPONSE_OK) {
            if (upsnd->valueData2 == UPSDN_SETTING_IP) {
                HandleAPNUpdate(GSM_APN_ACTIVE);
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_APN_CONFIG_CMD) == 0) {
        ULong2StringModemCMD *upsd = (ULong2StringModemCMD *)request;
        if (type == MODEM_RESPONSE_OK) {
            UPSD_SETTING_INDEX next = GetNextSetting(upsd->valueData2);
            if (next == upsd->valueData2) {
                // Enable APN, GATT will enable itself
                gsmManager->ForceCommand(new ULong2ModemCMD(0,3, GSM_APN_CONTROL_CMD, APN_CONNECT_CMD_TIMEOUT));
            } else {
                SendSetting(next);
            }
        } else if (type >= MODEM_RESPONSE_ERROR) {
            // ERROR
            Deactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_UBLOX_RESOLVE_DNS_CMD) == 0) {
        IPResolveExtraModemCMD *ipResolve = (IPResolveExtraModemCMD *)request;
        if (type == MODEM_RESPONSE_DATA) {
            char *ipString = response + strlen(GSM_UBLOX_RESOLVE_DNS_CMD) + 2;
            GetAddr(ShiftQuotations(ipString), &ipResolve->ipAddr);
        } else {
            HandleHostNameResolve(ipResolve->charData, ipResolve->ipAddr);
        }
        return true;
    }
    return GPRSManager::OnGSMResponse(request, response, respLen, type);
}
bool UbloxGPRSManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (IsEvent(GSM_APN_DEACTIVATED_EVENT, data, dataLen)) {
        // AT+CGATT will be automatically set to 0
        HandleAPNUpdate(GSM_APN_DEACTIVATED);
        return true;
    }

    return GPRSManager::OnGSMEvent(data, dataLen);
}

UPSD_SETTING_INDEX UbloxGPRSManager::GetNextSetting(int index)
{
    index++;
    if (index < UPSD_SETTING_APN) {
        index = UPSD_SETTING_APN;
    } else if (index > UPSD_SETTING_PSWD) {
        index = UPSD_SETTING_ASSIGNED_IP;
    }
    return (UPSD_SETTING_INDEX)index;
}

void UbloxGPRSManager::SendSetting(UPSD_SETTING_INDEX setting)
{
    switch (setting)
    {
    case UPSD_SETTING_APN:
        gsmManager->ForceCommand(new ULong2StringModemCMD(0, 1, apn, GSM_APN_CONFIG_CMD));
        break;
    case UPSD_SETTING_LOGIN:
        gsmManager->ForceCommand(new ULong2StringModemCMD(0, 2, login, GSM_APN_CONFIG_CMD));
        break;
    case UPSD_SETTING_PSWD:
        gsmManager->ForceCommand(new ULong2StringModemCMD(0, 3, password, GSM_APN_CONFIG_CMD));
        break;
    default:
        gsmManager->ForceCommand(new ULong2StringModemCMD(0, 7, (char *)"0.0.0.0", GSM_APN_CONFIG_CMD));
        break;
    }
}

bool UbloxGPRSManager::ResolveHostNameCMD(const char *hostName)
{
    return gsmManager->AddCommand(new IPResolveExtraModemCMD(0, hostName, GSM_UBLOX_RESOLVE_DNS_CMD, APN_CONNECT_CMD_TIMEOUT));
}

void UbloxGPRSManager::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == checkTimer) {
        checkTimer = 0;
        gsmManager->ForceCommand(new ULong2ModemCMD(0,8, GSM_APN_FETCH_DATA_CMD));
    }
}
void UbloxGPRSManager::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == checkTimer) {
        checkTimer = 0;
    }
}

void UbloxGPRSManager::FlushAuthData()
{
    GPRSManager::FlushAuthData();
    Timer::Stop(checkTimer);
}