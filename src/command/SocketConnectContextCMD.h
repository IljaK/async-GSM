#pragma once
#include "SocketConnectCMD2.h"
#include "common/GSMUtils.h"

struct SocketConnectContextCMD: public SocketConnectCMD2
{
    uint8_t contextId = 0;
    uint8_t accessMode = 0;

    SocketConnectContextCMD(uint8_t accessMode, uint8_t contextId, uint8_t socketId, char *protocolType, IPAddr ip, uint16_t port, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):SocketConnectCMD2(socketId, protocolType, ip, port, cmd, timeout) {
        this->contextId = contextId;
    }

    virtual ~SocketConnectContextCMD() {
        
    }

    void WriteStream(Print *stream) override {
        stream->print(contextId, 10);
        stream->print(',');
        SocketConnectCMD2::WriteStream(stream);
        stream->print(',');
        stream->print(',');
        stream->print(accessMode, 10);
    }
};