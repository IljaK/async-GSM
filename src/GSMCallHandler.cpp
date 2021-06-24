#include "GSMCallHandler.h"

GSMCallHandler::GSMCallHandler(BaseGSMHandler *gsmHandler)
{
    this->gsmHandler = gsmHandler;
}
GSMCallHandler::~GSMCallHandler()
{

}

bool GSMCallHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (strncmp(data, GSM_CALL_DTMF_CMD, strlen(GSM_CALL_DTMF_CMD)) == 0) {
        if (callStateHandler != NULL) {
            char *pCode = data + strlen(GSM_CALL_DTMF_CMD) + 2;
            callStateHandler->OnDTMF(pCode[0]);
        }
        return true;
    }
    if (strncmp(data, GSM_CALL_STATE_TRIGGER, strlen(GSM_CALL_STATE_TRIGGER)) == 0) {
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
            //if (!gsmHandler->ForceCommand("AT+CLCC")) {
            if (!gsmHandler->ForceCommand(new BaseModemCMD(GSM_CALL_STATE_CMD))) {
                callStateHandler->HandleCallState(callState);
            }

        } else {
            if (callStateHandler != NULL) {
                callStateHandler->HandleCallState(callState);
            }
            if (callState == VOICE_CALL_STATE_DISCONNECTED || callState == VOICE_CALL_STATE_DISCONNECTED) {
                if (callingNumber != NULL) {
                    free(callingNumber);
                    callingNumber = NULL;
                }
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

        if (callingNumber != NULL) {
            free(callingNumber);
            callingNumber = NULL;
        }

        callingNumber = (char *)malloc(strlen(clccArgs[5]) + 1);
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
    //gsmHandler->ForceCommand("ATH");
    gsmHandler->ForceCommand(new BaseModemCMD(GSM_CALL_HANGUP_CMD));
}

char * GSMCallHandler::GetCallingNumber() {
    return callingNumber;
}

void GSMCallHandler::OnModemReboot()
{
    if (callingNumber != NULL) {
        free(callingNumber);
    }
    callState = VOICE_CALL_STATE_DISCONNECTED;
}