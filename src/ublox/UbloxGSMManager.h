#pragma once
#include "UbloxGSMNetworkManager.h"
#include "UbloxGSMModemManager.h"
#include "UbloxGPRSManager.h"
#include "UbloxGSMCallManager.h"
#include "UbloxGSMSocketManager.h"

class UbloxGSMManager: public UbloxGSMModemManager
{
protected:
    UbloxGPRSManager gprsManager;
    UbloxGSMNetworkManager gsmManager;
    UbloxGSMCallManager callManager;
    UbloxGSMSocketManager socketManager;
    void ReBootModem(bool hardReset = true) override;

public:
    UbloxGSMManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~UbloxGSMManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    //void OnModemBooted() override;
    //void OnModemFailedBoot() override;
};