#pragma once
#include "GSMNetworkManager.h"

// Enables / disables the smart temperature mode:
// 0 (default value and factory-programmed value): smart temperature feature disabled
// 1: smart temperature feature enabled; the indication by means of the +UUSTS URC and shutting down (if needed) are performed
// 2: smart temperature indication enabled; the +UUSTS URC will be issued to notify the Smart Temperature Supervisor status
// 3: smart temperature feature enabled with no indication; the shutdown (if needed) is performed, but without a URC notification 
// Allowed values:
// TOBY-L4 - 1, 3 (default value and factory-programmed value)
// TOBY-L2 / MPCI-L2 / LARA-R2 / TOBY-R2 / SARA-U2 / LISA-U2 / LISA-U1 /
// SARA-G4 / SARA-G3 / LEON-G1 - 0 (default value and factory-programmed value), 1, 2
constexpr char GSM_TEMP_THRESHOLD_CMD[] = "+USTS"; // PIN required: No
constexpr char GSM_TEMP_THRESHOLD_EVENT[] = "+UUSTS";
// Returns the values of internal temperature sensors of the specified unit.
constexpr char GSM_TEMP_CMD[] = "+UTEMP"; // PIN required: No
// Reports the network or the device PS (Packet Switched) radio capabilities.
constexpr char GSM_NETWORK_TYPE_STATUS[] = "+UREG"; // AT+UCREG=1 // PIN required: No
// Allows to enable / disable the reporting voice or data call status on the DTE using the URC +UCALLSTAT
constexpr char GSM_CMD_CALL_STAT[] = "+UCALLSTAT"; // AT+UCALLSTAT=1 // PIN required: No
// Enables/disables the HEX mode for +USOWR, +USOST, +USORD and +USORF AT commands.
constexpr char GSM_CMD_HEX_MODE[] = "+UDCONF"; // AT+UDCONF=1,1 // PIN required: No
// Enables/disables the DTMF detector and, independently for each specific AT terminal, the related URCs
constexpr char GSM_CMD_DTFM[] = "+UDTMFD"; // AT+UDTMFD=1,2 // PIN required: No


enum GSM_UBLOX_CONFIGURATION_STEP {
    GSM_UBLOX_CONFIGURATION_STEP_NONE = 0,

    GSM_UBLOX_CONFIGURATION_STEP_USTS,
    GSM_UBLOX_CONFIGURATION_STEP_UTEMP,
    GSM_UBLOX_CONFIGURATION_STEP_UREG,
    GSM_UBLOX_CONFIGURATION_STEP_UCALLSTAT,
    GSM_UBLOX_CONFIGURATION_STEP_UDCONF,
    GSM_UBLOX_CONFIGURATION_STEP_UDTMFD,

    GSM_UBLOX_CONFIGURATION_STEP_COMPLETE
};

class UbloxGSMNetworkManager: public GSMNetworkManager
{
private:
    uint8_t ubloxConfigurationStep = GSM_UBLOX_CONFIGURATION_STEP_NONE;
    void NextUbloxConfigurationStep();

protected:
    void FetchModemStats() override;

    void ContinueConfigureModem() override;

public:
    UbloxGSMNetworkManager(GSMModemManager *gsmManager);
    virtual ~UbloxGSMNetworkManager();

    bool OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
};