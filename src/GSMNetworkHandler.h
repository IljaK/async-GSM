#pragma once
#include "GSMCallHandler.h"
#include "command/PinModemCMD.h"

constexpr unsigned long REQUEST_DELAY_DURATION = 100000ul;
constexpr unsigned long QUALITY_CHECK_DURATION = 20000000ul;

constexpr char GSM_SIM_PIN_CMD[] = "+CPIN";
constexpr char GSM_SIM_PIN_READY[] = "READY"; // Ready sim card state
constexpr char GSM_SIM_PIN_REQUIRE[] = "SIM PIN"; // Pin code input sim card state

// Enables / disables the smart temperature mode:
// 0 (default value and factory-programmed value): smart temperature feature disabled
// 1: smart temperature feature enabled; the indication by means of the +UUSTS URC and shutting down (if needed) are performed
// 2: smart temperature indication enabled; the +UUSTS URC will be issued to notify the Smart Temperature Supervisor status
// 3: smart temperature feature enabled with no indication; the shutdown (if needed) is performed, but without a URC notification 
// Allowed values:
// TOBY-L4 - 1, 3 (default value and factory-programmed value)
// TOBY-L2 / MPCI-L2 / LARA-R2 / TOBY-R2 / SARA-U2 / LISA-U2 / LISA-U1 /
// SARA-G4 / SARA-G3 / LEON-G1 - 0 (default value and factory-programmed value), 1, 2
constexpr char GSM_TEMP_THRESOLD_CMD[] = "+USTS"; 
constexpr char GSM_TEMP_THRESOLD_EVENT[] = "+UUSTS";
constexpr char GSM_TEMP_CMD[] = "+UTEMP";

// Reports the network or the device PS (Packet Switched) radio capabilities.
constexpr char GSM_NETWORK_TYPE_STATUS[] = "+UREG"; // AT+UCREG=1
constexpr char GSM_CMD_NETWORK_REG[] = "+CREG"; // AT+CREG=1
constexpr char GSM_CMD_CALL_STATUS[] = "+UCALLSTAT"; // AT+UCALLSTAT=1
constexpr char GSM_CMD_HEX_MODE[] = "+UDCONF"; // AT+UDCONF=1,1
constexpr char GSM_CMD_DTFM[] = "+UDTMFD"; // AT+UDTMFD=1,2

constexpr char GSM_CMD_SMS_FROMAT[] = "+CMGF"; // "AT+CMGF=1" // Incomming message format: 0 - PDU, 1 - text
constexpr char GSM_CMD_SMS_INDICATION[] = "+CNMI"; // AT+CNMI=2,2,0,0,0 // Message event format

constexpr char GSM_CMD_TIME_ZONE[] = "+CTZU"; // "AT+CTZU=1"

constexpr char GSM_SMS_EVENT[] = "+CMT"; // Data event
constexpr char GSM_CMD_SMS_SEND[] = "+CMGS"; // "Send sms"

constexpr char GSM_CMD_NETWORK_QUALITY[] = "+CSQ";
constexpr char GSM_CMD_TIME[] = "+CCLK";

// Provides the event status:
enum GSM_THRESOLD_STATE {
    GSM_THRESOLD_T_MINUS_2 = -2, // -2: temperature below t-2 threshold
    GSM_THRESOLD_T_MINUS_1 = -1, // -1: temperature below t-1 threshold
    GSM_THRESOLD_T = 0, // 0: temperature inside the allowed range - not close to the limits
    GSM_THRESOLD_T_PLUS_1 = 1, // 1: temperature above t+1 threshold
    GSM_THRESOLD_T_PLUS_2 = 2, // 2: temperature above the t+2 threshold

    GSM_THRESOLD_SHUTDOWN_AFTER_CALL = 10, // 10: timer expired and no emergency call is in progress, shutdown phase started
    GSM_THRESOLD_SHUTDOWN = 20, // 20: emergency call ended, shutdown phase started
    GSM_THRESOLD_ERROR = 100, // 100: error during measurement
};

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

