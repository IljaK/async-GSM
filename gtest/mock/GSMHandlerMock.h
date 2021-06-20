#include <Arduino.h>
#include "GSMHandler.h"
#include "socket/GSMSocketHandler.h"

class GSMHandlerMock: public GSMHandler, public IGSMSocketHandler
{
private:
    /* data */
public:
    GSMHandlerMock(/* args */);
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

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

    void ReadResponse(char * response);
    void SetReady();

    GPRSHandler *GetGprsHandler();
    GSMNetworkHandler *GetGsmHandler();
    GSMSocketHandler * GetSocketHandler();
};
