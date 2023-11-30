#pragma once
#include "socket/GSMSocketManager.h"
#include "GSMModemManager.h"

// You can use AT+CIPOPEN to establish a connection with TCP server and UDP server, the maximumof theconnections is 10.
constexpr char GSM_SIMCOM_SOCKET_CONNECT_CMD[] = "+CIPOPEN"; // AT+CIPOPEN=0,"TCP","183.230.174.137",6031

//constexpr char GSM_SIMCOM_SOCKET_CONFIG_CMD[] = "+USOSO"; // +USOSO: (0-6),(0,6,65535)
//constexpr char GSM_SIMCOM_SOCKET_TYPE_CMD[] = "+USOSEC"; // AT+USOSEC=0,1,1

constexpr char GSM_SIMCOM_SOCKET_CLOSE_CMD[] = "+CIPCLOSE"; // AT+CIPCLOSE is used to close a TCP or UDP Socket
constexpr char GSM_SIMCOM_SOCKET_CLOSE_EVENT[] = "+IPCLOSE"; // +IPCLOSE: <client_index>,<close_reason>

constexpr char GSM_SIMCOM_SOCKET_READ_CMD[] = "+CIPRXGET"; // +USORD: (0-6),(0-1024)
constexpr char GSM_SIMCOM_SOCKET_READ_EVENT[] = "+RECEIVE"; // +RECEIVE: <socket>,<length> +RECEIVE: 3,16

/*
AT+CIPSEND is used to send data to remote side. 
If service type is TCP, the data is firstly sent tothemodule’s internal TCP/IP stack, 
and then sent to server by protocol stack. The <length> field maybeempty. 
*/
constexpr char GSM_SIMCOM_SOCKET_WRITE_CMD[] = "+CIPSEND"; // AT+CIPSEND=<link_num>,<length>


class SimComGSMSocketManager: public GSMSocketManager
{
private:
    uint8_t receivingSocketId = GSM_SOCKET_ERROR_ID;
protected:

    bool Connect(GSMSocket *sock) override;
    bool SetKeepAlive(GSMSocket *sock) override;
    bool SetSSL(GSMSocket *sock) override;
    bool Close(uint8_t socketId) override;
    bool SendInternal(GSMSocket *socket, ByteArray *packet) override;
    bool ConnectSocketInternal(GSMSocket *socket) override;

public:
    SimComGSMSocketManager(GSMModemManager *gsmManager);
    virtual ~SimComGSMSocketManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;
    bool OnGSMExpectedData(uint8_t * data, size_t dataLen) override;

};