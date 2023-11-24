#pragma once
#include "common/GSMUtils.h"
#include "IGPRSHandler.h"
#include "GSMModemManager.h"
#include "command/ByteModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ULong2StringModemCMD.h"
#include "command/IPResolveModemCMD.h"

constexpr unsigned long APN_CONNECT_CMD_TIMEOUT = 20000000ul;
constexpr unsigned long APN_STATUS_CHECK_DELAY = 200000ul;

constexpr char GSM_GPRS_CMD[] = "+CGATT";

enum GSM_APN_STATE {
    GSM_APN_DEACTIVATED,
    GSM_APN_ACTIVATING,
    GSM_APN_ACTIVE,
    GSM_APN_DEACTIVATING
};

class GPRSManager: public IBaseGSMHandler
{
private:
    IGPRSHandler *apnHandler = NULL;
    GSM_APN_STATE apnState = GSM_APN_DEACTIVATED;

protected:
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
    bool ForceDeactivate();

public:
    GPRSManager(GSMModemManager *gsmManager);
    virtual ~GPRSManager();

    void SetAPNHandler(IGPRSHandler *apnHandler);

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    bool Connect(char* apn, char* login = NULL, char* password = NULL);
    bool Deactivate();

    bool IsActive();
    IPAddr GetDeviceAddr();
    void OnModemReboot();

    GSM_APN_STATE GetApnState();
    bool ResolveHostName(const char *hostName);
};