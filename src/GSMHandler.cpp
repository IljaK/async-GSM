#include "GSMHandler.h"



GSMHandler::GSMHandler():
    BaseGSMHandler(),
    gsmApn(this),
    gsmHandler(this),
    gprsHandler(this),
    socketHandler(this)
{

}

GSMHandler::~GSMHandler()
{
    
}

bool GSMHandler::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    if (gsmHandler.OnGSMResponse(request, response, type)) {
        return true;
    }
    if (socketHandler.OnGSMResponse(request, response, type)) {
        return true;
    }
    return false;
}

bool GSMHandler::OnGSMEvent(char * data)
{
    if (gsmHandler.OnGSMEvent(data)) {
        return true;
    }
    if (socketHandler.OnGSMEvent(data)) {
        return true;
    }
    return false;
}