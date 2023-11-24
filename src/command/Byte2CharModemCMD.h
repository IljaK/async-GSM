#pragma once
#include "Byte2ModemCMD.h"

struct Byte2CharModemCMD: public Byte2ModemCMD
{
    char * data3 = NULL;

    Byte2CharModemCMD( uint8_t data1, uint8_t data2, const char * data3, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        Byte2ModemCMD(data1, data2, cmd, timeout) {
        this->data3 = makeNewCopy(data3);
    }

    virtual ~Byte2CharModemCMD() {
        if (data3 != NULL) {
            free(data3);
            data3 = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        Byte2ModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print('"');
        stream->print(data3);
        stream->print('"');
    }
};