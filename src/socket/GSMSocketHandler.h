#pragma once
#include "../BaseGSMHandler.h"
#include "SocketArray.h"
#include "GSMSocket.h"
#include "IGSMSocketHandler.h"
#include "command/BaseModemCMD.h"

constexpr unsigned long SOCKET_CONNECTION_TIMEOUT = 60000000ul;
constexpr unsigned long SOCKET_CMD_TIMEOUT = 10000000ul;

constexpr char GSM_SOCKET_CREATE_CMD[] = "+USOCR"; // AT+USOCR=6  6 - TCP; 16 - UDP;
constexpr char GSM_SOCKET_CONNECT_CMD[] = "+USOCO"; // "AT+USOCO=%d,\"%s\",%d", _socket, _host, _port);
constexpr char GSM_SOCKET_CONNECT_EVENT[] = "+UUSOCO"; //

constexpr char GSM_SOCKET_CONFIG_CMD[] = "+USOSO"; // +USOSO: (0-6),(0,6,65535)
constexpr char GSM_SOCKET_TYPE_CMD[] = "+USOSEC"; // AT+USOSEC=0,1,1

constexpr char GSM_SOCKET_CLOSE_CMD[] = "+USOCL"; // +USOCL: (0-6),(0-1) 1 = async close -> +UUSOCL
constexpr char GSM_SOCKET_CLOSE_EVENT[] = "+UUSOCL"; //

constexpr char GSM_SOCKET_READ_CMD[] = "+USORD"; // +USORD: (0-6),(0-1024)
constexpr char GSM_SOCKET_READ_EVENT[] = "+UUSORD"; // +UUSORD: <socket>,<length> +UUSORD: 3,16
constexpr char GSM_SOCKET_WRITE_CMD[] = "+USOWR"; // AT+USOWR=<socket>,<length>,<data>

#define MAX_SOCKET_AMOUNT 7
//constexpr uint16_t GSM_SOCKET_BUFFER_SIZE = (MODEM_SERIAL_BUFFER_SIZE / 2) - 20;
#define GSM_SOCKET_BUFFER_SIZE 256

class SocketArray;

class GSMSocketHandler: public IBaseGSMHandler
{
private:
    uint16_t closedTimeout = 0;
    SocketArray *socketArray = NULL;
    BaseGSMHandler *gsmHandler = NULL;
    IGSMSocketHandler *socketHandler = NULL;
    uint8_t pendingSockTransmission = 255; // 255 = NONE
    bool DestroySocket(uint8_t socketId);
protected:
    friend class GSMSocket;
    bool Connect(GSMSocket *socket);
    bool SetKeepAlive(GSMSocket *socket);
    bool SetSSL(GSMSocket *socket);
    bool Close(GSMSocket *socket);
    size_t Send(GSMSocket *socket);
    size_t SendNextAvailableData();
public:
    GSMSocketHandler(BaseGSMHandler *gsmHandler);
    virtual ~GSMSocketHandler();
    void SetSocketHandler(IGSMSocketHandler *socketHandler);

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    bool CreateSocket();
    bool CloseSocket(uint8_t socketId);

    bool CreateServer();
    void OnModemReboot();

    GSMSocket * GetSocket(uint8_t socketId);
    void PrintDebug(Print *stream);
};