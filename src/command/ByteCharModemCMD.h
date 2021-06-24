#pragma once
#include "CharModemCMD.h"

struct ByteCharModemCMD: public CharModemCMD
{
    uint8_t data;

    ByteCharModemCMD(uint8_t data, const char * charData2, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        CharModemCMD(charData2, cmd, timeout, false, false) {
            this->data = data;
    }

    virtual ~ByteCharModemCMD() {
    }

    void WriteStream(Print *stream) override {
        stream->print(data);
        stream->print(',');
        CharModemCMD::WriteStream(stream);
    }
};