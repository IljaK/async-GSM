#pragma once
#include <stdio.h>
#include <stdint.h>
#include "GSMSocketHandler.h"
#include "common/GSMUtils.h"
#include "array/ByteStackArray.h"
#include "SocketMessageBuffer.h"

constexpr unsigned long SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT = 1000000ul;

enum GSM_SOCKET_STATE: uint8_t {
    GSM_SOCKET_STATE_DISCONNECTED,
    GSM_SOCKET_STATE_CONNECTING,
    GSM_SOCKET_STATE_READY,
    GSM_SOCKET_STATE_CLOSING,
    GSM_SOCKET_STATE_CLOSING_DELAY
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

class GSMSocket: public ITimerCallback {
private:
    TimerID closeTimer = 0;

    uint8_t socketId;
    GSM_SOCKET_STATE state = GSM_SOCKET_STATE_DISCONNECTED;
    bool keepAlive = false;
    GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE;

    IPAddr ip;
    uint16_t port = 0;

    char * domain = NULL;
    GSMSocketHandler * socketHandler = NULL;
protected:
    ByteStackArray outgoingMessageStack;
    void OnKeepAliveConfirm(bool isSuccess);
    void OnSSLConfirm(bool isSuccess);
    void OnConnectionConfirm(int error);

    friend class GSMSocketHandler;
public:
    GSMSocket(GSMSocketHandler * socketHandler, uint8_t socketId);
    virtual ~GSMSocket();

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    uint8_t GetId();
    IPAddr GetIp();
    uint16_t GetPort();
    GSM_SOCKET_STATE GetState();
    uint16_t GetAvailable();
    uint8_t *GetData();

    bool IsKeepAlive();
    GSM_SOCKET_SSL SSLType();

    bool Connect(char *ip, uint16_t port, bool keepAlive = false, GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE);
    bool Close();

    bool StartListen(uint16_t port);
    bool SendData(uint8_t *data, size_t len);
    bool IsConnected();
};
