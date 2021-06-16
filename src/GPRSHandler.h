#pragma once
#include "common/GSMUtils.h"
#include "BaseGSMHandler.h"

constexpr unsigned long APN_CONNECT_CMD_TIMEOUT = 20000000ul;
constexpr unsigned long APN_STATUS_CHECK_DELAY = 200000ul;

constexpr char GSM_GPRS_CMD[] = "+CGATT";

// "AT+UPSD=0,1,\"%s\"", _apn
// "AT+UPSD=0,2,\"%s\"", _username
// "AT+UPSD=0,3,\"%s\"", _password
// "AT+UPSD=0,7,\"0.0.0.0\""
constexpr char GSM_APN_CONFIG_CMD[] = "+UPSD"; // AT+USOCR=6  6 - TCP; 16 - UDP;

// "AT+UPSDA=0,3"  Activate stored profile
// "AT+UPSDA=0,4"  PDP context deactivation
constexpr char GSM_APN_CONTROL_CMD[] = "+UPSDA";

// "AT+UPSND=0,8" - Check the status of connection
// +UPSND: 0,8,1
// "AT+UPSND=0,0" - Check the status of ip
// +UPSND: 0,0,"37.157.74.89"
constexpr char GSM_APN_FETCH_DATA_CMD[] = "+UPSND";

// Triggers on profile has been disabled
// In case of PDP context deactivation with AT+UPSDA=0,4, the +UUPSDD URC is not issued
constexpr char GSM_APN_DEACTIVATED_EVENT[] = "+UUPSDD"; // +UUPSDD: 0

enum UPSD_SETTING_INDEX {
    UPSD_SETTING_APN = 1,
    UPSD_SETTING_LOGIN = 2,
    UPSD_SETTING_PSWD = 3,
    UPSD_SETTING_ASSIGNED_IP = 7
};

enum UPSDN_SETTING_INDEX {
    UPSDN_SETTING_IP = 0,
    UPSDN_SETTING_STATUS = 8
};

class IGPRSHandler
{
public:
    virtual void OnGPRSStarted(bool isSuccess) = 0;
    virtual void OnGPRSStopped(bool isSuccess) = 0;
};

enum GSM_APN_STATE {
    GSM_APN_DEACTIVATED,
    GSM_APN_ACTIVATING,
    GSM_APN_ACTIVE,
    GSM_APN_DEACTIVATING
};

class GPRSHandler: public IBaseGSMHandler, public ITimerCallback
{
private:
    BaseGSMHandler *gsmHandler = NULL;
    IGPRSHandler *apnHandler = NULL;
    GSM_APN_STATE apnState = GSM_APN_DEACTIVATED;
    TimerID checkTimer = 0;

    IPAddr deviceAddr;

    char *apn = NULL;
    char* login = NULL;
    char* password = NULL;

    void FlushAuthData();

    UPSD_SETTING_INDEX GetNextSetting(int index);
    void SendSetting(UPSD_SETTING_INDEX setting);

public:
    GPRSHandler(BaseGSMHandler *gsmHandler);
    virtual ~GPRSHandler();

    void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    void SetAPNHandler(IGPRSHandler *apnHandler);

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

    bool Connect(char* apn, char* login = NULL, char* password = NULL);
    bool Connect();
    bool Deactivate();

    bool IsActive();
    IPAddr GetDeviceAddr();
    void OnModemReboot();
};