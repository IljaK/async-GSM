#pragma once
#include "CharModemCMD.h"
#include "common/GSMUtils.h"

struct IPResolveModemCMD: public CharModemCMD
{
    IPAddr ipAddr;

    IPResolveModemCMD(const char * charData, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        CharModemCMD(charData, cmd, timeout, true) {
        ipAddr.dword = 0;
    }

    virtual ~IPResolveModemCMD() {
    }

    void WriteStream(Print *stream) override {
        CharModemCMD::WriteStream(stream);
    }
};