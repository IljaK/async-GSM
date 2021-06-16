#include "SocketArray.h"

SocketArray::SocketArray(const size_t maxSize):StackArray(maxSize)
{

}
void SocketArray::FreeItem(GSMSocket * item)
{
    delete item;
}

GSMSocket *SocketArray::PeekSocket(uint8_t socketId) {
    if (Size() == 0) return NULL;

    GSMSocket * sock = NULL;
    for (size_t i = 0; i < Size(); i++) {
        sock = Peek(i);
        if (sock->GetId() == socketId) {
            return sock;
        }
    }
    return NULL;
}

GSMSocket *SocketArray::UnshiftSocket(uint8_t socketId) {
    if (Size() == 0) return NULL;

    GSMSocket * sock = NULL;
    for (size_t i = 0; i < Size(); i++) {
        sock = Peek(i);
        if (sock->GetId() == socketId) {
            return Unshift(i);
        }
    }
    return NULL;
}