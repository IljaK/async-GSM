#pragma once
#include "BaseModemCMD.h"

struct ULongModemCMD: public BaseModemCMD
{
    unsigned long valueData;

    ULongModemCMD(unsigned long valueData, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->valueData = valueData;
    }

    virtual ~ULongModemCMD() {}

    void WriteStream(Print *stream) override {
        stream->print(valueData, 10);
    }
};