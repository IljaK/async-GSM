#pragma once
#include "BaseGSMHandler.h"
#include "GSMNetworkHandler.h"
#include "GSMCallHandler.h"
#include "socket/GSMSocketHandler.h"

class IGSMHandler: public IModemBootHandler, public IBaseGSMHandler
{

};

class GSMHandler: public BaseGSMHandler
{
private:
    IGSMHandler *handler = NULL;
protected:
    GSMSocketHandler socketHandler;
    GSMNetworkHandler gsmHandler;

public:
    GSMHandler();
    virtual ~GSMHandler();

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

    void SetHandler(IGSMHandler *handler);

    void OnModemBooted() override;
    void OnModemFailedBoot() override;
};