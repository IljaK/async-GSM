#pragma once
#include "BaseModemCMD.h"
#include "array/ByteStackArray.h"

struct BaseStreamWriteModemCMD: public BaseModemCMD
{
    BaseStreamWriteModemCMD(const char *cmd, unsigned long timeout):BaseModemCMD(cmd, timeout, false, true, false, false) {
        SetHasExtraTrigger(true);
    }

    virtual ~BaseStreamWriteModemCMD() {
    }

    void EndStream(Stream *modemStream, bool cancel) {
        if (modemStream == NULL) return;
        if (cancel) {
            modemStream->write(ESC_ASCII_SYMBOL);
            return;
        } 
        modemStream->write(CRTLZ_ASCII_SYMBOL);
    }

    char *ExtraTriggerValue() override {
        return (char*)"> ";
    }
};