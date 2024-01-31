#include "GPRSManager.h"
#include "command/ULong2ModemCMD.h"
#include "command/IPResolveModemCMD.h"

GPRSManager::GPRSManager(GSMModemManager *gsmManager):coolDownTimer(this)
{
    deviceAddr.dword = 0;
    this->gsmManager = gsmManager;
}
GPRSManager::~GPRSManager()
{
    FlushAuthData();
}


void GPRSManager::SetAPNHandler(IGPRSHandler *apnHandler)
{
    this->apnHandler = apnHandler;
}


void GPRSManager::FlushAuthData()
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
}

bool GPRSManager::Connect(char* apn, char* login, char* password)
{
    if (apnState != GSM_APN_DEACTIVATED) {
        return false;
    }
    FlushAuthData();
    
    this->apn = makeNewCopy(apn);
    this->login = makeNewCopy(login);
    this->password = makeNewCopy(password);

    // Steps:
    // 1) Check CGATT
    // 2) Activate CGATT (If necessary)
    // 3) ConnectInternal();

    bool started = SendCGACT(true, true, MODEM_COMMAND_TIMEOUT);
    if (started) {
        HandleAPNUpdate(GSM_APN_ACTIVATING);
    }
    return started;
}

bool GPRSManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strcmp(request->cmd, GSM_GPRS_CMD) == 0) {
        if (request->GetIsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                // +CGATT: 1
                char *cgattContent = response + strlen(GSM_GPRS_CMD) + 2;
                OnCGACT(true, atoi(cgattContent) == 1, true);
            } else if (type == MODEM_RESPONSE_OK) {
                // Do nothing?
            } else if (type >= MODEM_RESPONSE_ERROR) {
                OnCGACT(true, 0, false);
            }
            return true;
        }

        ByteModemCMD *cgatt = (ByteModemCMD *)request;
        OnCGACT(false, cgatt->byteData == 1, type == MODEM_RESPONSE_OK);
        return true;
    }
    return false;
}

bool GPRSManager::SendCGACT(bool isCheck, uint8_t value, unsigned long timeout)
{
    if (isCheck) {
        return gsmManager->ForceCommand(new BaseModemCMD(GSM_GPRS_CMD, MODEM_COMMAND_TIMEOUT, true));
    }
    if (coolDownTimer.IsRunning()) {
        // Delayed send
        return true;
    }
    return gsmManager->ForceCommand(new ByteModemCMD(value, GSM_GPRS_CMD, APN_CONNECT_CMD_TIMEOUT));
}

void GPRSManager::OnCGACT(bool isCheck, bool value, bool isSuccess)
{
    if (isCheck) {
        // Check is +CGATT active before further GPRS activation
        if (isSuccess) {
            if (value) {
                // Continue PDP activation
                ConnectInternal();
            } else {
                // Activate +CGATT
                SendCGACT(false, true);
            }
            return;
        }
        ForceDeactivate();
        return;
    }
    if (value) {
        if (isSuccess) {
            // Continue PDP activation
            ConnectInternal();
            //HandleAPNUpdate(GSM_APN_ACTIVE);
        } else {
            //HandleAPNUpdate(GSM_APN_DEACTIVATED);
            ForceDeactivate();
        }
        return;
    }
    HandleAPNUpdate(GSM_APN_DEACTIVATED);
}

bool GPRSManager::OnGSMEvent(char * data, size_t dataLen)
{
    return false;
}

bool GPRSManager::ForceDeactivate()
{
    if (apnState != GSM_APN_DEACTIVATED) {
        apnState = GSM_APN_DEACTIVATING;
    }
    deviceAddr.dword = 0;
    SendCGACT(false, 0);
    return true;
}

bool GPRSManager::Deactivate()
{
    if (apnState == GSM_APN_ACTIVE) {
        ForceDeactivate();
        return true;
    }
    return false;
}

bool GPRSManager::IsActive()
{
    return apnState == GSM_APN_ACTIVE;
}

IPAddr GPRSManager::GetDeviceAddr()
{
    return deviceAddr;
}
void GPRSManager::OnModemReboot()
{
    FlushAuthData();
    apnState = GSM_APN_DEACTIVATED;
}

bool GPRSManager::ResolveHostName(const char *hostName)
{
    if (!IsActive()) {
        return false;
    }
    // ByteCharModemCMD
    return ResolveHostNameCMD(hostName);
}

void GPRSManager::HandleHostNameResolve(const char *hostName, IPAddr ip)
{
    if (apnHandler != NULL) {
        apnHandler->OnHostNameResolve(hostName, ip, ip.dword != 0);
    }
}

void GPRSManager::HandleAPNUpdate(GSM_APN_STATE apnState)
{
    if (this->apnState == apnState) {
        return;
    }

    this->apnState = apnState;
    if (apnHandler != NULL) {
        switch (apnState)
        {
        case GSM_APN_DEACTIVATED:
            deviceAddr.dword = 0;
            coolDownTimer.StartMicros(GPRS_COOLDOWN_DELAY);
            apnHandler->OnGPRSDeactivated();
            break;
        case GSM_APN_ACTIVATING:
            break;
        case GSM_APN_ACTIVE:
            apnHandler->OnGPRSActivated();
            break;
        case GSM_APN_DEACTIVATING:
            break;
        }
    }
}

GSM_APN_STATE GPRSManager::GetApnState()
{
    return apnState;
}

void GPRSManager::OnTimerComplete(Timer *timer)
{
    if (timer == &coolDownTimer) {
        if (GetApnState() == GSM_APN_STATE::GSM_APN_ACTIVATING) {
            SendCGACT(false, true);
        }
        return;
    }
}