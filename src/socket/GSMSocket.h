#pragma once
#include <stdio.h>
#include <stdint.h>
#include "GSMSocketHandler.h"

enum GSM_SOCKET_STATE: uint8_t {
    GSM_SOCKET_STATE_DISCONNECTED,
    GSM_SOCKET_STATE_CONNECTING,
    GSM_SOCKET_STATE_READY,
    GSM_SOCKET_STATE_CLOSING
};

enum GSM_SOCKET_SSL: uint8_t {
    GSM_SOCKET_SSL_DISABLE,
    GSM_SOCKET_SSL_PROFILE_DEF,
    GSM_SOCKET_SSL_PROFILE_1,
    GSM_SOCKET_SSL_PROFILE_2,
    GSM_SOCKET_SSL_PROFILE_3,
    GSM_SOCKET_SSL_PROFILE_4
};

class GSMSocketHandler;

class GSMSocket {
private:
    uint8_t socketId;
    GSM_SOCKET_STATE state = GSM_SOCKET_STATE_DISCONNECTED;
    bool keepAlive = false;
    GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE;

    uint8_t ip[4] = {0,0,0,0};
    uint16_t port = 0;

    char * domain = NULL;
    GSMSocketHandler * socketHandler = NULL;
    uint16_t available = 0;
    uint8_t *data = NULL;
protected:
    void OnKeepAliveConfirm(bool isSuccess);
    void OnSSLConfirm(bool isSuccess);
    void OnConnectionConfirm(int error);
    void OnReadData(char *data, uint16_t available);

    friend class GSMSocketHandler;
public:
    GSMSocket(GSMSocketHandler * socketHandler, uint8_t socketId);
    ~GSMSocket();

    uint8_t GetId();
    uint8_t *GetIp();
    uint16_t GetPort();
    GSM_SOCKET_STATE GetState();
    uint16_t GetAvailable();
    uint8_t *GetData();

    bool IsKeepAlive();
    GSM_SOCKET_SSL SSLType();

    bool Connect(char *ip, uint16_t port, bool keepAlive = false, GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE);
    bool Close();

    bool StartListen(uint16_t port);
    bool SendData(uint8_t *data, uint16_t len);
};
