#include "GSMSocket.h"


GSMSocket::GSMSocket(GSMSocketManager * socketManager, uint8_t socketId):
    outgoingMessageStack(16, 128)
{
    this->socketManager = socketManager;
    this->socketId = socketId;
}
GSMSocket::~GSMSocket()
{
    if (domain != NULL) {
        free(domain);
        domain = NULL;
    }
    outgoingMessageStack.Clear();
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

void GSMSocket::OnConnectionConfirm(bool isSuccess)
{
    if (!isSuccess) {
        state = GSM_SOCKET_STATE_CLOSING_DELAY;
        closeTimer = Timer::Start(this, SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT);
        return;
    }
    state = GSM_SOCKET_STATE_READY;
}

bool GSMSocket::Connect(char *ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType)
{
    if (state != GSM_SOCKET_STATE_DISCONNECTED) {
        return false;
    }
    
    GetAddr(ip, &this->ip);

    return Connect(this->ip, port, keepAlive, sslType);
}

bool GSMSocket::Connect(IPAddr ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType)
{
    if (state != GSM_SOCKET_STATE_DISCONNECTED) {
        return false;
    }
    state = GSM_SOCKET_STATE_CONNECTING;
    this->keepAlive = keepAlive;
    this->sslType = sslType;
    this->port = port;
    this->ip = ip;

    if (keepAlive) {
        return socketManager->SetKeepAlive(this);
    }
    if (sslType != GSM_SOCKET_SSL_DISABLE) {
        return socketManager->SetSSL(this);
    }

    return socketManager->Connect(this);
}

bool GSMSocket::Close()
{
    if (state == GSM_SOCKET_STATE_CLOSING || state == GSM_SOCKET_STATE_CLOSING_DELAY) {
        return false;
    }
    state = GSM_SOCKET_STATE_CLOSING;
    return socketManager->Close(this);
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

void GSMSocket::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == closeTimer) {
        closeTimer = 0;
        socketManager->DestroySocket(socketId);
    }
}

void GSMSocket::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == closeTimer) {
        closeTimer = 0;
    }
}
