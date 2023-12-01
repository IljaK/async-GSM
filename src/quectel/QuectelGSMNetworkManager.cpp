#include "QuectelGSMNetworkManager.h"
#include "common/GSMUtils.h"
#include "command/ULong2ModemCMD.h"
#include "command/LongArrayModemCMD.h"

QuectelGSMNetworkManager::QuectelGSMNetworkManager(GSMModemManager *modemManager):
    GSMNetworkManager(modemManager)
{

}

QuectelGSMNetworkManager::~QuectelGSMNetworkManager()
{

}

bool QuectelGSMNetworkManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (GSMNetworkManager::OnGSMEvent(data, dataLen)) {
        return true;
    }

    if (IsEvent(QUECTEL_NETWORK_TYPE_STATUS, data, dataLen)) {
        char *ureg = data + strlen(QUECTEL_NETWORK_TYPE_STATUS) + 2;
        GSM_NETWORK_TYPE type = GSM_NETWORK_UNKNOWN;
        switch (atoi(ureg))
        {
            case 0:
                // 0: not registered for PS service
                break;
            case 1:
                // 1: registered for PS service, RAT=2G, GPRS available
                type = GSM_NETWORK_2G_GPRS;
                break;
            case 2:
                // 2: registered for PS service, RAT=2G, EDGE available
                type = GSM_NETWORK_2G_EDGE;
                break;
            case 3:
                // 3: registered for PS service, RAT=3G, WCDMA available
                type = GSM_NETWORK_3G_WCDMA;
                break;
            case 4:
                // 4: registered for PS service, RAT=3G, HSDPA available
                type = GSM_NETWORK_3G_HSDPA;
                break;
            case 5:
                // 5: registered for PS service, RAT=3G, HSUPA available
                type = GSM_NETWORK_3G_HSUPA;
                break;
            case 6:
                // 6: registered for PS service, RAT=3G, HSDPA and HSUPA available
                type = GSM_NETWORK_3G_HSDPA_HSUPA;
                break;
            case 7:
                // 7: registered for PS service, RAT=4G
                type = GSM_NETWORK_4G_LTE;
                break;
            case 8:
                // 8: registered for PS service, RAT=2G, GPRS available, DTM available
                type = GSM_NETWORK_2G_GPRS;
                break;
            case 9:
                // 9: registered for PS service, RAT=2G, EDGE available, DTM available
                type = GSM_NETWORK_2G_EDGE;
                break;
        }
        GSMNetworkManager::UpdateNetworkType(type);
        return true;
    }

    if (IsEvent(QUECTEL_TEMP_THRESHOLD_CMD, data, dataLen)) {
        // GSM_TEMP_THRESHOLD_EVENT
        char *uusts = data + strlen(QUECTEL_TEMP_THRESHOLD_CMD) + 2;
		char *uustsArgs[2];
        SplitString(uusts, ',', uustsArgs, 2, false);
        if (uustsArgs[1] != NULL) {
            GSMNetworkManager::UpdateThresoldState((GSM_THRESHOLD_STATE)atoi(uustsArgs[1]));
        }
        return true;
    }

    return false;
}

bool QuectelGSMNetworkManager::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (GSMNetworkManager::OnGSMResponse(request, response, respLen, type)) {
        return true;

    }

    // +QTEMP: <bb_temp>,<XO_temp>,<PA_temp>
    // <bb_temp> Integer type. Baseband temperature. Unit: Degree Celsius.
    // <XO_temp> Integer type. XO temperature. Unit: Degree Celsius.
    // <PA_temp> Integer type. PA temperature. Unit: Degree Celsius

    if (strcmp(request->cmd, QUECTEL_TEMP_CMD) == 0) {
        if (request->GetIsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                if (strlen(response) > strlen(QUECTEL_TEMP_CMD) + 2) {
                    char* temp = response + strlen(QUECTEL_TEMP_CMD) + 2;
                    char *tempArgs[3];
                    SplitString(temp, ',', tempArgs, 3, false);
                    UpdateTemperature(atoi(tempArgs[0]));
                }
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // TODO: ?
            }
        }
        return true;
    }

    // ConfigurationCommands
    if (strcmp(request->cmd, QUECTEL_TEMP_THRESHOLD_CMD) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, QUECTEL_NETWORK_TYPE_STATUS) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_QUECTEL_ATH_FIX_CMD) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_CONFIG_CMD) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_TIMEOUT_CMD) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }

    return false;
}

void QuectelGSMNetworkManager::FetchModemStats()
{
    GSMNetworkManager::FetchModemStats();
    modemManager->AddCommand(new BaseModemCMD(QUECTEL_TEMP_CMD, MODEM_COMMAND_TIMEOUT, false));
}

void QuectelGSMNetworkManager::ContinueConfigureModem()
{
    simcomConfigurationStep = GSM_QUECTEL_CONFIGURATION_STEP_NONE;
    // Quectel Specific configuration
    NextQuectelConfigurationStep();
}

void QuectelGSMNetworkManager::NextQuectelConfigurationStep()
{
    simcomConfigurationStep++;
    if (simcomConfigurationStep >= GSM_QUECTEL_CONFIGURATION_STEP_COMPLETE) {
        // DONE?
        ConfigureModemCompleted();
        return;
    }

    switch (simcomConfigurationStep) {
        case GSM_QUECTEL_CONFIGURATION_STEP_NONE:
            // TODO: ?
            break;
        case GSM_QUECTEL_CONFIGURATION_STEP_CMTE:
            modemManager->ForceCommand(new ByteModemCMD(1, QUECTEL_TEMP_THRESHOLD_CMD));
            break;
        case GSM_QUECTEL_CONFIGURATION_STEP_CNSMOD:
            modemManager->ForceCommand(new ByteModemCMD(1, QUECTEL_NETWORK_TYPE_STATUS));
            break;
        case GSM_QUECTEL_CONFIGURATION_STEP_CVHU:
            modemManager->ForceCommand(new ByteModemCMD(0, GSM_QUECTEL_ATH_FIX_CMD));
            break;

        case GSM_QUECTEL_CONFIGURATION_STEP_CIPCCFG:
            {
                // <NmRetry>,<DelayTm>,<Ack>,<errMode>,<Header-Type>,<AsyncMode>,<TimeoutVal>
                long vals[7] = { 10, 0, 0, 1, 1, 0, 500 };
                modemManager->ForceCommand(new LongArrayModemCMD(vals, (uint8_t)(sizeof(vals) / sizeof(long)), GSM_QUECTEL_SOCKET_CONFIG_CMD));
            }
            break;
        case GSM_QUECTEL_CONFIGURATION_STEP_CIPTIMEOUT:
            {
                // <netopen_timeout>,<cipopen_timeout>,<cipsend_timeout>
                long vals[3] = { 120000, 10000, 10000 };
                modemManager->ForceCommand(new LongArrayModemCMD(vals, (uint8_t)(sizeof(vals) / sizeof(long)), GSM_QUECTEL_SOCKET_TIMEOUT_CMD));
            }
            break;
        default:
            break;

    }
}