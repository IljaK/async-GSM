#pragma once
#include "QuectelGSMNetworkManager.h"
#include "QuectelGSMModemManager.h"
#include "QuectelGSMCallManager.h"
#include "QuectelGPRSManager.h"
#include "QuectelGSMCallManager.h"
#include "QuectelGSMSocketManager.h"

// +QIURC: "pdpdeact",<contextID>
constexpr char GSM_QUECTEL_URC_EVENT[] = "+QIURC"; 

// AT+QURCCFG Configure URC Indication Option
constexpr char GSM_QUECTEL_URC_IND_EVENT[] = "+QIND";

class QuectelGSMManager: public QuectelGSMModemManager
{
protected:
    QuectelGPRSManager gprsManager;
    QuectelGSMNetworkManager gsmManager;
    QuectelGSMCallManager callManager;
    QuectelGSMSocketManager socketManager;
    void ReBootModem(bool hardReset = true) override;

public:
    QuectelGSMManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~QuectelGSMManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
    bool OnGSMExpectedData(uint8_t * data, size_t dataLen) override;

    //void OnModemBooted() override;
    //void OnModemFailedBoot() override;
};