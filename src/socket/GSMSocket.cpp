#pragma once
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

bool GSMSocket::Connect(char *ip, uint16_t port, bool keepAlive, GSM_SOCKET_SSL sslType)
{
    Serial.println("GSMSocket::Connect");
    state = GSM_SOCKET_STATE_CONNECTING;
    this->keepAlive = keepAlive;
    this->sslType = sslType;

    char octet[4];

    int len = strlen(ip);
    int octetIndex = 0;
    int octetCharIndex = 0;

    for (int i = 0; i < len; i++, octetCharIndex++) {
        if (ip[i] == '.') {
            octet[octetCharIndex] = 0;
            octetCharIndex = 0;
            ip[octetIndex] = atoi(octet);
            octetIndex++;
        } else if (i + 1 == len) {
            octetCharIndex++;
            octet[octetCharIndex] = 0;
            ip[octetIndex] = atoi(octet);
        } else if (octetCharIndex >= 4) {
            return false;
        }
    }

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
        return;
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
uint8_t *GSMSocket::GetIp()
{
    return ip;
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