#include "SimComGSMSocketManager.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectCMD2.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketStreamWriteModemCMD.h"
#include "command/SockeCreateModemCMD.h"
#include "command/LongArrayModemCMD.h"

SimComGSMSocketManager::SimComGSMSocketManager(GSMModemManager *gsmManager, GPRSManager *gprsManager):GSMSocketManager(gsmManager, gprsManager, 7)
{

}
SimComGSMSocketManager::~SimComGSMSocketManager()
{
    
}

bool SimComGSMSocketManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{

    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_CONNECT_CMD) == 0) {
        SocketConnectCMD2 *cipopen = (SocketConnectCMD2 *)request;
        if (type == MODEM_RESPONSE_OK) {
            // OnSocketConnection(cipopen->socketId, GSM_SOCKET_ERROR_NONE);
            // Wait till event ocures
        } else if (type >= MODEM_RESPONSE_ERROR) {
            OnSocketConnection(cipopen->socketId, GSM_SOCKET_ERROR_FAILED_CONNECT);
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_READ_CMD) == 0) {
        LongArrayModemCMD *cipixgetCMD = (LongArrayModemCMD*)request;
        uint8_t socketId = cipixgetCMD->values[1];
        //GSMSocket *sock = GetSocket(usordCMD->byteData);
        //uint16_t size = 0;
        if (type == MODEM_RESPONSE_DATA) {
            // TODO:
            char *cipixget = response + strlen(GSM_SIMCOM_SOCKET_READ_CMD) + 2;
            char *cipixgetArgs[4];
            SplitString(cipixget, ',', cipixgetArgs, 4, false);

            uint16_t dataSize = atoi(cipixgetArgs[2]);

            if (dataSize > 0) {
                gsmManager->SetExpectFixedLength(dataSize, 100000ul);
                long vals[3] = { 2, socketId, dataSize };
                gsmManager->AddCommand(new LongArrayModemCMD(vals, 3, GSM_SIMCOM_SOCKET_READ_CMD));
            }

        } else if (type == MODEM_RESPONSE_EXPECT_DATA) {
            OnSocketData(socketId, (uint8_t *)response, respLen);

        } else if (type == MODEM_RESPONSE_OK) {
            // TODO: Check if some thing left?
        } else {
            CloseSocket(socketId);
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_WRITE_CMD) == 0) {

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
    if (strcmp(request->cmd, GSM_SIMCOM_SOCKET_CLOSE_CMD) == 0) {

        ByteModemCMD *ciplose = (ByteModemCMD *)request;

        if (type >= MODEM_RESPONSE_OK) {
            OnSocketClosed(ciplose->byteData);
        }
        return true;
    }
    return false;
}

bool SimComGSMSocketManager::OnGSMEvent(char * data, size_t dataLen)
{
    // +CIPOPEN: 0,0
    if (IsEvent(GSM_SIMCOM_SOCKET_CONNECT_CMD, data, dataLen)) {
        char *cipopen = data + strlen(GSM_SIMCOM_SOCKET_CONNECT_CMD) + 2;
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

    // +IPCLOSE: <client_index>,<close_reason>
    if (IsEvent(GSM_SIMCOM_SOCKET_CLOSE_EVENT, data, dataLen)) {
        char *ipclose = data + strlen(GSM_SIMCOM_SOCKET_CLOSE_EVENT) + 2;
        char* ipcloseArgs[2];
        SplitString(ipclose, ',', ipcloseArgs, 2, false);
        uint8_t socketId = atoi(ipcloseArgs[0]);
        OnSocketClosed(socketId);
        return true;
    }

    if (IsEvent(GSM_SIMCOM_SOCKET_READ_EVENT, data, dataLen)) {
        // +RECEIVE,0,31
        // Other format, without extra symbols ": "
        char* receive = data + strlen(GSM_SIMCOM_SOCKET_READ_EVENT) + 1;
        char* receiveArgs[2];

        SplitString(receive, ',', receiveArgs, 2, false);
        uint8_t socketId = atoi(receiveArgs[0]);
        uint16_t available = atoi(receiveArgs[1]);

        if (available > GSM_SOCKET_BUFFER_SIZE) {
            available = GSM_SOCKET_BUFFER_SIZE;
        }

        if (available > 0) {
            long vals[3] = { 2, socketId, available };
            gsmManager->AddCommand(new LongArrayModemCMD(vals, 3, GSM_SIMCOM_SOCKET_READ_CMD));
        }
        return true;
    }
    if (IsEvent(GSM_SIMCOM_SOCKET_WRITE_CMD, data, dataLen)) {
        // +CIPSEND: 0,29,29
        // Notification that data has been sent, nothing todo here
        return true;
    }
    return false;
}

bool SimComGSMSocketManager::ConnectSocketInternal(GSMSocket *socket)
{
    // TODO: UDP support
    // Response time:
    // Range: 3000ms-120000ms
    // default: 120000ms
    socket = OnSocketCreated(GetNextAvailableSocketIndex());
    return socket != NULL;
}

bool SimComGSMSocketManager::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // AT+CIPOPEN=0,"TCP","183.230.174.137",6031
    return gsmManager->AddCommand(new SocketConnectCMD2(sock->GetId(), (char *)"TCP", sock->GetIp(), sock->GetPort(), GSM_SIMCOM_SOCKET_CONNECT_CMD, SOCKET_CONNECTION_TIMEOUT));
}

bool SimComGSMSocketManager::SetKeepAlive(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // No configuration here, just connect
    OnKeepAliveConfirm(sock->GetId());
    return true;
}

bool SimComGSMSocketManager::SetSSL(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // No configuration here, just connect
    OnSSLConfirm(sock->GetId());
    return true;

}

bool SimComGSMSocketManager::Close(uint8_t socketId)
{
    if (socketId == GSM_SOCKET_ERROR_ID) return false;
    return gsmManager->AddCommand(new ByteModemCMD(socketId, GSM_SIMCOM_SOCKET_CLOSE_CMD, SOCKET_CONNECTION_TIMEOUT));
}


bool SimComGSMSocketManager::SendInternal(GSMSocket *socket, ByteArray *packet)
{
    return gsmManager->AddCommand(new SocketStreamWriteModemCMD(socket->GetId(), packet, GSM_SIMCOM_SOCKET_WRITE_CMD, SOCKET_CMD_TIMEOUT));
}


bool SimComGSMSocketManager::SetTCPNoDelay(GSMSocket *socket)
{
    // TODO:
    if (socket == NULL) return false;
    OnTCPNoDelayConfirm(socket->GetId());
    return true;
}