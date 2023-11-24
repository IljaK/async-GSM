#pragma once
#include "ByteModemCMD.h"

struct ByteChar2ModemCMD: public ByteModemCMD
{
    char * data2 = NULL;
    char * data3 = NULL;

    ByteChar2ModemCMD( uint8_t data1, const char * data2, const char * data3, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ByteModemCMD(data1, cmd, timeout) {
        this->data2 = makeNewCopy(data2);
        this->data3 = makeNewCopy(data3);
    }

    virtual ~ByteChar2ModemCMD() {
        if (data2 != NULL) {
            free(data2);
            data2 = NULL;
        }
        if (data3 != NULL) {
            free(data3);
            data3 = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        ByteModemCMD::WriteStream(stream);
        stream->print(',');
        stream->print('"');
        stream->print(data2);
        stream->print('"');
        stream->print(',');
        stream->print('"');
        stream->print(data3);
        stream->print('"');
    }
};