#pragma once
#include "SocketConnectCMD.h"
#include "common/GSMUtils.h"

struct SocketConnectCMD2: public SocketConnectCMD
{
    char * protocolType = NULL;

    SocketConnectCMD2(uint8_t socketId, char *protocolType, IPAddr ip, uint16_t port, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):SocketConnectCMD(socketId, ip, port, cmd, timeout) {
        this->protocolType = makeNewCopy(protocolType);
    }

    virtual ~SocketConnectCMD2() {
        if (protocolType != NULL) {
            free(protocolType);
            protocolType = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        stream->print(socketId, 10);
        stream->print(',');
        stream->print('"');
        stream->print(protocolType);
        stream->print('"');
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