#pragma once
#include "BaseModemCMD.h"
#include "common/Util.h"

struct SMSSendModemCMD: public BaseModemCMD
{
    uint8_t customData = 0;
    char phoneNumber[20] = { 0 };

    SMSSendModemCMD(char *phoneNumber, const char *cmd, uint8_t customData = 0, unsigned long timeout = 5000000UL):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->customData = customData;
        strcpy(this->phoneNumber, phoneNumber);
        SetHasExtraTrigger(true);
    }

    virtual ~SMSSendModemCMD() {
    }

    void WriteStream(Print *stream) override {
        stream->print('"');
        stream->print(phoneNumber);
        stream->print('"');
    }

    char *ExtraTriggerValue() override {
        return (char*)"> ";
    }
};