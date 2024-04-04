#pragma once
#include "SimComGSMNetworkManager.h"
#include "SimComGSMModemManager.h"
#include "SimComGSMCallManager.h"
#include "SimComGPRSManager.h"
#include "SimComGSMCallManager.h"
#include "SimComGSMSocketManager.h"

class SimComGSMManager: public SimComGSMModemManager
{
protected:
    SimComGSMSocketManager socketManager;
    SimComGPRSManager gprsManager;
    SimComGSMCallManager callManager;
    SimComGSMNetworkManager gsmManager;
    void ReBootModem(bool hardReset = true) override;

public:
    SimComGSMManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~SimComGSMManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
    bool OnGSMExpectedData(uint8_t * data, size_t dataLen) override;

    //void OnModemBooted() override;
    //void OnModemFailedBoot() override;
};