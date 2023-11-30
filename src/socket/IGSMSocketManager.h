#pragma once
#include "GSMSocket.h"
#include "GSMSocketTypes.h"

class GSMSocket;

class IGSMSocketManager {
public:
    virtual void OnSocketConnected(GSMSocket * socket) = 0;
    
    virtual void OnSocketData(GSMSocket * socket, uint8_t *data, size_t len) = 0;

    virtual void OnSocketClose(GSMSocket * socket) = 0;
    
    virtual void OnSocketStartListen(GSMSocket * socket) = 0;
};