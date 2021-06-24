#include "GSMHandler.h"



GSMHandler::GSMHandler():
    BaseGSMHandler(),
    gprsHandler(this),
    gsmHandler(this),
    socketHandler(this)
{

}

GSMHandler::~GSMHandler()
{
    
}

void GSMHandler::OnModemReboot()
{
    gprsHandler.OnModemReboot();
    socketHandler.OnModemReboot();
    gsmHandler.OnModemReboot();
}

bool GSMHandler::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (gsmHandler.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (socketHandler.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (gprsHandler.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    return false;
}

bool GSMHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (gsmHandler.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (socketHandler.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (gprsHandler.OnGSMEvent(data, dataLen)) {
        return true;
    }
    return false;
}