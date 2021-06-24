#pragma once
#include "BaseModemCMD.h"

struct CharModemCMD: public BaseModemCMD
{
    const char * charData;

    CharModemCMD(const char * charData, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT, bool dataQuotations = false, bool semicolon = false):
        BaseModemCMD(cmd, timeout, false, true, dataQuotations, semicolon),
        charData(charData) {
    }

    virtual ~CharModemCMD() {
    }

    void WriteStream(Print *stream) override {
        stream->print(charData);
    }
};