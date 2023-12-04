#pragma once
#include "common/TimeUtil.h"

#define MAX_PHONE_LENGTH 20

// Provides the event status:
enum GSM_THRESHOLD_STATE {
    GSM_THRESHOLD_T_MINUS_2 = -2, // -2: temperature below t-2 threshold
    GSM_THRESHOLD_T_MINUS_1 = -1, // -1: temperature below t-1 threshold
    GSM_THRESHOLD_T = 0, // 0: temperature inside the allowed range - not close to the limits
    GSM_THRESHOLD_T_PLUS_1 = 1, // 1: temperature above t+1 threshold
    GSM_THRESHOLD_T_PLUS_2 = 2, // 2: temperature above the t+2 threshold

    GSM_THRESHOLD_SHUTDOWN_AFTER_CALL = 10, // 10: timer expired and no emergency call is in progress, shutdown phase started
    GSM_THRESHOLD_SHUTDOWN = 20, // 20: emergency call ended, shutdown phase started
    GSM_THRESHOLD_ERROR = 100, // 100: error during measurement
};

enum GSM_REG_STATE {
    GSM_REG_STATE_CONNECTING_HOME, // not registered, the MT is not currently searching a new operator to register to
    GSM_REG_STATE_CONNECTED_HOME, // registered, home network
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
    GSM_NETWORK_UNKNOWN,
    GSM_NETWORK_2G_GSM,
    GSM_NETWORK_2G_GPRS,
    GSM_NETWORK_2G_EDGE,
    GSM_NETWORK_3G_WCDMA,
    GSM_NETWORK_3G_HSDPA, // only(WCDMA)
    GSM_NETWORK_3G_HSUPA, // only(WCDMA)
    GSM_NETWORK_3G_HSDPA_HSUPA, // (HSDPA and HSUPA, WCDMA)
    GSM_NETWORK_4G_LTE, // 7: registered for PS service, RAT=4G
    GSM_NETWORK_OTHER
};

enum GSM_INIT_STATE {
    GSM_STATE_NONE,
    GSM_STATE_PIN,
    GSM_STATE_WAIT_CREG,
    GSM_STATE_READY,
    GSM_STATE_ERROR
};

enum GSM_FAIL_STATE {
    GSM_FAIL_UNKNOWN,
    GSM_FAIL_OTHER_PIN,
    GSM_FAIL_REG_NETWORK
};

struct GSMNetworkStats {
    GSM_REG_STATE regState = GSM_REG_STATE_UNKNOWN;
    GSM_NETWORK_TYPE networkType = GSM_NETWORK_UNKNOWN;
    GSM_THRESHOLD_STATE thresholdState = GSM_THRESHOLD_T;
    uint8_t signalStrength = 0;
    uint8_t signalQuality = 0;
    float temperature = 0;
};

struct IncomingSMSInfo {
    char sender[MAX_PHONE_LENGTH] = { 0 };
    timez_t timeStamp;
};

class IGSMNetworkManager {
public:
    virtual void OnGSMSimUnlocked() = 0;
    virtual void OnGSMFailed(GSM_FAIL_STATE failState) = 0;
    virtual void OnGSMConnected() = 0;
    virtual void OnGSMStatus(GSM_REG_STATE state) = 0;
    virtual void OnGSMQuality(uint8_t strength, uint8_t quality) = 0;
    virtual void OnGSMNetworkType(GSM_NETWORK_TYPE type) = 0;
    virtual void OnGSMThreshold(GSM_THRESHOLD_STATE type) = 0;
    virtual bool OnSMSSendStream(Print *smsStream, char *phoneNumber, uint8_t customData) = 0;
    virtual void OnSMSReceive(IncomingSMSInfo *smsInfo, char *message, size_t messageLen) = 0;
};
