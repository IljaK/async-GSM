#pragma once
#include "BaseModemCMD.h"
#include "common/GSMUtils.h"

struct SocketConnectCMD: public BaseModemCMD
{
    IPAddr ip;
    uint8_t socketId;
    uint16_t port;

    SocketConnectCMD(uint8_t socketId, IPAddr ip, uint16_t port, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->socketId = socketId;
        this->ip = ip;
        this->port = port;
    }

    virtual ~SocketConnectCMD() {}

    void WriteStream(Print *stream) override {

        // %u,\"%u.%u.%u.%u\",%u"

        stream->print(socketId, 10);
        stream->print(',');
        stream->print('"');
        stream->print(ip.octet[0], 10);
        stream->print('.');
        stream->print(ip.octet[1], 10);
        stream->print('.');
        stream->print(ip.octet[2], 10);
        stream->print('.');
        stream->print(ip.octet[3], 10);
        stream->print('"');
        stream->print(',');
        stream->print(port, 10);
    }
};