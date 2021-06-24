#pragma once
#include "ByteModemCMD.h"

struct ByteShortModemCMD: public ByteModemCMD
{
    uint16_t data2;

    ByteShortModemCMD(uint8_t byteData, uint16_t data2, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):ByteModemCMD(byteData, cmd, timeout) {
        this->data2 = data2;
    }

    virtual ~ByteShortModemCMD() {}

    void WriteStream(Print *stream) override {
        ByteModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print(data2, 10);
    }
};