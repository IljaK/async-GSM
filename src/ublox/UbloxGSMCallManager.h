#pragma once
#include "GSMModemManager.h"
#include "GSMCallManager.h"

constexpr char GSM_CALL_DTMF_EVENT[] = "+UUDTMFD"; // Call state

class UbloxGSMCallManager: public GSMCallManager
{
private:
    void HandleCallState(VOICE_CALLSTATE state);
public:
    bool OnGSMEvent(char * data, size_t dataLen) override;
    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;

    UbloxGSMCallManager(GSMModemManager *gsmManager);
    virtual ~UbloxGSMCallManager();

    //void HangUpCall() override;
};