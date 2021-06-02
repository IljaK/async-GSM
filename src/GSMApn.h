#pragma once
#include "BaseGSMHandler.h"
#include "GSMUtils.h"

// "AT+UPSD=0,1,\"%s\"", _apn
// "AT+UPSD=0,2,\"%s\"", _username
// "AT+UPSD=0,3,\"%s\"", _password
// "AT+UPSD=0,7,\"0.0.0.0\""
constexpr char GSM_APN_CONFIG_CMD[] = "+UPSD"; // AT+USOCR=6  6 - TCP; 16 - UDP;
// "AT+UPSDA=0,3"  Activate stored profile
// "AT+UPSDA=0,4"  PDP context deactivation
constexpr char GSM_APN_CONTROL_CMD[] = "+UPSDA";

// "AT+UPSND=0,8" - Check the status of connection profile dentifier "0".
constexpr char GSM_APN_CHECK_CMD[] = "+UPSND";

constexpr char GSM_APN_ACTIVATED_EVENT[] = "+UUPSDA"; // +UUPSDA: 0,"100.97.138.97"
constexpr char GSM_APN_DEACTIVATED_EVENT[] = "+UUPSDD"; // +UUPSDD: 0

enum UPSD_SETTING_INDEX {
    UPSD_SETTING_APN = 1,
    UPSD_SETTING_LOGIN = 2,
    UPSD_SETTING_PSWD = 3,
    UPSD_SETTING_ASSIGNED_IP = 7
};

class IGSMApnHandler
{
public:
    virtual void OnApnActivated(bool isSuccess) = 0;
    virtual void OnApnDeactivated(bool isSuccess) = 0;
};

enum GSM_APN_STATE {
    GSM_APN_DEACTIVATED,
    GSM_APN_ACTIVATING,
    GSM_APN_ACTIVE,
    GSM_APN_DEACTIVATING
};

class GSMApn: public IBaseGSMHandler
{
private:
    BaseGSMHandler *gsmHandler = NULL;
    IGSMApnHandler *apnHandler = NULL;
    GSM_APN_STATE apnState = GSM_APN_DEACTIVATED;

    IPAddr apnAddr;
    IPAddr deviceAddr;

    char* login = NULL;
    char* password = NULL;

    void FlushAuthData();

    UPSD_SETTING_INDEX GetNextSetting(int index);
    void SendSetting(UPSD_SETTING_INDEX setting);

public:
    GSMApn(BaseGSMHandler *gsmHandler);
    virtual ~GSMApn();

    void SetAPNHandler(IGSMApnHandler *apnHandler);

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

    bool Connect(char* apn, char* login = NULL, char* password = NULL);
    bool Connect();
    bool Deactivate();

    bool IsActive();
    IPAddr GetDeviceAddr();
};