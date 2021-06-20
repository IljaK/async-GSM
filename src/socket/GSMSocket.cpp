#include "GSMSocket.h"


GSMSocket::GSMSocket(GSMSocketHandler * socketHandler, uint8_t socketId):
    outgoingMessageStack(16, 128)
{
    this->socketHandler = socketHandler;
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

void GSMSocket::OnKeepAliveConfirm(bool isSuccess)
{
    // TODO: if !isSuccess ?
    if (sslType > GSM_SOCKET_SSL_DISABLE) {
        socketHandler->SetSSL(this);
    } else {
        socketHandler->Connect(this);
    }
}

void GSMSocket::OnSSLConfirm(bool isSuccess)
{
    // TODO: if !isSuccess ?
    socketHandler->Connect(this);
}

void GSMSocket::OnConnectionConfirm(int error)
{
    Serial.print("OnConnectionConfirm: ");
    Serial.println(error);
    if (error == 0) {
        state = GSM_SOCKET_STATE_READY;
    } else {
        state = GSM_SOCKET_STATE_CLOSING_DELAY;
        closeTimer = Timer::Start(this, SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT);
    }
}

bool GSMSocket::Connect(char *ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType)
{
    if (state != GSM_SOCKET_STATE_DISCONNECTED) {
        return false;
    }
    state = GSM_SOCKET_STATE_CONNECTING;
    this->keepAlive = keepAlive;
    this->sslType = sslType;
    this->port = port;

    GetAddr(ip, &this->ip);

    if (keepAlive) {
        return socketHandler->SetKeepAlive(this);
    }
    if (sslType != GSM_SOCKET_SSL_DISABLE) {
        return socketHandler->SetSSL(this);
    }

    return socketHandler->Connect(this);
}

bool GSMSocket::Close()
{
    if (state == GSM_SOCKET_STATE_CLOSING || state == GSM_SOCKET_STATE_CLOSING_DELAY) {
        return false;
    }
    state = GSM_SOCKET_STATE_CLOSING;
    return socketHandler->Close(this);
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
    socketHandler->Send(this);
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
        socketHandler->DestroySocket(socketId);
    }
}

void GSMSocket::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == closeTimer) {
        closeTimer = 0;
    }
}
