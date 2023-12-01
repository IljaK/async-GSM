#pragma once
#include "GSMModemManager.h"
#include "GSMCallManager.h"

class QuectelGSMCallManager: public GSMCallManager
{
private:

public:
    bool OnGSMEvent(char * data, size_t dataLen) override;
    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;

    QuectelGSMCallManager(GSMModemManager *gsmManager);
    virtual ~QuectelGSMCallManager();
};