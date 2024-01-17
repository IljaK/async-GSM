#include "GSMCallManager.h"
#include "common/GSMUtils.h"

GSMCallManager::GSMCallManager(GSMModemManager *gsmManager)
{
    this->gsmManager = gsmManager;
}
GSMCallManager::~GSMCallManager()
{

}

bool GSMCallManager::OnGSMEvent(char * data, size_t dataLen)
{
    // Event: [+CLCC: 1,1,4,0,0,"+37211111",145]
    if (IsEvent(GSM_CALL_STATE_CMD, data, dataLen)) {
        HandleDetailedCallInfo(data, dataLen);
        return true;
    }

    return false;
}

bool GSMCallManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strncmp(response, GSM_CALL_STATE_CMD, strlen(GSM_CALL_STATE_CMD)) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
            // +CLCC: 1,1,4,0,0,"+37211111",145,""
            HandleDetailedCallInfo(response, respLen);
        }
        return true;
    }
    return false;
}


void GSMCallManager::HandleDetailedCallInfo(VOICE_CALLSTATE state, char *number, char *name)
{
    switch (state)
    {
    case VOICE_CALL_STATE_DIALING:
    case VOICE_CALL_STATE_INCOMING:
        callingNumber[0] = 0;
        strcpy(callingNumber, number);
        break;
    case VOICE_CALL_STATE_DISCONNECTED:
        callingNumber[0] = 0;
        break;
    default:
        break;
    }
    UpdateCallState(state);
}

void GSMCallManager::HandleDetailedCallInfo(char * content, size_t contentLen)
{
    char *clccContent = content + strlen(GSM_CALL_STATE_CMD) + 2;
    char *clccArgs[8];
    size_t len = SplitString(clccContent, ',', clccArgs, 8, false);
    // remove quotations
    ShiftQuotations(clccArgs, len);

    HandleDetailedCallInfo((VOICE_CALLSTATE)atoi(clccArgs[2]), clccArgs[5], clccArgs[7]);
}

void GSMCallManager::SetCallStateHandler(IGSMCallManager* callStateHandler)
{
    this->callStateHandler = callStateHandler;
}

void GSMCallManager::HangUpCall()
{
    gsmManager->ForceCommand(new BaseModemCMD(GSM_CALL_HANGUP_CMD));
}

char * GSMCallManager::GetCallingNumber() {
    return callingNumber;
}

void GSMCallManager::OnModemReboot()
{
    callState = VOICE_CALL_STATE_DISCONNECTED;
}

void GSMCallManager::HandleDTMF(char sign)
{
    if (sign == 0) return;

    if (callStateHandler != NULL) {
        callStateHandler->OnDTMF(sign);
    }
}

void GSMCallManager::UpdateCallState(VOICE_CALLSTATE state)
{
    if (callState == state) return;

    callState = state;

    if (callState == VOICE_CALL_STATE_DISCONNECTED || callState == VOICE_CALL_STATE_DISCONNECTED) {
        callingNumber[0] = 0;
    }
    
    if (callStateHandler != NULL) {
        callStateHandler->HandleCallState(state);
    }
}