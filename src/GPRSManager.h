#pragma once
#include "common/GSMUtils.h"
#include "IGPRSHandler.h"
#include "GSMModemManager.h"
#include "command/ByteModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ULong2StringModemCMD.h"
#include "command/IPResolveModemCMD.h"
#include "common/Timer.h"

constexpr unsigned long APN_CONNECT_CMD_TIMEOUT = 20000000ul;
constexpr unsigned long APN_STATUS_CHECK_DELAY = 200000ul;

constexpr unsigned long GPRS_COOLDOWN_DELAY = 1000000ul;

constexpr char GSM_GPRS_CMD[] = "+CGATT";

enum GSM_APN_STATE {
    GSM_APN_DEACTIVATED,
    GSM_APN_ACTIVATING,
    GSM_APN_ACTIVE,
    GSM_APN_DEACTIVATING
};

class GPRSManager: public IBaseGSMHandler, public ITimerCallback
{
private:
    IGPRSHandler *apnHandler = NULL;
    GSM_APN_STATE apnState = GSM_APN_DEACTIVATED;

protected:
    Timer coolDownTimer;
    IPAddr deviceAddr;

    char *apn = NULL;
    char* login = NULL;
    char* password = NULL;
    GSMModemManager *gsmManager = NULL;
    virtual bool ResolveHostNameCMD(const char *hostName) = 0;
    void HandleHostNameResolve(const char *hostName, IPAddr ip);
    void HandleAPNUpdate(GSM_APN_STATE apnState);

    // TODO: Override to perform connection
    virtual bool ConnectInternal() { return false; };
    virtual void FlushAuthData();
    virtual bool ForceDeactivate();

    virtual void OnCGACT(bool isCheck, bool value, bool isSuccess);
    bool SendCGACT(bool isCheck, uint8_t value, unsigned long timeout = APN_CONNECT_CMD_TIMEOUT);

public:
    GPRSManager(GSMModemManager *gsmManager);
    virtual ~GPRSManager();

    void SetAPNHandler(IGPRSHandler *apnHandler);

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
    void OnTimerComplete(Timer *timer) override;

    bool Connect(char* apn, char* login = NULL, char* password = NULL);
    virtual bool Deactivate();

    bool IsActive();
    IPAddr GetDeviceAddr();
    void OnModemReboot();

    GSM_APN_STATE GetApnState();
    bool ResolveHostName(const char *hostName);
};