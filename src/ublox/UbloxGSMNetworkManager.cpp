#include "UbloxGSMNetworkManager.h"
#include "common/GSMUtils.h"
#include "command/ULong2ModemCMD.h"

UbloxGSMNetworkManager::UbloxGSMNetworkManager(GSMModemManager *modemManager):
    GSMNetworkManager(modemManager)
{

}

UbloxGSMNetworkManager::~UbloxGSMNetworkManager()
{

}

bool UbloxGSMNetworkManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (GSMNetworkManager::OnGSMEvent(data, dataLen)) {
        return true;
    }

    if (IsEvent(GSM_NETWORK_TYPE_STATUS, data, dataLen)) {
        char *ureg = data + strlen(GSM_NETWORK_TYPE_STATUS) + 2;
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
    if (IsEvent(GSM_TEMP_THRESHOLD_EVENT, data, dataLen)) {
        // GSM_TEMP_THRESHOLD_EVENT
        char *uusts = data + strlen(GSM_TEMP_THRESHOLD_EVENT) + 2;
		char *uustsArgs[2];
        SplitString(uusts, ',', uustsArgs, 2, false);
        if (uustsArgs[1] != NULL) {
            GSMNetworkManager::UpdateThresoldState((GSM_THRESHOLD_STATE)atoi(uustsArgs[1]));
        }
        return true;
    }

    return false;
}

bool UbloxGSMNetworkManager::OnGSMResponse(BaseModemCMD *request, char *response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (GSMNetworkManager::OnGSMResponse(request, response, respLen, type)) {
        return true;

    }
    // ConfigurationCommands
    if (strcmp(request->cmd, GSM_TEMP_CMD) == 0) {
        if (request->GetIsCheck()) {
            if (type == MODEM_RESPONSE_DATA) {
                if (strlen(response) > strlen(GSM_TEMP_CMD) + 2) {
                    char* temp = response + strlen(GSM_TEMP_CMD) + 2;
                    float tempVal = atoi(temp);
                    UpdateTemperature(tempVal / 10.0f);
                }
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // TODO: ?
            }
        } else {
            NextUbloxConfigurationStep();
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_CMD_HEX_MODE) == 0) {
        NextUbloxConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_DTFM) == 0) {
        NextUbloxConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_NETWORK_TYPE_STATUS) == 0) {
        NextUbloxConfigurationStep();
        return true;
    }
    if (strcmp(request->cmd, GSM_CMD_CALL_STAT) == 0) {
        NextUbloxConfigurationStep();
        return true;
    }

    // Functional commands
    if (strcmp(request->cmd, GSM_TEMP_THRESHOLD_CMD) == 0) {
        NextUbloxConfigurationStep();
        return true;
    }

    return false;
}

void UbloxGSMNetworkManager::FetchModemStats()
{
    GSMNetworkManager::FetchModemStats();
    modemManager->AddCommand(new BaseModemCMD(GSM_TEMP_CMD, MODEM_COMMAND_TIMEOUT, true));
}

void UbloxGSMNetworkManager::ContinueConfigureModem()
{
    // Ublox Specific configuration
    NextUbloxConfigurationStep();
}

void UbloxGSMNetworkManager::NextUbloxConfigurationStep()
{
    ubloxConfigurationStep++;
    if (ubloxConfigurationStep >= GSM_UBLOX_CONFIGURATION_STEP_COMPLETE) {
        // DONE?
        ConfigureModemCompleted();
        return;
    }

    switch (ubloxConfigurationStep) {
        case GSM_UBLOX_CONFIGURATION_STEP_NONE:
            // TODO: ?
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_USTS:
            modemManager->ForceCommand(new ByteModemCMD(1, GSM_TEMP_THRESHOLD_CMD));
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_UTEMP:
            modemManager->ForceCommand(new ByteModemCMD(0, GSM_TEMP_CMD));
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_UREG:
            modemManager->ForceCommand(new ByteModemCMD(1, GSM_NETWORK_TYPE_STATUS));
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_UCALLSTAT:
            modemManager->ForceCommand(new ByteModemCMD(1, GSM_CMD_CALL_STAT));
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_UDCONF:
            modemManager->ForceCommand(new ULong2ModemCMD(1,1, GSM_CMD_HEX_MODE));
            break;
        case GSM_UBLOX_CONFIGURATION_STEP_UDTMFD:
            modemManager->ForceCommand(new ULong2ModemCMD(1,2, GSM_CMD_DTFM));
            break;

    }
}