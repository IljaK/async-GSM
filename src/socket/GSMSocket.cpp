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
    }
    outgoingMessageStack.Clear();
}


void GSMSocket::OnKeepAliveConfirm(bool isSuccess)
{
    Serial.println("GSMSocket::OnKeepAliveConfirm");
    // TODO: if !isSuccess ?
    if (sslType > GSM_SOCKET_SSL_DISABLE) {
        socketHandler->SetSSL(this);
    } else {
        socketHandler->Connect(this);
    }
}

void GSMSocket::OnSSLConfirm(bool isSuccess)
{
    Serial.println("GSMSocket::OnSSLConfirm");
    // TODO: if !isSuccess ?
    socketHandler->Connect(this);
}

void GSMSocket::OnConnectionConfirm(int error)
{
    Serial.println("GSMSocket::OnConnectionConfirm");
    if (error == 0) {
        state = GSM_SOCKET_STATE_READY;
    } else {
        // TODO: reconnect?
    }
}

void GSMSocket::OnSendData(uint16_t size)
{
    // TODO: Send remain data
}

bool GSMSocket::Connect(char *ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType)
{
    Serial.println("GSMSocket::Connect");
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
    if (state == GSM_SOCKET_STATE_CLOSING) {
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