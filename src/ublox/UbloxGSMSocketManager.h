#pragma once
#include "socket/GSMSocketManager.h"
#include "GSMModemManager.h"

#define UBLOX_MAX_SOCKETS_AMOUNT 7

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

class UbloxGSMSocketManager: public GSMSocketManager
{
protected:

    bool Connect(GSMSocket *sock) override;
    bool SetKeepAlive(GSMSocket *sock) override;
    bool SetSSL(GSMSocket *sock) override;
    bool Close(GSMSocket *socket) override;
    bool SendInternal(GSMSocket *socket, ByteArray *packet) override;

public:
    UbloxGSMSocketManager(GSMModemManager *gsmManager);
    virtual ~UbloxGSMSocketManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    bool CreateSocket() override;

};