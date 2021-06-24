#pragma once
#include "BaseGSMHandler.h"
#include "common/Util.h"

// Requires:
// AT+UDTMF=1,2
constexpr char GSM_CALL_STATE_CMD[] = "+CLCC"; // Call state
constexpr char GSM_CALL_STATE_TRIGGER[] = "+UCALLSTAT"; // Call state
constexpr char GSM_CALL_DTMF_CMD[] = "+UUDTMFD"; // Call state

constexpr char GSM_CALL_HANGUP_CMD[] = "H"; // Call state

enum VOICE_CALLSTATE : uint8_t
{
	VOICE_CALL_STATE_ACTIVE,
	VOICE_CALL_STATE_HELD,

	// Outgoing
	VOICE_CALL_STATE_DIALING,
	VOICE_CALL_STATE_ALERTING,

	// Incoming
	VOICE_CALL_STATE_INCOMING,
	VOICE_CALL_STATE_WAITING,

	VOICE_CALL_STATE_DISCONNECTED,
	VOICE_CALL_STATE_CONNECTED
};

class IGSMCallHandler {
public:
    virtual void HandleCallState(VOICE_CALLSTATE state) = 0;
    virtual void OnDTMF(char sign) = 0;
};

class GSMCallHandler: public IBaseGSMHandler, public ITimerCallback
{
private:
    char *callingNumber = NULL;
    IGSMCallHandler *callStateHandler;
    VOICE_CALLSTATE callState = VOICE_CALL_STATE_DISCONNECTED;
protected:
    BaseGSMHandler *gsmHandler = NULL;
public:
    GSMCallHandler(BaseGSMHandler *gsmHandler);
    virtual ~GSMCallHandler();

    virtual bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    virtual bool OnGSMEvent(char * data, size_t dataLen) override;
    
    void SetCallStateHandler(IGSMCallHandler *callStateHandler);
    void HangCall();
    char * GetCallingNumber();
    virtual void OnModemReboot();
};