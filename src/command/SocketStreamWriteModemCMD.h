#pragma once
#include "BaseStreamWriteModemCMD.h"
#include "array/ByteStackArray.h"

struct SocketStreamWriteModemCMD: public BaseStreamWriteModemCMD
{
    uint8_t socketId;
    ByteArray *data;
    char *trigger = NULL;

    SocketStreamWriteModemCMD(uint8_t socketId, ByteArray *data, const char *cmd, unsigned long timeout):BaseStreamWriteModemCMD(cmd, timeout) {
        this->socketId = socketId;
        this->data = data;
    }

    SocketStreamWriteModemCMD(char *trigger, uint8_t socketId, ByteArray *data, const char *cmd, unsigned long timeout):BaseStreamWriteModemCMD(cmd, timeout) {
        this->socketId = socketId;
        this->data = data;
        this->trigger = makeNewCopy(trigger);
    }

    virtual ~SocketStreamWriteModemCMD() {
        delete data;
        if (this->trigger != NULL) {
            free(this->trigger);
        }
    }

    void WriteStream(Print *stream) override {
        // AT+CIPSEND=(0-9),(1-1500)
        // AT+QISEND=(0-11),(1-1460)

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
        if (this->trigger == NULL) {
            return (char*)">";
        }
        return this->trigger;
    }
};