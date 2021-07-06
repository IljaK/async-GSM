#include "GSMCallHandler.h"
#include "common/GSMUtils.h"

GSMCallHandler::GSMCallHandler(BaseGSMHandler *gsmHandler)
{
    this->gsmHandler = gsmHandler;
}
GSMCallHandler::~GSMCallHandler()
{

}

bool GSMCallHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (IsEvent(GSM_CALL_DTMF_CMD, data, dataLen)) {
        if (callStateHandler != NULL) {
            char *pCode = data + strlen(GSM_CALL_DTMF_CMD) + 2;
            callStateHandler->OnDTMF(pCode[0]);
        }
        return true;
    }
    if (IsEvent(GSM_CALL_STATE_TRIGGER, data, dataLen)) {
        // Outgoing: 2->3->0->6
        // 0 = accepted ongoing call
        // Incoming:
        // 4->0->6

        char *clccContent = data + strlen(GSM_CALL_STATE_TRIGGER) + 2;
		char *clccArgs[2];
		SplitString(clccContent, ',', clccArgs, 8, false);

		uint8_t status = atoi(clccArgs[1]);
        callState = (VOICE_CALLSTATE)status;
        if (callState == VOICE_CALL_STATE_INCOMING) {
            // Request number
            if (!gsmHandler->ForceCommand(new BaseModemCMD(GSM_CALL_STATE_CMD))) {
                callStateHandler->HandleCallState(callState);
            }

        } else {
            if (callStateHandler != NULL) {
                callStateHandler->HandleCallState(callState);
            }
            if (callState == VOICE_CALL_STATE_DISCONNECTED || callState == VOICE_CALL_STATE_DISCONNECTED) {
                callingNumber[0] = 0;
            }
        }
        return true;
    }
    return false;
}

bool GSMCallHandler::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strncmp(response, GSM_CALL_STATE_CMD, strlen(GSM_CALL_STATE_CMD)) == 0) {
        // +CLCC: 1,1,4,0,0,"+37211111",145,""
        char *clccContent = response + strlen(GSM_CALL_STATE_CMD) + 2;
		char *clccArgs[8];
		size_t len = SplitString(clccContent, ',', clccArgs, 8, false);
		// remove quotations
		ShiftQuotations(clccArgs, len);

        callingNumber[0] = 0;
        strcpy(callingNumber, clccArgs[5]);

        //bool isIncoming = atoi(clccArgs[1]) == 1;
		uint8_t state = atoi(clccArgs[2]);
		callState = (VOICE_CALLSTATE)state;
        if (callStateHandler != NULL) {
            callStateHandler->HandleCallState(callState);
        }

        return true;
    }
    return false;
}

void GSMCallHandler::SetCallStateHandler(IGSMCallHandler* callStateHandler)
{
    this->callStateHandler = callStateHandler;
}

void GSMCallHandler::HangCall()
{
    gsmHandler->ForceCommand(new BaseModemCMD(GSM_CALL_HANGUP_CMD));
}

char * GSMCallHandler::GetCallingNumber() {
    return callingNumber;
}

void GSMCallHandler::OnModemReboot()
{
    callState = VOICE_CALL_STATE_DISCONNECTED;
}