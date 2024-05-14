#include "SimComGSMManager.h"

SimComGSMManager::SimComGSMManager(HardwareSerial *serial, int8_t resetPin, uint32_t targetBaudRate):SimComGSMModemManager(serial, resetPin, targetBaudRate),
    socketManager(this, &gprsManager),
    gprsManager(this),
    callManager(this),
    gsmManager(this)
{

}

SimComGSMManager::~SimComGSMManager()
{
    
}

void SimComGSMManager::ReBootModem(bool hardReset)
{
    gsmManager.OnModemReboot();
    callManager.OnModemReboot();
    gprsManager.OnModemReboot();
    socketManager.OnModemReboot();
    SimComGSMModemManager::ReBootModem(hardReset);
}

bool SimComGSMManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
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

bool SimComGSMManager::OnGSMEvent(char * data, size_t dataLen)
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


bool SimComGSMManager::OnGSMExpectedData(uint8_t * data, size_t dataLen)
{
    if (socketManager.OnGSMExpectedData(data, dataLen)) {
        return true;
    }
    return false;
}