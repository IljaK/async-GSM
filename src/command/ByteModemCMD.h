#pragma once
#include "BaseModemCMD.h"

struct ByteModemCMD: public BaseModemCMD
{
    uint8_t byteData;

    ByteModemCMD(uint8_t byteData, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->byteData = byteData;
    }

    virtual ~ByteModemCMD() {}

    void WriteStream(Print *stream) override {
        stream->print(byteData, 10);
    }
};