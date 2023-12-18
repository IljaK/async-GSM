#pragma once
#include "common/GSMUtils.h"
#include "IGPRSHandler.h"
#include "GSMModemManager.h"
#include "command/ByteModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ULong2StringModemCMD.h"
#include "GPRSManager.h"
#include "QuectelHostNameResolve.h"

// Step 1
// AT+CGDCONT - Define PDP context
// AT+CGDCONT: <cid>,<PDP_type>,<APN>
constexpr char QUECTEL_DEFINE_PDP[] = "+CGDCONT";

// Step 2
// AT+QICSGP Configure Parameters of a TCP/IP Context 
constexpr char QUECTEL_AUTH_PDP[] = "+QICSGP";

// Step 3
// AT+QIACT Activate a PDP Context
constexpr char QUECTEL_ACTIVATE_PDP[] = "+QIACT";

// Step 4
// Query IP address of the activated context:
// AT+QIACT?

// TODO: AT+QIDNSGIP Get IP Address by Domain Name
// AT+QIDNSGIP Get IP Address by Domain Name
constexpr char QUECTEL_RESOLVE_DNS_CMD[] = "+QIDNSGIP"; // Get IP of dns name


// Step 3
// AT+QIACT Activate a PDP Context
constexpr char QUECTEL_DEACTIVATE_PDP[] = "+QIDEACT";


class QuectelGPRSManager: public GPRSManager
{
private:
    Timer gipTimer;
    void InternalHostNameResolve();
    QuectelHostNameResolve *hostnameResolve = NULL;

protected:
    void FlushAuthData() override;
    bool ResolveHostNameCMD(const char *hostName) override;
    bool ConnectInternal() override;
    
    void OnCGACT(bool isCheck, bool value, bool isSuccess) override;

    void ActivatePDP();

public:
    QuectelGPRSManager(GSMModemManager *gsmManager);
    virtual ~QuectelGPRSManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

	void OnTimerComplete(Timer *timer) override;

    void HandlePDEACT(char **args, size_t argsLen);
    void HandleDNSGIP(char **args, size_t argsLen);

    bool ForceDeactivate() override;
};