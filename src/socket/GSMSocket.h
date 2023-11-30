#pragma once
#include <stdio.h>
#include <stdint.h>
#include "GSMSocketManager.h"
#include "common/GSMUtils.h"
#include "array/ByteStackArray.h"
#include "SocketMessageBuffer.h"
#include "GSMSocketTypes.h"

class GSMSocketManager;

class GSMSocket: public ITimerCallback {
private:
    GSM_SOCKET_ERROR error = GSM_SOCKET_ERROR_NONE;
    Timer closeTimer;

    uint8_t socketId = GSM_SOCKET_ERROR_ID;
    GSM_SOCKET_STATE state = GSM_SOCKET_STATE_CREATING;
    bool keepAlive = false;
    GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE;

    IPAddr ip;
    uint16_t port = 0;

    GSMSocketManager * socketManager = NULL;
protected:
    ByteStackArray outgoingMessageStack;
    void OnKeepAliveConfirm();
    void OnSSLConfirm();
    void OnSocketID(uint8_t socketId);
    void OnSocketConnection(GSM_SOCKET_ERROR error);

    friend class GSMSocketManager;
public:
    GSMSocket(GSMSocketManager * socketManager, IPAddr ip, uint16_t port, bool keepAlive = true, GSM_SOCKET_SSL sslType = GSM_SOCKET_SSL_DISABLE);
    virtual ~GSMSocket();

	void OnTimerComplete(Timer * timer) override;

    uint8_t GetId();
    IPAddr GetIp();
    uint16_t GetPort();
    GSM_SOCKET_STATE GetState();

    // TODO: ?
    // uint16_t GetAvailable();
    // uint8_t *GetData();

    bool IsKeepAlive();
    GSM_SOCKET_SSL SSLType();

    bool Close();

    bool StartListen(uint16_t port);
    bool SendData(uint8_t *data, size_t len);
    bool IsConnected();
    bool IsInitialising();
};