enum GSM_NETWORK_TYPE {
    GSM_NETWORK_UNKNOWN, // 0: not registered for PS service
    GSM_NETWORK_2G_GPRS, // 1: registered for PS service, RAT=2G, GPRS available
    GSM_NETWORK_2G_EDGE, // 2: registered for PS service, RAT=2G, EDGE available
    GSM_NETWORK_3G_WCDMA, // 3: registered for PS service, RAT=3G, WCDMA available
    GSM_NETWORK_3G_HSDPA, // 4: registered for PS service, RAT=3G, HSDPA available
    GSM_NETWORK_3G_HSUPA, // 5: registered for PS service, RAT=3G, HSUPA available
    GSM_NETWORK_3G_HSDPA_HSUPA, // 6: registered for PS service, RAT=3G, HSDPA and HSUPA available
    GSM_NETWORK_4G, // 7: registered for PS service, RAT=4G
    GSM_NETWORK_2G_GPRS_DTM, // 8: registered for PS service, RAT=2G, GPRS available, DTM available
    GSM_NETWORK_2G_EDGE_DTM, // 9: registered for PS service, RAT=2G, EDGE available, DTM available
};

enum GSM_INIT_STATE {
    GSM_STATE_NONE,
    GSM_STATE_PIN,
    GSM_STATE_CONFIG,
    GSM_STATE_WAIT_CREG,
    GSM_STATE_READY,
    GSM_STATE_ERROR
};

struct IncomingSMSData {
    char *sender = NULL;
    uint32_t utcTime = 0;
    uint32_t timeZone = 0;
    char *message = NULL;
};

struct GSMNetworkStats {
    GSM_REG_STATE regState = GSM_REG_STATE_UNKNOWN;
    GSM_NETWORK_TYPE networkType = GSM_NETWORK_UNKNOWN;
    GSM_THRESOLD_STATE thresoldState = GSM_THRESOLD_T;
    uint8_t signalStrength = 0;
    uint8_t signalQuality = 0;
};

class IGSMNetworkHandler {
public:
    virtual void OnGSMSimUnlocked() = 0;
    virtual void OnGSMFailed(GSM_PIN_STATE_CMD pinState) = 0;
    virtual void OnGSMConnected() = 0;
    virtual void OnGSMStatus(GSM_REG_STATE state) = 0;
    virtual void OnGSMQuality(uint8_t strength, uint8_t quality) = 0;
    virtual void OnGSMNetworkType(GSM_NETWORK_TYPE type) = 0;
    virtual void OnGSMThresold(GSM_THRESOLD_STATE type) = 0;
    virtual bool OnSMSSendStream(Print *smsStream) = 0;
    virtual void OnSMSReceive(IncomingSMSData *smsData) = 0;
};

class GSMNetworkHandler: public GSMCallHandler
{
private:
    time_t currentTime = 0;
    unsigned long syncTS = 0;
    int timeZone = 0;

    const char *simPin;
    GSM_INIT_STATE initState = GSM_STATE_NONE;
    GSMNetworkStats gsmStats;

    bool isSimUnlocked = false;
    TimerID delayedRequest = 0;

    IGSMNetworkHandler *listener = NULL;
    IncomingSMSData *incomingSms = NULL;

    void TriggerCommand();
    void IncreaseState();
    void FetchTime();

public:
    GSMNetworkHandler(BaseGSMHandler *gsmHandler);
    virtual ~GSMNetworkHandler();

    bool SendSMS(char *phone);

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    void Connect(const char *simPin);
    void SetGSMListener(IGSMNetworkHandler *listener);

    bool IsInitialized();
    bool IsSimUnlocked();

    unsigned long GetUTCTime();
    unsigned long GetLocalTime();

    GSMNetworkStats *GetGSMStats();

    void OnModemReboot() override;
};