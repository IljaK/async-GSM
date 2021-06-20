#include "GSMHandlerMock.h"

GSMHandlerMock::GSMHandlerMock():GSMHandler()
{
    socketHandler.SetSocketHandler(this);
}

GSMHandlerMock::~GSMHandlerMock()
{
}

void GSMHandlerMock::OnModemBooted()
{
    //GSMHandler::OnModemBooted();
}

void GSMHandlerMock::OnModemFailedBoot()
{
    //GSMHandler::OnModemFailedBoot();
}

void GSMHandlerMock::OnModemReboot()
{
    GSMHandler::OnModemReboot();
}

bool GSMHandlerMock::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    return GSMHandler::OnGSMResponse(request, response, type);
}

bool GSMHandlerMock::OnGSMEvent(char * data)
{
    return GSMHandler::OnGSMEvent(data);
}

void GSMHandlerMock::OnSocketCreateError()
{

}
void GSMHandlerMock::OnSocketOpen(GSMSocket * socket)
{
    
}
void GSMHandlerMock::OnSocketClose(uint8_t sockedId, bool isSuccess)
{
    
}
void GSMHandlerMock::OnSocketConnected(GSMSocket * socket, int error)
{
    
}
void GSMHandlerMock::OnSocketData(GSMSocket * socket, uint8_t *data, size_t len)
{
    
}
void GSMHandlerMock::OnSocketStartListen(GSMSocket * socket)
{
    
}

void GSMHandlerMock::ReadResponse(char * response)
{
	Timer::Loop();
	this->Loop();

	serial->AddRXBuffer(response);

	while (serial->available() > 0) {
		Timer::Loop();
		this->Loop();
	}
}

void GSMHandlerMock::SetReady()
{
    modemBootState = MODEM_BOOT_COMPLETED;
}

GPRSHandler *GSMHandlerMock::GetGprsHandler()
{
    return &gprsHandler;
}
GSMNetworkHandler *GSMHandlerMock::GetGsmHandler()
{
    return &gsmHandler;
}
GSMSocketHandler * GSMHandlerMock::GetSocketHandler()
{
    return &socketHandler;
}