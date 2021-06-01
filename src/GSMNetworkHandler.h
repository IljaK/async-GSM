#pragma once
#include "GSMCallHandler.h"
#include "common/Util.h"

constexpr unsigned long REQUEST_DELAY_DURATION = 100000ul;
constexpr unsigned long QUALITY_CHECK_DURATION = 20000000ul;

constexpr char GSM_SIM_PIN_CMD[] = "+CPIN";
constexpr char GSM_SIM_PIN_READY[] = "READY"; // Ready sim card state
constexpr char GSM_SIM_PIN_REQUIRE[] = "SIM PIN"; // Pin code input sim card state

constexpr char GSM_CMD_NETWORK_REG[] = "+CREG"; // "AT+CREG=1"
constexpr char GSM_CMD_MSG_FROMAT[] = "+CMGF"; // "AT+CMGF=1"
constexpr char GSM_CMD_HEX_MODE[] = "+UDCONF"; // "AT+UDCONF=1,1"
constexpr char GSM_CMD_TIME_ZONE[] = "+CTZU"; // "AT+CTZU=1"
constexpr char GSM_CMD_DTFM[] = "+UDTMFD"; // "AT+UDTMFD=1,2"
constexpr char GSM_CMD_CALL_STATUS[] = "+UCALLSTAT"; // "AT+UCALLSTAT=1"

constexpr char GSM_CMD_NETWORK_QUALITY[] = "+CSQ";
constexpr char GSM_CMD_TIME[] = "+CCLK";

enum GSM_REG_STATE {
    GSM_REG_STATE_CONNECTING_HOME, // not registered, the MT is not currently searching a new operator to register to
    GSM_REG_CONNECTED_HOME, // registered, home network
    GSM_REG_STATE_CONNECTING_OTHER, // not registered, but the MT is currently searching a new operator to register to
    GSM_REG_STATE_DENIED, // registration denied
    GSM_REG_STATE_UNKNOWN, 
    GSM_REG_STATE_CONNECTED_ROAMING, // registered, roaming
    GSM_REG_STATE_CONNECTED_SMS_ONLY_HOME, // registered for "SMS only", home network (applicable only when <AcTStatus> indicates E-UTRAN)
    GSM_REG_STATE_CONNECTED_SMS_ONLY_ROAMING, // registered for "SMS only", home network (applicable only when <AcTStatus> indicates E-UTRAN)
    GSM_REG_STATE_CONNECTED_EMERGENSY_ONLY, // attached for emergency bearer services only (see 3GPP TS 24.008 [67] and 3GPP TS 24.301 [102] that specify the condition when the MS is considered as attached for emergency bearer services)
    GSM_REG_STATE_CONNECTED_CSFB_NOT_HOME, // registered for "CSFB not preferred", home network (applicable only when <AcTStatus> indicates E-UTRAN)
    GSM_REG_STATE_CONNECTED_CSFB_NOT_ROAMING, // registered for "CSFB not preferred", roaming (applicable only when <AcTStatus> indicates E-UTRAN)
};

enum GSM_INIT_STATE {
    GSM_STATE_ERROR,

    GSM_STATE_CHECK_SIM,
    GSM_STATE_ENTER_SIM,
    GSM_STATE_AUTO_NETWOR_STATE,
    GSM_STATE_PREFERRED_MESSAGE_FORMAT,
    GSM_STATE_HEX_MODE,
    GSM_STATE_AUTOMATIC_TIME_ZONE,
    GSM_STATE_DTMF_DETECTION,
    GSM_STATE_WAIT_REG_NETWORK,
    GSM_STATE_CALL_STATUS,
    GSM_STATE_DONE
};

class IGSMNetworkHandler {
public:
    virtual void OnGSMSimUnlocked() = 0;
    virtual void OnGSMFailed(bool isPinError) = 0;
    virtual void OnGSMConnected() = 0;
    virtual void OnGSMStatus(GSM_REG_STATE state) = 0;
    virtual void OnGSMQuality(uint8_t strength, uint8_t quality) = 0;
};

class GSMNetworkHandler: public GSMCallHandler
{
private:
    time_t currentTime = 0;
    unsigned long syncTS = 0;
    int timeZone = 0;

    const char *simPin;
    GSM_REG_STATE regState = GSM_REG_STATE_UNKNOWN;
    GSM_INIT_STATE initState = GSM_STATE_CHECK_SIM;
    bool isSimUnlocked = false;
    TimerID delayedRequest = 0;

    IGSMNetworkHandler *listener = NULL;

    uint8_t signalStrength = 0;
    uint8_t signalQuality = 0;

    void TriggerCommand();
    void IncreaseState();
    void FetchTime();

public:
    GSMNetworkHandler(BaseGSMHandler *gsmHandler);
    virtual ~GSMNetworkHandler();

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    void Connect(const char *simPin);
    void SetGSMListener(IGSMNetworkHandler *listener);

    bool IsInitialized();
    bool IsSimUnlocked();

    uint8_t GetSignalStrength();
    uint8_t GetSignalQuality();

    unsigned long GetUTCTime();
    unsigned long GetLocalTime();
};