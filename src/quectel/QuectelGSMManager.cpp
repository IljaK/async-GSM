#include "QuectelGSMManager.h"

QuectelGSMManager::QuectelGSMManager(HardwareSerial *serial, int8_t resetPin):QuectelGSMModemManager(serial, resetPin),
    gprsManager(this),
    gsmManager(this),
    callManager(this),
    socketManager(this)
{

}

QuectelGSMManager::~QuectelGSMManager()
{
    
}

void QuectelGSMManager::ReBootModem(bool hardReset)
{
    gsmManager.OnModemReboot();
    callManager.OnModemReboot();
    gprsManager.OnModemReboot();
    socketManager.OnModemReboot();
    QuectelGSMModemManager::ReBootModem(hardReset);
}

bool QuectelGSMManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (gsmManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (socketManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (callManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (gprsManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    return false;
}

bool QuectelGSMManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (gsmManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (socketManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (callManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (gprsManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    return false;
}


bool QuectelGSMManager::OnGSMExpectedData(uint8_t * data, size_t dataLen)
{
    if (socketManager.OnGSMExpectedData(data, dataLen)) {
        return true;
    }
    return false;
}