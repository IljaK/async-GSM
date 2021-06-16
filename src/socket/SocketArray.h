#pragma once
#include "array/StackArray.h"
#include "GSMSocket.h"

class GSMSocket;

class SocketArray: public StackArray<GSMSocket *>
{
public:
    SocketArray(const size_t maxSize);
    void FreeItem(GSMSocket * item) override;

    GSMSocket *PeekSocket(uint8_t socketId);

    GSMSocket *UnshiftSocket(uint8_t socketId);
};