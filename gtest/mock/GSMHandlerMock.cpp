#include "GSMHandlerMock.h"

GSMHandlerMock::GSMHandlerMock():UbloxGSMManager()
{
    socketManager.SetSocketHandler(this);
}

GSMHandlerMock::~GSMHandlerMock()
{
    if (lastResponse != NULL) {
        free(lastResponse);
    }
    if (lastEvent != NULL) {
        free(lastEvent);
    }
}

void GSMHandlerMock::OnModemBooted()
{
    //GSMHandler::OnModemBooted();
}

void GSMHandlerMock::OnModemFailedBoot()
{
    //UbloxGSMManager::OnModemFailedBoot();
}

void GSMHandlerMock::OnModemReboot()
{
    UbloxGSMManager::OnModemReboot();
}

bool GSMHandlerMock::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (lastResponse != NULL) {
        free(lastResponse);
    }
    lastResponse = (char*)malloc(respLen + 1);
    strcpy(lastResponse, response);

    return UbloxGSMManager::OnGSMResponse(request, response, respLen, type);
}

bool GSMHandlerMock::OnGSMEvent(char * data, size_t dataLen)
{
    if (lastEvent != NULL) {
        free(lastEvent);
    }
    lastEvent = (char*)malloc(dataLen + 1);
    strcpy(lastEvent, data);
    return UbloxGSMManager::OnGSMEvent(data, dataLen);
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
    Flush();
}

UbloxGPRSManager *GSMHandlerMock::GetGprsHandler()
{
    return &gprsHandler;
}
GSMNetworkHandler *GSMHandlerMock::GetGsmHandler()
{
    return &gsmHandler;
}
GSMSocketManager * GSMHandlerMock::GetSocketHandler()
{
    return &socketManager;
}

BaseModemCMD *GSMHandlerMock::GetPendingCMD()
{
    return pendingCMD;
}


char *GSMHandlerMock::GetLastEvent()
{
    return lastEvent;
}
char *GSMHandlerMock::GetLastResponse()
{
    return lastResponse;
}