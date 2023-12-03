#include "UbloxGSMHandlerMock.h"

UbloxGSMHandlerMock::UbloxGSMHandlerMock():UbloxGSMManager(new HardwareSerial(), -1)
{
    socketManager.SetSocketHandler(this);
}

UbloxGSMHandlerMock::~UbloxGSMHandlerMock()
{
    if (lastResponse != NULL) {
        free(lastResponse);
    }
    if (lastEvent != NULL) {
        free(lastEvent);
    }
}

void UbloxGSMHandlerMock::OnModemBooted()
{
    //GSMHandler::OnModemBooted();
}

void UbloxGSMHandlerMock::OnModemFailedBoot()
{
    //UbloxGSMManager::OnModemFailedBoot();
}

bool UbloxGSMHandlerMock::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (lastResponse != NULL) {
        free(lastResponse);
    }
    lastResponse = (char*)malloc(respLen + 1);
    strcpy(lastResponse, response);

    return UbloxGSMManager::OnGSMResponse(request, response, respLen, type);
}

bool UbloxGSMHandlerMock::OnGSMEvent(char * data, size_t dataLen)
{
    if (lastEvent != NULL) {
        free(lastEvent);
    }
    lastEvent = (char*)malloc(dataLen + 1);
    strcpy(lastEvent, data);
    return UbloxGSMManager::OnGSMEvent(data, dataLen);
}

void UbloxGSMHandlerMock::OnSocketData(GSMSocket * socket, uint8_t *data, size_t len)
{
    
}
void UbloxGSMHandlerMock::OnSocketStartListen(GSMSocket * socket)
{
    
}

void UbloxGSMHandlerMock::ReadResponse(char * response)
{
	Timer::Loop();
	this->Loop();

	serial->AddRXBuffer(response);

	while (serial->available() > 0) {
		Timer::Loop();
		this->Loop();
	}
}

void UbloxGSMHandlerMock::SetReady()
{
    modemBootState = MODEM_BOOT_COMPLETED;
    Flush();
}

UbloxGPRSManager *UbloxGSMHandlerMock::GetGprsHandler()
{
    return &gprsManager;
}
UbloxGSMNetworkManager *UbloxGSMHandlerMock::GetGsmHandler()
{
    return &gsmManager;
}
UbloxGSMSocketManager * UbloxGSMHandlerMock::GetSocketHandler()
{
    return &socketManager;
}

BaseModemCMD *UbloxGSMHandlerMock::GetPendingCMD()
{
    return pendingCMD;
}


char *UbloxGSMHandlerMock::GetLastEvent()
{
    return lastEvent;
}
char *UbloxGSMHandlerMock::GetLastResponse()
{
    return lastResponse;
}


void UbloxGSMHandlerMock::OnModemStartReboot()
{
    
}

void UbloxGSMHandlerMock::OnSocketConnected(GSMSocket * socket)
{
    
}
void UbloxGSMHandlerMock::OnSocketClose(GSMSocket * socket)
{

}