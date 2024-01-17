#include "UbloxGSMCallManager.h"
#include "UbloxGSMNetworkManager.h"
#include "common/GSMUtils.h"

UbloxGSMCallManager::UbloxGSMCallManager(GSMModemManager *gsmManager):GSMCallManager(gsmManager)
{

}

UbloxGSMCallManager::~UbloxGSMCallManager()
{

}

/*
void UbloxGSMCallManager::HangUpCall()
{
    // 
    gsmManager->ForceCommand(new BaseModemCMD("H"));
}
*/

bool UbloxGSMCallManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (IsEvent(GSM_CALL_DTMF_EVENT, data, dataLen)) {
        char *pCode = data + strlen(GSM_CALL_DTMF_EVENT) + 2;
        HandleDTMF(pCode[0]);
        return true;
    }
    if (IsEvent(GSM_CMD_CALL_STAT, data, dataLen)) {
        // Outgoing: 2->3->0->6
        // 0 = accepted ongoing call
        // Incoming:
        // 4->0->6

        char *clccContent = data + strlen(GSM_CMD_CALL_STAT) + 2;
		char *clccArgs[2];
		SplitString(clccContent, ',', clccArgs, 2, false);

        HandleCallState((VOICE_CALLSTATE)atoi(clccArgs[1]));
        return true;
    }
    return GSMCallManager::OnGSMEvent(data, dataLen);
}

bool UbloxGSMCallManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    return GSMCallManager::OnGSMResponse(request, response, respLen, type);
}

void UbloxGSMCallManager::HandleCallState(VOICE_CALLSTATE state)
{
    if (state == VOICE_CALL_STATE_INCOMING) {
        if (gsmManager->ForceCommand(new BaseModemCMD(GSM_CALL_STATE_CMD))) {
            return;
        }
    }
    UpdateCallState(state);
}