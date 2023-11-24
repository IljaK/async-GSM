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
constexpr char SIMCOM_DEFINE_PDP[] = "+CGDCONT";

// Step 2
// AT+CGAUTH - Set type of authentication for PDP-IP connections of GPRS
// AT+CGAUTH=1,1,”TEST”,”123”
constexpr char SIMCOM_AUTH_PDP[] = "+CGAUTH";

// Step 3
// AT+CGACT - Activate PDP profile
// AT+CGACT=1,1
constexpr char SIMCOM_ACTIVATE_PDP[] = "+CGACT";

// Step 4
// AT+CGPADDR - The write command returns a list of PDP addresses for the specified context identifiers
// AT+CGPADDR=1
constexpr char SIMCOM_GET_PDP_IP[] = "+CGPADDR";

// AT+CDNSGIP - Query the IP Address of Given Domain Name
constexpr char SIMCOM_RESOLVE_DNS_CMD[] = "+CDNSGIP"; // Get IP of dns name


class SimComGPRSManager: public GPRSManager, public ITimerCallback
{
private:

protected:

    void FlushAuthData() override;
    bool ResolveHostNameCMD(const char *hostName) override;
    bool ConnectInternal() override;

public:
    SimComGPRSManager(GSMModemManager *gsmManager);
    virtual ~SimComGPRSManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;
};