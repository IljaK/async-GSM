#pragma once
#include "BaseModemCMD.h"
#include <cstring>

enum GSM_PIN_STATE_CMD {
    GSM_PIN_STATE_UNKNOWN,
    GSM_PIN_STATE_READY,
    GSM_PIN_STATE_SIM_PIN,
    GSM_PIN_STATE_SIM_PUK,
    GSM_PIN_STATE_SIM_PIN2,
    GSM_PIN_STATE_SIM_PUK2,
    GSM_PIN_STATE_PH_NET_PIN,
    GSM_PIN_STATE_PH_NETSUB_PIN,
    GSM_PIN_STATE_PH_SP_PIN,
    GSM_PIN_STATE_PH_CORP_PIN,
    GSM_PIN_STATE_PH_SIM_PIN,
};

constexpr const char SIM_PIN_STATE_1[] PROGMEM = "READY";
constexpr const char SIM_PIN_STATE_2[] PROGMEM = "SIM PIN";
constexpr const char SIM_PIN_STATE_3[] PROGMEM = "SIM PUK";
constexpr const char SIM_PIN_STATE_4[] PROGMEM = "SIM PIN2";
constexpr const char SIM_PIN_STATE_5[] PROGMEM = "SIM PUK2";
constexpr const char SIM_PIN_STATE_6[] PROGMEM = "PH-NET PIN";
constexpr const char SIM_PIN_STATE_7[] PROGMEM = "PH-NETSUB PIN";
constexpr const char SIM_PIN_STATE_8[] PROGMEM = "PH-SP PIN";
constexpr const char SIM_PIN_STATE_9[] PROGMEM = "PH-CORP PIN";
constexpr const char SIM_PIN_STATE_10[] PROGMEM = "PH-SIM PIN";

constexpr const char *SIM_PIN_STATE_MESSAGES [GSM_PIN_STATE_PH_SIM_PIN] PROGMEM = {
    SIM_PIN_STATE_1,
    SIM_PIN_STATE_2,
    SIM_PIN_STATE_3,
    SIM_PIN_STATE_4,
    SIM_PIN_STATE_5,
    SIM_PIN_STATE_6,
    SIM_PIN_STATE_7,
    SIM_PIN_STATE_8,
    SIM_PIN_STATE_9,
    SIM_PIN_STATE_10,
};

struct PinStatusModemCMD: public BaseModemCMD
{
    GSM_PIN_STATE_CMD pinState = GSM_PIN_STATE_UNKNOWN;

    PinStatusModemCMD(const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, true, false, false, false) {
        
    }

    virtual ~PinStatusModemCMD() {}

    void HandleDataContent(char *response, size_t respLen) {
        if (respLen <= cmdLen + 2) {
            return;
        }
        response += strlen(cmd) + 2;

        for (uint8_t i = 0; i < GSM_PIN_STATE_PH_SIM_PIN; i++) {
            if (strcmp_P(response, SIM_PIN_STATE_MESSAGES[i]) == 0) {
                pinState = (GSM_PIN_STATE_CMD)(i + 1);
                break;
            }
        }
    }

    void WriteStream(Print *stream) override {
        BaseModemCMD::WriteStream(stream);
    }
};