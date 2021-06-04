#pragma once
#include "common/Timer.h"
#include "array/StackArray.h"
#include "../BaseGSMHandler.h"
#include "GSMSocket.h"

constexpr unsigned long SOCKET_CONNECTION_TIMEOUT = 10000000ul;

constexpr char GSM_SOCKET_CREATE_CMD[] = "+USOCR"; // AT+USOCR=6  6 - TCP; 16 - UDP;
constexpr char GSM_SOCKET_OPEN_CMD[] = "+USOCO"; // "AT+USOCO=%d,\"%s\",%d", _socket, _host, _port);
constexpr char GSM_SOCKET_OPEN_EVENT[] = "+UUSOCO"; //

constexpr char GSM_SOCKET_CONFIG_CMD[] = "+USOSO"; // +USOSO: (0-6),(0,6,65535)
constexpr char GSM_SOCKET_TYPE_CMD[] = "+USOSEC"; // AT+USOSEC=0,1,1

constexpr char GSM_SOCKET_CLOSE_CMD[] = "+USOCL"; // +USOCL: (0-6),(0-1) 1 = async close -> +UUSOCL
constexpr char GSM_SOCKET_CLOSE_EVENT[] = "+UUSOCL"; //

constexpr char GSM_SOCKET_READ_CMD[] = "+USORD"; // +USORD: (0-6),(0-1024)
constexpr char GSM_SOCKET_READ_EVENT[] = "+UUSORD"; // +UUSORD: <socket>,<length> +UUSORD: 3,16
constexpr char GSM_SOCKET_WRITE_CMD[] = "+USOWR"; // "AT+USOCO=%d,\"%s\",%d", _socket, _host, _port);

#define MAX_SOCKET_AMOUNT 7
constexpr uint16_t GSM_SOCKET_BUFFER_SIZE = (MODEM_SERIAL_BUFFER_SIZE / 2) - 20;

class GSMSocket;

class IGSMSocketHandler {
public:
    virtual void OnSocketCreateError() = 0;

    virtual void OnSocketOpen(GSMSocket * socket) = 0;
    virtual void OnSocketClose(uint8_t sockedId) = 0;

    virtual void OnSocketConnected(GSMSocket * socket, int error) = 0;
    virtual void OnSocketData(GSMSocket * socket) = 0;

    virtual void OnSocketStartListen(GSMSocket * socket) = 0;
};

class GSMSocketHandler: public IBaseGSMHandler, public ITimerCallback
{
private:
    StackArray<GSMSocket *> socketArray;
    BaseGSMHandler *gsmHandler = NULL;
    IGSMSocketHandler *socketHandler = NULL;
    GSMSocket *GetSocket(uint8_t socketId);
    GSMSocket *UnshiftSocket(uint8_t socketId);
protected:
    friend class GSMSocket;
    bool Connect(GSMSocket *socket);
    bool SetKeepAlive(GSMSocket *socket);
    bool SetSSL(GSMSocket *socket);
    bool Close(GSMSocket *socket);
    uint16_t Send(uint8_t socketId, uint8_t *data, uint16_t len);
public:
    GSMSocketHandler(BaseGSMHandler *gsmHandler);
    virtual ~GSMSocketHandler();
    void SetSocketHandler(IGSMSocketHandler *socketHandler);

    bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data) override;

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    bool CreateSocket();
    bool CloseSocket(uint8_t socketId);

    bool CreateServer();
};