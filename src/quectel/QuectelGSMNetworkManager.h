#pragma once
#include "GSMNetworkManager.h"

//constexpr char QUECTEL_TEMP_THRESHOLD_CMD[] = "+CMTE"; // AT+CMTE=1 // PIN required: No

// This command is used to read the temperature of the module.
// Usage: AT+QTEMP - no question vark and values!
constexpr char QUECTEL_TEMP_CMD[] = "+QTEMP";

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
constexpr char QUECTEL_NETWORK_TYPE_STATUS[] = "+CNSMOD"; 
// TODO: DTMF

// AT+CIPCCFG Configure Parameters of Socket
constexpr char GSM_QUECTEL_SOCKET_CONFIG_CMD[] = "+CIPCCFG";
// AT+CIPTIMEOUT is used to set timeout value for AT+NETOPEN/AT+CIPOPEN/AT+CIPSEND.
// AT+CIPTIMEOUT: <netopen_timeout>,<cipopen_timeout>,<cipsend_timeout>
constexpr char GSM_QUECTEL_SOCKET_TIMEOUT_CMD[] = "+CIPTIMEOUT";

// set AT+CVHU=0. Otherwise, ATH command will be ignored and "OK" response is given only
constexpr char GSM_QUECTEL_ATH_FIX_CMD[] = "+CVHU";

// AT+QISDE Control Whether to Echo the Data for AT+QISEND
// AT+QICFG Configure Optional Parameters


enum GSM_QUECTEL_CONFIGURATION_STEP {
    GSM_QUECTEL_CONFIGURATION_STEP_NONE = 0,

    GSM_QUECTEL_CONFIGURATION_STEP_CMTE,
    GSM_QUECTEL_CONFIGURATION_STEP_CNSMOD,
    GSM_QUECTEL_CONFIGURATION_STEP_CVHU,

    GSM_QUECTEL_CONFIGURATION_STEP_CIPCCFG,
    GSM_QUECTEL_CONFIGURATION_STEP_CIPTIMEOUT,

    GSM_QUECTEL_CONFIGURATION_STEP_COMPLETE
};

class QuectelGSMNetworkManager: public GSMNetworkManager
{
private:
    uint8_t simcomConfigurationStep = GSM_QUECTEL_CONFIGURATION_STEP_NONE;
    void NextQuectelConfigurationStep();

protected:
    void FetchModemStats() override;
    void ContinueConfigureModem() override;

public:
    QuectelGSMNetworkManager(GSMModemManager *gsmManager);
    virtual ~QuectelGSMNetworkManager();

    bool OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
};