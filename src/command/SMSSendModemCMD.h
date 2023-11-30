#pragma once
#include "BaseStreamWriteModemCMD.h"
#include "common/Util.h"

struct SMSSendModemCMD: public BaseStreamWriteModemCMD
{
    uint8_t customData = 0;
    char phoneNumber[MAX_PHONE_LENGTH] = { 0 };

    SMSSendModemCMD(char *phoneNumber, const char *cmd, uint8_t customData = 0, unsigned long timeout = 5000000UL):BaseStreamWriteModemCMD(cmd, timeout) {
        this->customData = customData;
        strncpy(this->phoneNumber, phoneNumber, MAX_PHONE_LENGTH);
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