#include "SimComGSMNetworkManager.h"
#include "common/GSMUtils.h"
#include "command/ULong2ModemCMD.h"
#include "command/LongArrayModemCMD.h"

SimComGSMNetworkManager::SimComGSMNetworkManager(GSMModemManager *modemManager):
    GSMNetworkManager(modemManager)
{

}

SimComGSMNetworkManager::~SimComGSMNetworkManager()
{

}

bool SimComGSMNetworkManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (GSMNetworkManager::OnGSMEvent(data, dataLen)) {
        return true;
    }

    if (IsEvent(SIMCOM_NETWORK_TYPE_STATUS, data, dataLen)) {
        char *cnsmod = data + strlen(SIMCOM_NETWORK_TYPE_STATUS) + 2;
        GSM_NETWORK_TYPE type = GSM_NETWORK_UNKNOWN;
        switch (atoi(cnsmod))
        {
            case 0:
                break;
            case 1:
                type = GSM_NETWORK_2G_GSM;
                break;
            case 2:
                type = GSM_NETWORK_2G_GPRS;
                break;
            case 3:
                type = GSM_NETWORK_2G_EDGE;
                break;
            case 4:
                type = GSM_NETWORK_3G_WCDMA;
                break;
            case 5:
                type = GSM_NETWORK_3G_HSDPA;
                break;
            case 6:
                type = GSM_NETWORK_3G_HSUPA;
                break;
            case 7:
                type = GSM_NETWORK_3G_HSPA;
                break;
            case 8:
                type = GSM_NETWORK_4G_LTE;
                break;
        }
        GSMNetworkManager::UpdateNetworkType(type);
        return true;
    }

    if (IsEvent(SIMCOM_TEMP_THRESHOLD_CMD, data, dataLen)) {
        // GSM_TEMP_THRESHOLD_EVENT
        char *uusts = data + strlen(SIMCOM_TEMP_THRESHOLD_CMD) + 2;
		char *uustsArgs[2];
        SplitString(uusts, ',', uustsArgs, 2, false);
        if (uustsArgs[1] != NULL) {
            GSMNetworkManager::UpdateThresoldState((GSM_THRESHOLD_STATE)atoi(uustsArgs[1]));
        }
        return true;
    }

    return false;
}

bool SimComGSMNetworkManager::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (GSMNetworkManager::OnGSMResponse(request, response, respLen, type)) {
        return true;

    }
    if (strcmp(request->cmd, SIMCOM_TEMP_CMD) == 0) {
        if (request->GetIsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                if (strlen(response) > strlen(SIMCOM_TEMP_CMD) + 2) {
                    char* temp = response + strlen(SIMCOM_TEMP_CMD) + 2;
                    UpdateTemperature(atoi(temp));
                }
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // TODO: ?
            }
        }
        return true;
    }

    // ConfigurationCommands
    if (strcmp(request->cmd, SIMCOM_TEMP_THRESHOLD_CMD) == 0) {
        NextSimComConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, SIMCOM_NETWORK_TYPE_STATUS) == 0) {
        NextSimComConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_SIMCOM_ATH_FIX_CMD) == 0) {
        NextSimComConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_CONFIG_CMD) == 0) {
        NextSimComConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_TIMEOUT_CMD) == 0) {
        NextSimComConfigurationStep();
        return true;
    }

    return false;
}

void SimComGSMNetworkManager::FetchModemStats()
{
    GSMNetworkManager::FetchModemStats();
    modemManager->AddCommand(new BaseModemCMD(SIMCOM_TEMP_CMD, MODEM_COMMAND_TIMEOUT, false));
}

void SimComGSMNetworkManager::ContinueConfigureModem()
{
    simcomConfigurationStep = GSM_SIMCOM_CONFIGURATION_STEP_NONE;
    // SimCom Specific configuration
    NextSimComConfigurationStep();
}

void SimComGSMNetworkManager::NextSimComConfigurationStep()
{
    simcomConfigurationStep++;
    if (simcomConfigurationStep >= GSM_SIMCOM_CONFIGURATION_STEP_COMPLETE) {
        // DONE?
        ConfigureModemCompleted();
        return;
    }

    switch (simcomConfigurationStep) {
        case GSM_SIMCOM_CONFIGURATION_STEP_NONE:
            // TODO: ?
            break;
        case GSM_SIMCOM_CONFIGURATION_STEP_CMTE:
            modemManager->ForceCommand(new ByteModemCMD(1, SIMCOM_TEMP_THRESHOLD_CMD));
            break;
        case GSM_SIMCOM_CONFIGURATION_STEP_CNSMOD:
            modemManager->ForceCommand(new ByteModemCMD(1, SIMCOM_NETWORK_TYPE_STATUS));
            break;
        case GSM_SIMCOM_CONFIGURATION_STEP_CVHU:
            modemManager->ForceCommand(new ByteModemCMD(0, GSM_SIMCOM_ATH_FIX_CMD));
            break;

        case GSM_SIMCOM_CONFIGURATION_STEP_CIPCCFG:
            {
                // <NmRetry>,<DelayTm>,<Ack>,<errMode>,<Header-Type>,<AsyncMode>,<TimeoutVal>
                long vals[7] = { 10, 0, 0, 1, 1, 0, 500 };
                modemManager->ForceCommand(new LongArrayModemCMD(vals, (uint8_t)(sizeof(vals) / sizeof(long)), GSM_SIMCOM_SOCKET_CONFIG_CMD));
            }
            break;
        case GSM_SIMCOM_CONFIGURATION_STEP_CIPTIMEOUT:
            {
                // <netopen_timeout>,<cipopen_timeout>,<cipsend_timeout>
                long vals[3] = { 120000, 10000, 10000 };
                modemManager->ForceCommand(new LongArrayModemCMD(vals, (uint8_t)(sizeof(vals) / sizeof(long)), GSM_SIMCOM_SOCKET_TIMEOUT_CMD));
            }
            break;
        default:
            break;

    }
}