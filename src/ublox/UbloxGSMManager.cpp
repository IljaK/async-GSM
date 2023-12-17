#include "UbloxGSMManager.h"

UbloxGSMManager::UbloxGSMManager(HardwareSerial *serial, int8_t resetPin):UbloxGSMModemManager(serial, resetPin),
    socketManager(this, &gprsManager),
    gprsManager(this),
    callManager(this),
    gsmManager(this)
{

}

UbloxGSMManager::~UbloxGSMManager()
{
    
}

void UbloxGSMManager::ReBootModem(bool hardReset)
{
    gsmManager.OnModemReboot();
    callManager.OnModemReboot();
    gprsManager.OnModemReboot();
    socketManager.OnModemReboot();
    UbloxGSMModemManager::ReBootModem(hardReset);
}

bool UbloxGSMManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
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

bool UbloxGSMManager::OnGSMEvent(char * data, size_t dataLen)
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