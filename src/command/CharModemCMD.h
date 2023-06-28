#pragma once
#include "BaseModemCMD.h"

struct CharModemCMD: public BaseModemCMD
{
    char * charData = NULL;

    CharModemCMD(const char * charData, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT, bool dataQuotations = false, bool semicolon = false):
        BaseModemCMD(cmd, timeout, false, true, dataQuotations, semicolon) {
        this->charData = makeNewCopy(charData);
    }

    virtual ~CharModemCMD() {
        if (charData != NULL) {
            free(charData);
            charData = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        stream->print(charData);
    }
};