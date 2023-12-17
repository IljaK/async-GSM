#include "UbloxGSMSocketManager.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectCMD.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketHEXWriteModemCMD.h"
#include "command/SockeCreateModemCMD.h"

UbloxGSMSocketManager::UbloxGSMSocketManager(GSMModemManager *gsmManager, GPRSManager *gprsManager):GSMSocketManager(gsmManager, gprsManager, UBLOX_MAX_SOCKETS_AMOUNT)
{

}
UbloxGSMSocketManager::~UbloxGSMSocketManager()
{
    
}

bool UbloxGSMSocketManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    // Create socket
    if (strcmp(request->cmd, GSM_SOCKET_CREATE_CMD) == 0) {
        SockeCreateModemCMD *createSockSMD = (SockeCreateModemCMD *)request;
        if (type == MODEM_RESPONSE_DATA) {
            // +USOCR: 2
            size_t minLen = strlen(GSM_SOCKET_CREATE_CMD) + 2;
            if (respLen <= minLen) {
                return true;
            }
            char *sockId = response + minLen;
            size_t idLen = strlen(sockId);

            if (idLen > 0 && idLen <= 2) {
                createSockSMD->socketId = atoi(sockId);
            }
        } else if (type == MODEM_RESPONSE_OK) {
            GSMSocket * socket = GSMSocketManager::OnSocketCreated(createSockSMD->socketId);
            Connect(socket);
        } else {
            gprsManager->Deactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_CONNECT_CMD) == 0) {
        SocketConnectCMD *usoco = (SocketConnectCMD *)request;
        if (type == MODEM_RESPONSE_OK) {
            OnSocketConnection(usoco->socketId, GSM_SOCKET_ERROR_NONE);
        } else if (type == MODEM_RESPONSE_ERROR) {
            if (usoco->cme_error == 0) {
                OnSocketConnection(usoco->socketId, GSM_SOCKET_ERROR_FAILED_CONNECT);
            } else {
                gprsManager->Deactivate();
            }
        } else {
            gprsManager->Deactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_CONFIG_CMD) == 0) {
        ULong4ModemCMD *usoso = (ULong4ModemCMD*)request;
        if (type != MODEM_RESPONSE_DATA) {
            //GSMSocket *sock = GetSocket(usoso->valueData);
            if (type == MODEM_RESPONSE_OK) {
                if (usoso->valueData2 == 65535ul && usoso->valueData3 == 8ul) { // Activate keep alive setting
                    // Set keepidle duration
                    gsmManager->AddCommand(new ULong4ModemCMD(usoso->valueData, 6u, 2u, 30000ul, GSM_SOCKET_CONFIG_CMD));
                    return true;
                } else { // keepalive option
                    OnKeepAliveConfirm(usoso->valueData);
                }
            } else {
                OnSocketConnection(usoso->valueData, GSM_SOCKET_ERROR_FAILED_CONFIGURE_SOCKET);
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_TYPE_CMD) == 0) {
        Byte3ModemCMD *usosec = (Byte3ModemCMD *)request;

        if (type != MODEM_RESPONSE_DATA) {
            if (type == MODEM_RESPONSE_OK) {
                OnSSLConfirm(usosec->byteData);
            } else {
                OnSocketConnection(usosec->byteData, GSM_SOCKET_ERROR_FAILED_CONFIGURE_SOCKET);
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_READ_CMD) == 0) {
        ByteShortModemCMD *usordCMD = (ByteShortModemCMD*)request;
        //GSMSocket *sock = GetSocket(usordCMD->byteData);
        uint16_t size = 0;
        if (type == MODEM_RESPONSE_DATA) {
            char *usord = response + strlen(GSM_SOCKET_READ_CMD) + 2;
            char *usordArgs[3];
            SplitString(usord, ',', usordArgs, 3, false);
            usordArgs[2] = ShiftQuotations(usordArgs[2]);
            size = atoi(usordArgs[1]);
            // Decode string to same char buffer
            uint8_t *data = (uint8_t *)usordArgs[2];
            size_t decoded = decodeFromHex(usordArgs[2], data, size);
            OnSocketData(usordCMD->byteData, data, decoded);
        } else if (type == MODEM_RESPONSE_OK) {
            // TODO: No need to handle next read, event +UUSORD will be triggered again if there is something left
        } else {
            CloseSocket(usordCMD->byteData);
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_WRITE_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            SendNextAvailableData();
        } else if (type >= MODEM_RESPONSE_ERROR) {
            SocketHEXWriteModemCMD *writeCMD = (SocketHEXWriteModemCMD *)request;
            CloseSocket(writeCMD->socketId);
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_CLOSE_CMD) == 0) {

        ByteModemCMD *uusocl = (ByteModemCMD *)request;

        if (type >= MODEM_RESPONSE_OK) {
            /* TODO: Async supported device logic, SARA-U2 does not support it
            uint8_t isAsync = atoi(usoclArgs[1]);
            if (isAsync == 1) {
                if (type != MODEM_RESPONSE_OK) {
                    // TODO: ?
                }
                // No need to process state, async response will be triggered
                return true;
            }
            */
            OnSocketClosed(uusocl->byteData);
        }
        return true;
    }
    return false;
}

bool UbloxGSMSocketManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (IsEvent(GSM_SOCKET_CONNECT_EVENT, data, dataLen)) {
        char *uusoco = data + strlen(GSM_SOCKET_CONNECT_EVENT) + 2;
        char *uusocoData[2];
        SplitString(uusoco, ',', uusocoData, 2, false);
        uint8_t socketId = atoi(uusocoData[0]);
        int error = atoi(uusocoData[1]);
        if (error == 0) {
            OnSocketConnection(socketId, GSM_SOCKET_ERROR_NONE);
        } else {
            OnSocketConnection(socketId, GSM_SOCKET_ERROR_FAILED_CONNECT);
        }
        return true;
    }
    if (IsEvent(GSM_SOCKET_CLOSE_EVENT, data, dataLen)) {
        size_t pLen = strlen(GSM_SOCKET_CLOSE_EVENT) + 2;
        if (pLen >= dataLen) {
            return true;
        }
        char *pSockID = data + pLen;

        if (pSockID[0] == 0) {
            return true;
        }

        uint8_t socketId = atoi(pSockID);

        OnSocketClosed(socketId);
        return true;
    }
    if (IsEvent(GSM_SOCKET_READ_EVENT, data, dataLen)) {

        char* uusord = data + strlen(GSM_SOCKET_READ_EVENT) + 2;
        char* uusordArgs[2];

        SplitString(uusord, ',', uusordArgs, 2, false);
        uint8_t socketId = atoi(uusordArgs[0]);
        uint16_t available = atoi(uusordArgs[1]);

        if (available > 0) {
            gsmManager->ForceCommand(new ByteShortModemCMD(socketId, GSM_SOCKET_BUFFER_SIZE, GSM_SOCKET_READ_CMD));
        }
        return true;
    }
    return false;
}


