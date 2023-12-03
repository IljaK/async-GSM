#pragma once
#include <Arduino.h>
#include "ublox/UbloxGSMManager.h"
#include "ublox/UbloxGSMSocketManager.h"
#include "GSMExpectation.h"

typedef void (*gsmEventCb)(char * data, size_t dataLen);
typedef void (*gsmResponseCb)(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type);

class UbloxGSMHandlerMock: public UbloxGSMManager, public IGSMSocketManager
{
private:
    char *lastResponse = NULL;
    char *lastEvent = NULL;
    /* data */
public:
    UbloxGSMHandlerMock();
    virtual ~UbloxGSMHandlerMock();

    void OnModemBooted() override;
    void OnModemFailedBoot() override;
    void OnModemStartReboot() override;

    void OnSocketConnected(GSMSocket * socket) override;
    void OnSocketClose(GSMSocket * socket) override;

    void OnSocketData(GSMSocket * socket, uint8_t *data, size_t len) override;
    void OnSocketStartListen(GSMSocket * socket) override;

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    void ReadResponse(char * response);
    void SetReady();

    UbloxGPRSManager *GetGprsHandler();
    UbloxGSMNetworkManager *GetGsmHandler();
    UbloxGSMSocketManager * GetSocketHandler();

    BaseModemCMD *GetPendingCMD();

    char *GetLastEvent();
    char *GetLastResponse();
};
