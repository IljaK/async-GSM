#pragma once
#include "IGSMNetworkManager.h"
#include "GSMModemManager.h"
#include "common/Util.h"

// Requires:
// AT+UDTMF=1,2
constexpr char GSM_CALL_STATE_CMD[] = "+CLCC"; // Call state
constexpr char GSM_CALL_HANGUP_CMD[] = "+CHUP"; // Call state

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

class IGSMCallManager {
public:
    virtual void HandleCallState(VOICE_CALLSTATE state) = 0;
    virtual void OnDTMF(char sign) = 0;
};

class GSMCallManager: public IBaseGSMHandler
{
private:
    IGSMCallManager *callStateHandler;
    VOICE_CALLSTATE callState = VOICE_CALL_STATE_DISCONNECTED;
protected:
    char callingNumber[MAX_PHONE_LENGTH];
    GSMModemManager *gsmManager = NULL;
    void HandleDTMF(char sign);
    void UpdateCallState(VOICE_CALLSTATE state);
    void HandleDetailedCallInfo(char * content, size_t contentLen);
public:
    GSMCallManager(GSMModemManager *gsmManager);
    virtual ~GSMCallManager();

    virtual bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    virtual bool OnGSMEvent(char * data, size_t dataLen) override;
    
    void SetCallStateHandler(IGSMCallManager *callStateHandler);
    virtual void HangUpCall();
    char * GetCallingNumber();
    virtual void OnModemReboot();

    void HandleDetailedCallInfo(VOICE_CALLSTATE state, char *number, char *name);
};