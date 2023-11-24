#pragma once
#include "Byte2CharModemCMD.h"

struct Byte2Char2ModemCMD: public Byte2CharModemCMD
{
    char * data4 = NULL;

    Byte2Char2ModemCMD( uint8_t data1, uint8_t data2, const char * data3, const char * data4, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        Byte2CharModemCMD(data1, data2, data3, cmd, timeout) {
        this->data4 = makeNewCopy(data4);
    }

    virtual ~Byte2Char2ModemCMD() {
        if (data4 != NULL) {
            free(data4);
            data4 = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        Byte2CharModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print('"');
        stream->print(data4);
        stream->print('"');
    }
};