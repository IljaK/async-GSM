#pragma once
#include "GSMSocket.h"

class GSMSocket;

class IGSMSocketHandler {
public:
    virtual void OnSocketCreateError() = 0;

    virtual void OnSocketOpen(GSMSocket * socket) = 0;
    virtual void OnSocketClose(uint8_t sockedId, bool isSuccess) = 0;

    virtual void OnSocketConnected(GSMSocket * socket, int error) = 0;
    virtual void OnSocketData(GSMSocket * socket, uint8_t *data, size_t len) = 0;

    virtual void OnSocketStartListen(GSMSocket * socket) = 0;
};