#pragma once
#include <Arduino.h>
#include "UbloxGSMManager.h"
#include "socket/GSMSocketManager.h"
#include "GSMExpectation.h"

typedef void (*gsmEventCb)(char * data, size_t dataLen);
typedef void (*gsmResponseCb)(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type);

class GSMHandlerMock: public UbloxGSMManager, public IGSMSocketManager
{
private:
    char *lastResponse = NULL;
    char *lastEvent = NULL;
    /* data */
public:
    GSMHandlerMock();
    virtual ~GSMHandlerMock();

    void OnModemBooted() override;
    void OnModemFailedBoot() override;
    void OnModemReboot() override;

    void OnSocketCreateError() override;
    void OnSocketOpen(GSMSocket * socket) override;
    void OnSocketClose(uint8_t sockedId, bool isSuccess) override;

    void OnSocketConnected(GSMSocket * socket, int error) override;
    void OnSocketData(GSMSocket * socket, uint8_t *data, size_t len) override;
    void OnSocketStartListen(GSMSocket * socket) override;

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    void ReadResponse(char * response);
    void SetReady();

    UbloxGPRSManager *GetGprsHandler();
    GSMNetworkHandler *GetGsmHandler();
    GSMSocketManager * GetSocketHandler();

    BaseModemCMD *GetPendingCMD();

    char *GetLastEvent();
    char *GetLastResponse();
};
