#pragma once
#include "BaseGSMHandler.h"
#include "GSMNetworkHandler.h"
#include "GSMCallHandler.h"
#include "GPRSHandler.h"
#include "socket/GSMSocketHandler.h"

class GSMHandler: public BaseGSMHandler
{
protected:
    GPRSHandler gprsHandler;
    GSMNetworkHandler gsmHandler;
    GSMSocketHandler socketHandler;
public:
    GSMHandler();
    virtual ~GSMHandler();

    void OnModemReboot() override;

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    //void OnModemBooted() override;
    //void OnModemFailedBoot() override;
};