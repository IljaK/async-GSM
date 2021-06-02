#include "GSMSocket.h"


GSMSocket::GSMSocket(GSMSocketHandler * socketHandler, uint8_t socketId)
{
    this->socketHandler = socketHandler;
    this->socketId = socketId;
}
GSMSocket::~GSMSocket()
{

}


void GSMSocket::OnKeepAliveConfirm(bool isSuccess)
{
    Serial.println("GSMSocket::OnKeepAliveConfirm");
    // TODO: if !isSuccess ?
    if (sslType == GSM_SOCKET_SSL_DISABLE) {
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

bool GSMSocket::SendData(uint8_t *data, uint16_t len)
{
    if (state != GSM_SOCKET_STATE_READY) {
        return false;
    }
    
    // TODO: Check unwritten data

    uint16_t written = socketHandler->Send(socketId, data, len);

    // TODO: save remain data for next send?

    return written > 1;
}

void GSMSocket::OnReadData(char *data, uint16_t available)
{
    this->available += available;
    // TODO: Decode hex
    // TODO: Split to packets?
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
uint16_t GSMSocket::GetAvailable()
{
    return available;
}
bool GSMSocket::IsKeepAlive()
{
    return keepAlive;
}

GSM_SOCKET_SSL GSMSocket::SSLType()
{
    return sslType;
}