#pragma once
#include "GSMNetworkManager.h"


// While module’s temperature over the high threshold and below the low threshold, the URC will occur (+CMTE)
// -2 below -45 celsius degree. 
// -1 (-45,-30] celsius degree. 
// 1 (80,85] celsius degree.
// 2 over 85 celsius degree.
constexpr char SIMCOM_TEMP_THRESHOLD_CMD[] = "+CMTE"; // AT+CMTE=1 // PIN required: No

// This command is used to read the temperature of the module.
// Usage: AT+CPMUTEMP - no question vark and values!
constexpr char SIMCOM_TEMP_CMD[] = "+CPMUTEMP"; // +CPMUTEMP: 15

// This command is used to return the current network system mode.
// 0 no service
// 1 GSM
// 2 GPRS
// 3 EGPRS (EDGE)
// 4 WCDMA
// 5 HSDPA only(WCDMA)
// 6 HSUPA only(WCDMA)
// 7 HSPA (HSDPA and HSUPA, WCDMA)
// 8 LTE
constexpr char SIMCOM_NETWORK_TYPE_STATUS[] = "+CNSMOD"; 
// TODO: DTMF

// AT+CIPCCFG Configure Parameters of Socket
constexpr char GSM_SIMCOM_SOCKET_CONFIG_CMD[] = "+CIPCCFG";
// AT+CIPTIMEOUT is used to set timeout value for AT+NETOPEN/AT+CIPOPEN/AT+CIPSEND.
// AT+CIPTIMEOUT: <netopen_timeout>,<cipopen_timeout>,<cipsend_timeout>
constexpr char GSM_SIMCOM_SOCKET_TIMEOUT_CMD[] = "+CIPTIMEOUT";

// set AT+CVHU=0. Otherwise, ATH command will be ignored and "OK" response is given only
constexpr char GSM_SIMCOM_ATH_FIX_CMD[] = "+CVHU";

// TODO: Time zone reporting?


enum GSM_SIMCOM_CONFIGURATION_STEP {
    GSM_SIMCOM_CONFIGURATION_STEP_NONE = 0,

    GSM_SIMCOM_CONFIGURATION_STEP_CMTE,
    GSM_SIMCOM_CONFIGURATION_STEP_CNSMOD,
    GSM_SIMCOM_CONFIGURATION_STEP_CVHU,

    GSM_SIMCOM_CONFIGURATION_STEP_CIPCCFG,
    GSM_SIMCOM_CONFIGURATION_STEP_CIPTIMEOUT,

    GSM_SIMCOM_CONFIGURATION_STEP_COMPLETE
};

class SimComGSMNetworkManager: public GSMNetworkManager
{
private:
    uint8_t simcomConfigurationStep = GSM_SIMCOM_CONFIGURATION_STEP_NONE;
    void NextSimComConfigurationStep();

protected:
    void FetchModemStats() override;
    void ContinueConfigureModem() override;

public:
    SimComGSMNetworkManager(GSMModemManager *gsmManager);
    virtual ~SimComGSMNetworkManager();

    bool OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
};