#pragma once
#include "BaseModemCMD.h"
#include "array/ByteStackArray.h"

struct SocketWriteModemCMD: public BaseModemCMD
{
    uint8_t socketId;
    ByteArray *data;

    SocketWriteModemCMD(uint8_t socketId, ByteArray *data, const char *cmd, unsigned long timeout):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->socketId = socketId;
        this->data = data;
    }

    virtual ~SocketWriteModemCMD() {
        if (data->array != NULL) {
            free(data->array);
        }
        free(data);
    }

    void WriteStream(Print *stream) override {
        // AT+USOWR=3,12,"Hello world!"

        stream->print(socketId);
        stream->print(',');
        stream->print(data->length);
        stream->print(',');
        stream->print('"');

        char hexBuff[3];
        for (size_t i = 0; i < data->length; i++) {
            if (encodeToHex(data->array + i, 1, hexBuff) == 0) {
                break;
            }
            stream->print(hexBuff[0]);
            stream->print(hexBuff[1]);
        }
        
        stream->print('"');
        // TODO: Write hex data to stream
    }
};