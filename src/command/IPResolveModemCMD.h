#pragma once
#include "ByteCharModemCMD.h"
#include "common/GSMUtils.h"

struct IPResolveModemCMD: public ByteCharModemCMD
{
    IPAddr ipAddr;

    IPResolveModemCMD(uint8_t data, const char * charData2, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ByteCharModemCMD(data, charData2, cmd, timeout) {
        ipAddr.dword = 0;
    }

    virtual ~IPResolveModemCMD() {
    }

    void WriteStream(Print *stream) override {
        ByteCharModemCMD::WriteStream(stream);
    }
};