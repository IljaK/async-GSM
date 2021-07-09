#pragma once
#include "BaseModemCMD.h"
#include "common/GSMUtils.h"

struct SockeCreateModemCMD: public BaseModemCMD
{
    uint8_t socketType;
    uint8_t socketId = 255;

    SockeCreateModemCMD(uint8_t socketType, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->socketType = socketType;
    }

    virtual ~SockeCreateModemCMD() {}

    void WriteStream(Print *stream) override {
        stream->print(socketType, 10);

        // %u,\"%u.%u.%u.%u\",%u"
    }
};