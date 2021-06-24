#pragma once
#include "ByteModemCMD.h"

struct Byte2ModemCMD: public ByteModemCMD
{
    uint8_t byteData2;

    Byte2ModemCMD(uint8_t byteData, uint8_t byteData2, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):ByteModemCMD(byteData, cmd, timeout) {
        this->byteData2 = byteData2;
    }

    virtual ~Byte2ModemCMD() {}

    void WriteStream(Print *stream) override {
        ByteModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print(byteData2, 10);
    }
};