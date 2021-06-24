#pragma once
#include "ULong2ModemCMD.h"

struct ULong4ModemCMD: public ULong2ModemCMD
{
    unsigned long valueData3;
    unsigned long valueData4;

    ULong4ModemCMD(unsigned long valueData, unsigned long valueData2, unsigned long valueData3, unsigned long valueData4, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ULong2ModemCMD(valueData, valueData2, cmd, timeout) {
        this->valueData3 = valueData3;
        this->valueData4 = valueData4;
    }

    virtual ~ULong4ModemCMD() {}

    void WriteStream(Print *stream) override {
        ULong2ModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print(valueData3, 10);
        stream->print(',');
        stream->print(valueData4, 10);
    }
};