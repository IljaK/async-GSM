#include "GSMHandler.h"

GSMHandler::GSMHandler():
    BaseGSMHandler(),
    socketHandler(this),
    gsmHandler(this)
{

}

GSMHandler::~GSMHandler()
{
    
}

void GSMHandler::OnModemBooted()
{
    if (handler != NULL) {
        handler->OnModemBooted();
    }
}
void GSMHandler::OnModemFailedBoot()
{
    if (handler != NULL) {
        handler->OnModemFailedBoot();
    }
}

bool GSMHandler::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    if (gsmHandler.OnGSMResponse(request, response, type)) {
        return true;
    }
    if (socketHandler.OnGSMResponse(request, response, type)) {
        return true;
    }
    if (handler != NULL && handler->OnGSMResponse(request, response, type)) {
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
    if (handler != NULL && handler->OnGSMEvent(data)) {
        return true;
    }
    return false;
}

void GSMHandler::SetHandler(IGSMHandler *handler)
{
    this->handler = handler;
}