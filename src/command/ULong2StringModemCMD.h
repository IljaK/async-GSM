#pragma once
#include "ULong2ModemCMD.h"

struct ULong2StringModemCMD: public ULong2ModemCMD
{
    char *valueData3;

    ULong2StringModemCMD(unsigned long valueData, unsigned long valueData2, char *valueData3, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ULong2ModemCMD(valueData, valueData2, cmd, timeout) {
            this->valueData3 = (char *)malloc(strlen(valueData3) + 1);
            strcpy(this->valueData3, valueData3);
    }

    virtual ~ULong2StringModemCMD() {
        if (valueData3 != NULL) {
            free(valueData3);
            valueData3 = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        ULong2ModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print('"');
        if (valueData3 != NULL) {
            stream->print(valueData3);
        }
        stream->print('"');
    }
};