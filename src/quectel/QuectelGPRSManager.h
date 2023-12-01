#pragma once
#include "common/GSMUtils.h"
#include "IGPRSHandler.h"
#include "GSMModemManager.h"
#include "command/ByteModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ULong2StringModemCMD.h"
#include "GPRSManager.h"

// Step 1
// AT+CGDCONT - Define PDP context
// AT+CGDCONT=1,"IP","APN"
constexpr char QUECTEL_DEFINE_PDP[] = "+CGDCONT";

// Step 2
// AT+CGAUTH - Set type of authentication for PDP-IP connections of GPRS
// AT+CGAUTH=1,1,”TEST”,”123”
constexpr char QUECTEL_AUTH_PDP[] = "+CGAUTH";

// Step 3
// AT+CGACT - Activate PDP profile
// AT+CGACT=1,1
constexpr char QUECTEL_ACTIVATE_PDP[] = "+CGACT";

// Step 4
// AT+CGPADDR - The write command returns a list of PDP addresses for the specified context identifiers
// AT+CGPADDR=1
constexpr char QUECTEL_GET_PDP_IP[] = "+CGPADDR";

// Step 5
// AT+NETOPEN is used to start service by activating PDP context. 
// You must execute AT+NETOPEN beforeany other TCP/UDP related operations.
constexpr char GSM_ACTIVATE_IP_CONTEXT_CMD[] = "+NETOPEN";

// TODO: AT+QIDNSGIP Get IP Address by Domain Name
// AT+CDNSGIP - Query the IP Address of Given Domain Name
constexpr char QUECTEL_RESOLVE_DNS_CMD[] = "+CDNSGIP"; // Get IP of dns name

// +CIPEVENT: NETWORK CLOSED UNEXPECTEDLY
constexpr char GSM_QUECTEL_NETWORK_CLOSE_EVENT[] = "+CIPEVENT";


class QuectelGPRSManager: public GPRSManager
{
private:

protected:

    void FlushAuthData() override;
    bool ResolveHostNameCMD(const char *hostName) override;
    bool ConnectInternal() override;

public:
    QuectelGPRSManager(GSMModemManager *gsmManager);
    virtual ~QuectelGPRSManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
};