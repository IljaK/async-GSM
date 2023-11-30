#pragma once
#include "BaseModemCMD.h"
#include "array/ByteStackArray.h"

struct SocketHEXWriteModemCMD: public BaseModemCMD
{
    uint8_t socketId;
    ByteArray *data;

    SocketHEXWriteModemCMD(uint8_t socketId, ByteArray *data, const char *cmd, unsigned long timeout):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->socketId = socketId;
        this->data = data;
    }

    virtual ~SocketHEXWriteModemCMD() {
        delete data;
    }

    void WriteStream(Print *stream) override {
        // AT+USOWR=3,12,"Hello world!"

        stream->print(socketId);
        stream->print(',');
        stream->print(data->GetLength());
        stream->print(',');
        stream->print('"');

        char hexBuff[3];
        for (size_t i = 0; i < data->GetLength(); i++) {
            if (encodeToHex(data->GetArrayAt(i), 1, hexBuff) == 0) {
                break;
            }
            stream->print(hexBuff[0]);
            stream->print(hexBuff[1]);
        }
        
        stream->print('"');
    }
};