#pragma once
#include "BaseStreamWriteModemCMD.h"
#include "array/ByteStackArray.h"

struct SocketStreamWriteModemCMD: public BaseStreamWriteModemCMD
{
    uint8_t socketId;
    ByteArray *data;

    SocketStreamWriteModemCMD(uint8_t socketId, ByteArray *data, const char *cmd, unsigned long timeout):BaseStreamWriteModemCMD(cmd, timeout) {
        this->socketId = socketId;
        this->data = data;
    }

    virtual ~SocketStreamWriteModemCMD() {
        delete data;
    }

    void WriteStream(Print *stream) override {
        // AT+CIPSEND=(0-9),(1-1500)

        stream->print(socketId);
        stream->print(',');
        stream->print(data->GetLength());
    }
    /*
    The <length> field may be empty.
    While it is empty, each <Ctrl+Z> character present in the data should be coded as <ETX><Ctrl+Z>.
    Each<ESC> character present in the data should be coded as <ETX><ESC>.
    Each <ETX> character will becoded as <ETX><ETX>. Single <Ctrl+Z> means end of the input data.
    Single <ESC> is used tocancel thesending.
    <ETX> is 0x03, and <Ctrl+Z> is 0x1A,<ESC> is 0x1B.
    */
    void WriteSteamContent(Stream *stream) {
        /*
        for (size_t i = 0; i < data->length; i++) {
            if (isExtraSign(data->array[i])) {
                stream->write(0x03);
            }
            stream->write(data->array[i]);
        } */
        stream->write(data->GetArray(), data->GetLength());
        EndStream(stream, false);
    }

    bool isExtraSign(uint8_t sign) {
        switch(sign) {
            case 0x03:
            case 0x1A:
            case 0x1B:
                return true;
        }
        return false;
    }

    char *ExtraTriggerValue() override {
        return (char*)">";
    }
};