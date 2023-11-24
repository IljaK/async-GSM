#include "SimComGSMNetworkManager.h"
#include "common/GSMUtils.h"
#include "command/ULong2ModemCMD.h"

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
        char *ureg = data + strlen(SIMCOM_NETWORK_TYPE_STATUS) + 2;
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

    return false;
}

void SimComGSMNetworkManager::FetchModemStats()
{
    GSMNetworkManager::FetchModemStats();
    modemManager->AddCommand(new BaseModemCMD(SIMCOM_TEMP_CMD, MODEM_COMMAND_TIMEOUT, false));
}

void SimComGSMNetworkManager::ContinueConfigureModem()
{
    Serial.println("SimComGSMNetworkManager::ContinueConfigureModem()");
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

    }
}