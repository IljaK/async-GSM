#include "QuectelGSMNetworkManager.h"
#include "common/GSMUtils.h"
#include "command/ULong2ModemCMD.h"
#include "command/LongArrayModemCMD.h"

QuectelGSMNetworkManager::QuectelGSMNetworkManager(GSMModemManager *modemManager):
    delayedInitTimer(this),
    GSMNetworkManager(modemManager)
{

}

QuectelGSMNetworkManager::~QuectelGSMNetworkManager()
{

}


void QuectelGSMNetworkManager::NextConfigurationStep()
{
    if (GetConfigurationStep() == GSM_MODEM_CONFIGURATION_STEP_NONE) {
        delayedInitTimer.StartSeconds(2);
        return;
    }
    GSMNetworkManager::NextConfigurationStep();
}

bool QuectelGSMNetworkManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (GSMNetworkManager::OnGSMEvent(data, dataLen)) {
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

    if (strcmp(request->cmd, QUECTEL_NETWORK_TYPE_STATUS) == 0) {
        // +QNWINFO: <Act>,<oper>,<band>,<channel>
        char *qnwinfo = response + strlen(QUECTEL_NETWORK_TYPE_STATUS) + 2;
        char *qnwinfoArgs[4];
        SplitString(qnwinfo, ',', qnwinfoArgs, 4, false);
        GSM_NETWORK_TYPE type = GSM_NETWORK_UNKNOWN;

        if (strcmp("CDMA1X", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("CDMA1X AND HDR", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("CDMA1X AND EHRPD", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("HDR", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("HDR-EHRPD", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("GSM", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_2G_GSM;
        } else if (strcmp("GPRS", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_2G_GPRS;
        } else if (strcmp("EDGE", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_2G_EDGE;
        } else if (strcmp("WCDMA", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_3G_WCDMA;
        } else if (strcmp("HSDPA", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_3G_HSDPA;
        } else if (strcmp("HSUPA", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_3G_HSUPA;
        } else if (strcmp("HSPA+", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_3G_HSDPA_HSUPA;
        } else if (strcmp("TDSCDMA", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_OTHER;
        } else if (strcmp("TDD LTE", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_4G_LTE;
        } else if (strcmp("FDD LTE", qnwinfoArgs[0]) == 0) {
            type = GSM_NETWORK_4G_LTE;
        }

        GSMNetworkManager::UpdateNetworkType(type);
        return true;
    }

    if (strcmp(request->cmd, GSM_QUECTEL_ATH_FIX_CMD) == 0) {
        NextQuectelConfigurationStep();
        return true;
    }

    return false;
}

void QuectelGSMNetworkManager::FetchModemStats()
{
    GSMNetworkManager::FetchModemStats();
    modemManager->AddCommand(new BaseModemCMD(QUECTEL_TEMP_CMD, MODEM_COMMAND_TIMEOUT, false));
    modemManager->AddCommand(new BaseModemCMD(QUECTEL_NETWORK_TYPE_STATUS, MODEM_COMMAND_TIMEOUT, false));
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
        case GSM_QUECTEL_CONFIGURATION_STEP_CVHU:
            modemManager->ForceCommand(new ByteModemCMD(0, GSM_QUECTEL_ATH_FIX_CMD));
            break;
        default:
            break;
    }
}

void QuectelGSMNetworkManager::OnTimerComplete(Timer *timer)
{
    if (timer == &delayedInitTimer) {
        GSMNetworkManager::NextConfigurationStep();
        return;
    }
    GSMNetworkManager::OnTimerComplete(timer);
}

void QuectelGSMNetworkManager::OnModemReboot()
{
    delayedInitTimer.Stop();
    GSMNetworkManager::OnModemReboot();
}