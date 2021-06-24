#pragma once
#include "ULongModemCMD.h"

struct ULong2ModemCMD: public ULongModemCMD
{
    unsigned long valueData2;

    ULong2ModemCMD(unsigned long valueData, unsigned long valueData2, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ULongModemCMD(valueData, cmd, timeout) {
        this->valueData2 = valueData2;
    }

    virtual ~ULong2ModemCMD() {}

    void WriteStream(Print *stream) override {
        ULongModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print(valueData2, 10);
    }
};