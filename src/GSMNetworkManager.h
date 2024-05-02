#pragma once
#include "GSMCallManager.h"
#include "IGSMNetworkManager.h"
#include "command/PinModemCMD.h"
#include "common/TimeUtil.h"
#include "common/Timer.h"

constexpr unsigned long QUALITY_CHECK_DURATION = 20000ul;

constexpr char GSM_SIM_PIN_CMD[] = "+CPIN";
constexpr char GSM_SIM_PIN_READY[] = "READY"; // Ready sim card state
constexpr char GSM_SIM_PIN_REQUIRE[] = "SIM PIN"; // Pin code input sim card state


constexpr char GSM_CMD_NETWORK_REG[] = "+CREG"; // AT+CREG=1 // PIN required: Yes

// Forces an attempt to select and register with the GSM/UMTS/LTE network operator
constexpr char GSM_CMD_OPERATOR_CMD[] = "+COPS"; // AT+CREG=1 // PIN required: Yes

constexpr char GSM_CMD_SMS_FROMAT[] = "+CMGF"; // "AT+CMGF=1" // Incomming message format: 0 - PDU, 1 - text // PIN required: Yes
constexpr char GSM_CMD_SMS_INDICATION[] = "+CNMI"; // AT+CNMI=2,2,0,0,0 // Message event format

constexpr char GSM_CMD_TIME_ZONE[] = "+CTZU"; // "AT+CTZU=1" // Automatic time/time zone update // PIN required: Yes

constexpr char GSM_SMS_EVENT[] = "+CMT"; // Data event
constexpr char GSM_CMD_SMS_SEND[] = "+CMGS"; // "Send sms"

constexpr char GSM_CMD_NETWORK_QUALITY[] = "+CSQ"; // Signal quality
constexpr char GSM_CMD_TIME[] = "+CCLK";

constexpr unsigned long GSM_NETWORG_REG_TIMEOUT = 180ul;
constexpr unsigned long GSM_UNKNOWN_NETWORK_TIMEOUT = 30ul;
constexpr unsigned long GSM_NETWORG_REG_ERROR_TIMEOUT = 10000ul;
constexpr unsigned long GSM_NETWORG_CREG_INTERVAL = 5000ul;

enum GSM_MODEM_CONFIGURATION_STEP: uint8_t {
    GSM_MODEM_CONFIGURATION_STEP_NONE = 0,
    GSM_MODEM_CONFIGURATION_STEP_CNMI,
    GSM_MODEM_CONFIGURATION_STEP_CMFG,
    GSM_MODEM_CONFIGURATION_STEP_CTZU,

    // Reset network registration
    // GSM_MODEM_CONFIGURATION_STEP_COPS_OFF,
    // GSM_MODEM_CONFIGURATION_STEP_COPS_ON,

    GSM_MODEM_CONFIGURATION_STEP_COMPLETE
};

class GSMNetworkManager: public IBaseGSMHandler, public ITimerCallback
{
private:
    unsigned long syncTS = 0;

    const char *simPin;
    GSM_INIT_STATE initState = GSM_STATE_NONE;
    GSMNetworkStats gsmStats;

    bool isSimUnlocked = false;
    Timer gsmReconnectTimer;
    Timer gsmSimTimer;
    Timer gsmNetStatsTimer;
    Timer gsmCREGTimer;
    Timer gsmUnknownNetworkTimer;

    IGSMNetworkManager *listener = NULL;
    IncomingSMSInfo *incomingSms = NULL;

    uint8_t configurationStep = GSM_MODEM_CONFIGURATION_STEP_NONE;

    void StopTimers();

    void HandleGSMFail(GSM_FAIL_STATE failState);
    inline void FlushIncomingSMS();
    inline void HandleSimUnlocked();
    inline void UpdateCregResult(GSM_REG_STATE state);

protected:
    timez_t currentTime;
    GSMModemManager *modemManager;
    GSM_INIT_STATE GetInitState();
    GSM_MODEM_CONFIGURATION_STEP GetConfigurationStep();

    virtual void FetchModemStats();
    inline GSM_REG_STATE GetCregState(char * data, size_t dataLen);

    void UpdateThresoldState(GSM_THRESHOLD_STATE state);
    void UpdateNetworkType(GSM_NETWORK_TYPE type);
    void UpdateTemperature(float temp);

    virtual void NextConfigurationStep();

    virtual void ContinueConfigureModem();
    void ConfigureModemCompleted();
    virtual void OnTimeUpdated();

public:
    GSMNetworkManager(GSMModemManager *gsmManager);
    virtual ~GSMNetworkManager();

    bool InitSendSMS(char *phone, uint8_t customData = 0);

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

	void OnTimerComplete(Timer *timer) override;

    void Connect(const char *simPin);
    void SetGSMListener(IGSMNetworkManager *listener);

    bool IsInitialized();
    bool IsSimUnlocked();

    time_t GetUTCTime();
    time_t GetLocalTime();

    GSMNetworkStats GetGSMStats();

    virtual void OnModemReboot();

    void UpdateSignalQuality(int rssi, int ber);
};