bool UbloxGSMSocketManager::ConnectSocketInternal(GSMSocket *socket)
{
    return gsmManager->AddCommand(new SockeCreateModemCMD(6, GSM_SOCKET_CREATE_CMD, SOCKET_CMD_TIMEOUT));
}

bool UbloxGSMSocketManager::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    return gsmManager->AddCommand(new SocketConnectCMD(sock->GetId(), sock->GetIp(), sock->GetPort(), GSM_SOCKET_CONNECT_CMD, SOCKET_CONNECTION_TIMEOUT));
}

bool UbloxGSMSocketManager::SetKeepAlive(GSMSocket *sock)
{
    if (sock == NULL) return false;
    //char cmd[32];
    // AT+USOSO=2,6,2,30000
    // 2: keepidle option: send keepidle probes when it is idle for <opt_val> milliseconds
    // <opt_val>: signed 32 bit numeric parameter representing the milliseconds for
    // "keepidle" option. The range is 0-2147483647. The default value is 7200000 (2 hours)
    return gsmManager->AddCommand(new ULong4ModemCMD(sock->GetId(), 65535ul, 8u, 1u, GSM_SOCKET_CONFIG_CMD));
}

bool UbloxGSMSocketManager::SetSSL(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // AT+USOSEC=0,1,1
    // 0 (default value): disable the SSL/TLS/DTLS on the socket.
    // 1: enable the socket security; a USECMNG profile can be specified with the
    // <usecmng_profile_id> parameter.
    
    int ssl = (int)sock->SSLType();
    ssl--;
    return gsmManager->AddCommand(new Byte3ModemCMD(sock->GetId(), 1, ssl, GSM_SOCKET_TYPE_CMD));

}

bool UbloxGSMSocketManager::Close(uint8_t socketId)
{
    // TODO: Async close, SARAU2 does not have async close support
    return gsmManager->AddCommand(new ByteModemCMD(socketId, GSM_SOCKET_CLOSE_CMD, SOCKET_CONNECTION_TIMEOUT));
}


bool UbloxGSMSocketManager::SendInternal(GSMSocket *socket, ByteArray *packet)
{
    return gsmManager->AddCommand(new SocketHEXWriteModemCMD(socket->GetId(), packet, GSM_SOCKET_WRITE_CMD, SOCKET_CMD_TIMEOUT));
}


