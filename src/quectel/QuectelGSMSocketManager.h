#pragma once
#include "socket/GSMSocketManager.h"
#include "GSMModemManager.h"

// AT+QIOPEN Open a Socket Service
// AT+QIOPEN=<contextID>,<connectID>,<service_type>,<IP_address>/<domain_name>,<remote_port>[,<local_port>[,<access_mode>]]
constexpr char GSM_QUECTEL_SOCKET_CONNECT_CMD[] = "+QIOPEN"; // 

constexpr char GSM_QUECTEL_SOCKET_CLOSE_CMD[] = "+QICLOSE"; // AT+CIPCLOSE is used to close a TCP or UDP Sockets

constexpr char GSM_QUECTEL_SOCKET_READ_CMD[] = "+QIRD"; // +QIRD:  (0-11),(0-1500)

/*
AT+CIPSEND is used to send data to remote side. 
If service type is TCP, the data is firstly sent tothemodule’s internal TCP/IP stack, 
and then sent to server by protocol stack. The <length> field maybeempty. 
*/
constexpr char GSM_QUECTEL_SOCKET_WRITE_CMD[] = "+QISEND"; // AT+CIPSEND=<link_num>,<length>


class QuectelGSMSocketManager: public GSMSocketManager
{
protected:

    bool Connect(GSMSocket *sock) override;
    bool SetKeepAlive(GSMSocket *sock) override;
    bool SetSSL(GSMSocket *sock) override;
    bool Close(uint8_t socketId) override;
    bool SendInternal(GSMSocket *socket, ByteArray *packet) override;
    bool ConnectSocketInternal(GSMSocket *socket) override;

public:
    QuectelGSMSocketManager(GSMModemManager *gsmManager, GPRSManager *gprsManager);
    virtual ~QuectelGSMSocketManager();

    bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    bool OnGSMEvent(char * data, size_t dataLen) override;

    void HandleURCRecv(char **args, size_t argsLen);
    void HandleURCIncoming(char **args, size_t argsLen);
    void HandleURCClosed(char **args, size_t argsLen);

};