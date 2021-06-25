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

struct PinModemCMD: public BaseModemCMD
{
    GSM_PIN_STATE_CMD pinState = GSM_PIN_STATE_UNKNOWN;

    PinModemCMD(const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        
    }

    virtual ~PinModemCMD() {}

    void HandleDataContent(char *response, size_t respLen) {
        size_t cmdLen = strlen(cmd);
        if (respLen <= cmdLen + 2) {
            return;
        }
        response += strlen(cmd) + 2;

        if (String(F("READY")).equals(response)) {
            pinState = GSM_PIN_STATE_READY;
        } else if (String(F("SIM PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_SIM_PIN;
        } else  if (String(F("SIM PUK")).equals(response)) {
            pinState = GSM_PIN_STATE_SIM_PUK;
        } else  if (String(F("SIM PIN2")).equals(response)) {
            pinState = GSM_PIN_STATE_SIM_PIN2;
        } else  if (String(F("SIM PUK2")).equals(response)) {
            pinState = GSM_PIN_STATE_SIM_PUK2;
        } else  if (String(F("PH-NET PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_PH_NET_PIN;
        } else  if (String(F("PH-NETSUB PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_PH_NETSUB_PIN;
        } else  if (String(F("PH-SP PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_PH_SP_PIN;
        } else if (String(F("PH-CORP PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_PH_CORP_PIN;
        } else if (String(F("PH-SIM PIN")).equals(response)) {
            pinState = GSM_PIN_STATE_PH_SIM_PIN;
        }
    }

    void WriteStream(Print *stream) override {
        BaseModemCMD::WriteStream(stream);
    }
};