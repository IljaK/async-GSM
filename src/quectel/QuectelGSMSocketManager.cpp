#include "QuectelGSMSocketManager.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectContextCMD.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketStreamWriteModemCMD.h"
#include "command/SockeCreateModemCMD.h"
#include "command/LongArrayModemCMD.h"

QuectelGSMSocketManager::QuectelGSMSocketManager(GSMModemManager *gsmManager, GPRSManager *gprsManager):GSMSocketManager(gsmManager, gprsManager, 7)
{

}
QuectelGSMSocketManager::~QuectelGSMSocketManager()
{
    
}

bool QuectelGSMSocketManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{

    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_CONNECT_CMD) == 0) {
        SocketConnectContextCMD *cipopen = (SocketConnectContextCMD *)request;
        if (type == MODEM_RESPONSE_OK) {
            // OnSocketConnection(cipopen->socketId, GSM_SOCKET_ERROR_NONE);
            // Wait till event ocures
        } else if (type >= MODEM_RESPONSE_ERROR) {
            OnSocketConnection(cipopen->socketId, GSM_SOCKET_ERROR_FAILED_CONNECT);
        }
        return true;
    }

    // Buffer access mode:
    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_READ_CMD) == 0) {
        ByteShortModemCMD *usordCMD = (ByteShortModemCMD*)request;
        //GSMSocket *sock = GetSocket(usordCMD->byteData);
        //uint16_t size = 0;
        if (type == MODEM_RESPONSE_DATA) {
            /*
            char *usord = response + strlen(GSM_QUECTEL_SOCKET_READ_CMD) + 2;
            char *usordArgs[3];
            SplitString(usord, ',', usordArgs, 3, false);
            usordArgs[2] = ShiftQuotations(usordArgs[2]);
            size = atoi(usordArgs[1]);
            // Decode string to same char buffer
            uint8_t *data = (uint8_t *)usordArgs[2];
            size_t decoded = decodeFromHex(usordArgs[2], data, size);
            OnSocketData(usordCMD->byteData, data, decoded);
            */
        } else if (type == MODEM_RESPONSE_OK) {
            // TODO: No need to handle next read, event +UUSORD will be triggered again if there is something left
        } else {
            CloseSocket(usordCMD->byteData);
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_WRITE_CMD) == 0) {

        SocketStreamWriteModemCMD *writeCMD = (SocketStreamWriteModemCMD *)request;
        if (type == MODEM_RESPONSE_EXTRA_TRIGGER) {
            // TODO: Write data
            writeCMD->WriteSteamContent(gsmManager->GetModemStream());
        } else if (type == MODEM_RESPONSE_OK) {
            SendNextAvailableData();
        } else if (type > MODEM_RESPONSE_OK) {
            CloseSocket(writeCMD->socketId);
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_QUECTEL_SOCKET_CLOSE_CMD) == 0) {

        ByteModemCMD *ciplose = (ByteModemCMD *)request;

        if (type >= MODEM_RESPONSE_OK) {
            OnSocketClosed(ciplose->byteData);
        }
        return true;
    }
    return false;
}

bool QuectelGSMSocketManager::OnGSMEvent(char * data, size_t dataLen)
{

    // +CIPOPEN: 0,0
    if (IsEvent(GSM_QUECTEL_SOCKET_CONNECT_CMD, data, dataLen)) {
        char *cipopen = data + strlen(GSM_QUECTEL_SOCKET_CONNECT_CMD) + 2;
        char* cipopenArgs[2];
        SplitString(cipopen, ',', cipopenArgs, 2, false);
        uint8_t socketId = atoi(cipopenArgs[0]);
        uint8_t error = atoi(cipopenArgs[1]);
        if (error == 0) {
            OnSocketConnection(socketId, GSM_SOCKET_ERROR_NONE);
        } else {
            OnSocketConnection(socketId, GSM_SOCKET_ERROR_FAILED_CONNECT);
        }
        return true;
    }
    if (IsEvent(GSM_QUECTEL_SOCKET_WRITE_CMD, data, dataLen)) {
        // +CIPSEND: 0,29,29
        // Notification that data has been sent, nothing todo here
        return true;
    }
    return false;
}

bool QuectelGSMSocketManager::ConnectSocketInternal(GSMSocket *socket)
{
    // TODO: UDP support
    // Response time:
    // Range: 3000ms-120000ms
    // default: 120000ms
    socket = OnSocketCreated(GetNextAvailableSocketIndex());
    return Connect(socket);
}

bool QuectelGSMSocketManager::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // AT+QIOPEN=<contextID>,<connectID>,<service_type>,<IP_address>/<domain_name>,<remote_port>[,<local_port>[,<access_mode>]]
    return gsmManager->AddCommand(new SocketConnectContextCMD(1, sock->GetId(), (char *)"TCP", sock->GetIp(), sock->GetPort(), 1, GSM_QUECTEL_SOCKET_CONNECT_CMD, SOCKET_CONNECTION_TIMEOUT));
}

bool QuectelGSMSocketManager::SetKeepAlive(GSMSocket *sock)
{
    if (sock == NULL) return false;
    return false;
}

bool QuectelGSMSocketManager::SetSSL(GSMSocket *sock)
{
    if (sock == NULL) return false;
    return false;

}

bool QuectelGSMSocketManager::Close(uint8_t socketId)
{
    if (socketId == GSM_SOCKET_ERROR_ID) return false;
    return gsmManager->AddCommand(new ByteModemCMD(socketId, GSM_QUECTEL_SOCKET_CLOSE_CMD, SOCKET_CONNECTION_TIMEOUT));
}


bool QuectelGSMSocketManager::SendInternal(GSMSocket *socket, ByteArray *packet)
{
    return gsmManager->AddCommand(new SocketStreamWriteModemCMD(socket->GetId(), packet, GSM_QUECTEL_SOCKET_WRITE_CMD, SOCKET_CMD_TIMEOUT));
}

void QuectelGSMSocketManager::HandleURCRecv(char **args, size_t argsLen)
{
    // Buffer access mode:
    // "recv",<connectID>
    //Serial.print("HandleURCRecv args: ");
    //Serial.println((int)argsLen);

    // Indirect push mode:
    // "recv",<connectID>,<currectrecvlength><CR><LF><data>.

    // +QIURC: "recv",0,7
    if (argsLen < 3) return;

    uint8_t socketId = atoi(args[1]);
    uint16_t available = atoi(args[2]);

    SetExpectingResponse(socketId, available);

}
void QuectelGSMSocketManager::HandleURCIncoming(char **args, size_t argsLen)
{
    // "incoming",<connectID>,<serverID>,<remoteIP>,<remote_port>
}
void QuectelGSMSocketManager::HandleURCClosed(char **args, size_t argsLen)
{
    // +QIURC: "closed",0
    // TODO: Destroy socket
    if (argsLen < 2) return;
    OnSocketClosed(atoi(args[1]));
}
