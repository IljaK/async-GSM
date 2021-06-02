#pragma once
#include "BaseGSMHandler.h"

constexpr char GSM_GPRS_CMD[] = "+CGATT";

// "AT+CGATT=1"
// "AT+UPSD=0,1,\"%s\"", _apn
// "AT+UPSD=0,2,\"%s\"", _username
// "AT+UPSD=0,3,\"%s\"", _password
// "AT+UPSD=0,7,\"0.0.0.0\""
// "AT+UPSDA=0,3" // Activate stored profile
// "AT+UPSND=0,8" - check profile status

// Deactivate
// "AT+UPSDA=0,4" // PDP context deactivation
// "AT+CGATT=0" // Disable gprs

class IGPRSHandler
{
    virtual void OnGPRSAttached();
    virtual void OnGPRSDetached();
};

class GPRSHandler
{
private:
    BaseGSMHandler *gsmHandler = NULL;
public:
    GPRSHandler(BaseGSMHandler *gsmHandler);
    ~GPRSHandler();
    void Attach(const char* apn, const char* user_name, const char* password);
    void Detach();
};