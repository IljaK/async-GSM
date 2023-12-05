#pragma once
#include "GSMNetworkManager.h"

//constexpr char QUECTEL_TEMP_THRESHOLD_CMD[] = "+CMTE"; // AT+CMTE=1 // PIN required: No

// This command is used to read the temperature of the module.
// Usage: AT+QTEMP - no question mark and values!
constexpr char QUECTEL_TEMP_CMD[] = "+QTEMP";

// AT+QNWINFO Query Network Information
// Usage: AT+QTEMP - no question mark and values!
constexpr char QUECTEL_NETWORK_TYPE_STATUS[] = "+QNWINFO"; 

// set AT+CVHU=0. Otherwise, ATH command will be ignored and "OK" response is given only
constexpr char GSM_QUECTEL_ATH_FIX_CMD[] = "+CVHU";

// AT^DSCI Call Status Indication
//constexpr char GSM_QUECTEL_CALL_INDICATE_CMD[] = "^DSCI";

// URC Indication:
constexpr char GSM_QUECTEL_URC_ENABLE_CMD[] = "+QINDCFG";

// AT+QURCCFG Configure URC Indication Option
constexpr char GSM_QUECTEL_URC_PORT_CMD[] = "+QURCCFG";

// TODO: DTMF


enum GSM_QUECTEL_CONFIGURATION_STEP {
    GSM_QUECTEL_CONFIGURATION_STEP_NONE = 0,

    GSM_QUECTEL_CONFIGURATION_STEP_CVHU,
    //GSM_QUECTEL_CONFIGURATION_STEP_DSCI,
    GSM_QUECTEL_CONFIGURATION_STEP_QURCCFG,
    GSM_QUECTEL_CONFIGURATION_STEP_QINDCFG_CSQ,
    GSM_QUECTEL_CONFIGURATION_STEP_QINDCFG_SMS,
    GSM_QUECTEL_CONFIGURATION_STEP_QINDCFG_ACT,
    GSM_QUECTEL_CONFIGURATION_STEP_QINDCFG_CCINFO,
    GSM_QUECTEL_CONFIGURATION_STEP_QINDCFG_ALL,

    GSM_QUECTEL_CONFIGURATION_STEP_COMPLETE
};

class QuectelGSMNetworkManager: public GSMNetworkManager
{
private:
    Timer delayedInitTimer;
    uint8_t simcomConfigurationStep = GSM_QUECTEL_CONFIGURATION_STEP_NONE;
    void NextQuectelConfigurationStep();

protected:
    void NextConfigurationStep() override;
    void FetchModemStats() override;
    void ContinueConfigureModem() override;

public:
    QuectelGSMNetworkManager(GSMModemManager *gsmManager);
    virtual ~QuectelGSMNetworkManager();

    bool OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

	void OnTimerComplete(Timer *timer) override;
    void OnModemReboot() override;

    void UpdateNetworkType(char *networkType);
};