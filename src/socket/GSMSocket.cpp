#include "GSMSocket.h"


GSMSocket::GSMSocket(GSMSocketManager * socketManager, IPAddr ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType):
    destroyTimer(this),
    outgoingMessageStack(16, 128)
{
    this->socketManager = socketManager;
    this->ip = ip;
    this->port = port;
    this->keepAlive = keepAlive;
    this->sslType = sslType;

    // TODO: Start timer:
    destroyTimer.StartMicros(SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT);
}
GSMSocket::~GSMSocket()
{
    outgoingMessageStack.Clear();
}


void GSMSocket::OnSocketID(uint8_t socketId)
{
    this->socketId = socketId;
    state = GSM_SOCKET_STATE_CONNECTING;
}


void GSMSocket::OnSocketConnection(GSM_SOCKET_ERROR error)
{
    this->error = error;
    if (error != GSM_SOCKET_ERROR_NONE) {
        socketManager->Close(socketId);
        state = GSM_SOCKET_STATE_CLOSING_DELAY;
        // Self destruction...
        destroyTimer.StartMicros(SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT);
        return;
    }
    state = GSM_SOCKET_STATE_READY;
    destroyTimer.Stop();
}

void GSMSocket::OnKeepAliveConfirm()
{
    if (sslType > GSM_SOCKET_SSL_DISABLE) {
        socketManager->SetSSL(this);
    } else {
        socketManager->Connect(this);
    }
}

void GSMSocket::OnSSLConfirm()
{
    socketManager->Connect(this);
}

bool GSMSocket::Close()
{
    if (state == GSM_SOCKET_STATE_CLOSING || state == GSM_SOCKET_STATE_CLOSING_DELAY) {
        return false;
    }
    state = GSM_SOCKET_STATE_CLOSING;
    return socketManager->Close(socketId);
}

bool GSMSocket::StartListen(uint16_t port)
{
    return false;
}

bool GSMSocket::SendData(uint8_t *data, size_t len)
{
    if (state != GSM_SOCKET_STATE_READY) {
        return false;
    }
    if (outgoingMessageStack.Append(data, len) == 0) {
        return false;
    }
    socketManager->Send(this);
    return true;
}

uint8_t GSMSocket::GetId()
{
    return socketId;
}
IPAddr GSMSocket::GetIp()
{
    return IPAddr(ip);
}
uint16_t GSMSocket::GetPort()
{
    return port;
}
GSM_SOCKET_STATE GSMSocket::GetState()
{
    return state;
}
bool GSMSocket::IsKeepAlive()
{
    return keepAlive;
}

GSM_SOCKET_SSL GSMSocket::SSLType()
{
    return sslType;
}

bool GSMSocket::IsConnected() {
    return state == GSM_SOCKET_STATE_READY;
}

void GSMSocket::OnTimerComplete(Timer * timer)
{
    if (timer == &destroyTimer) {
        socketManager->DestroySocket(socketId);
    }
}

bool GSMSocket::IsInitialising() {
    return GetId() == GSM_SOCKET_ERROR_ID && GetState() == GSM_SOCKET_STATE_CREATING;
}
