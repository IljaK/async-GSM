#pragma once
#include "Byte2ModemCMD.h"

struct Byte3ModemCMD: public Byte2ModemCMD
{
    uint8_t byteData3;

    Byte3ModemCMD(uint8_t byteData, uint8_t byteData2, uint8_t byteData3, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):Byte2ModemCMD(byteData, byteData2, cmd, timeout) {
        this->byteData3 = byteData3;
    }

    virtual ~Byte3ModemCMD() {}

    void WriteStream(Print *stream) override {
        Byte2ModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print(byteData3, 10);
    }
};