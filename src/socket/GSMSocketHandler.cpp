#include "GSMSocketHandler.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectCMD.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketWriteModemCMD.h"

GSMSocketHandler::GSMSocketHandler(BaseGSMHandler *gsmHandler)
{
    socketArray = new SocketArray(MAX_SOCKET_AMOUNT);
    this->gsmHandler = gsmHandler;
}
GSMSocketHandler::~GSMSocketHandler()
{
    socketArray->Clear();
    delete socketArray;
}

void GSMSocketHandler::SetSocketHandler(IGSMSocketHandler *socketHandler)
{
    this->socketHandler = socketHandler;
}

void GSMSocketHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{

}

void GSMSocketHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    
}

bool GSMSocketHandler::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    // Create socket
    if (strcmp(request->cmd, GSM_SOCKET_CREATE_CMD) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
            // +USOCR: 2
            if (strncmp(response, GSM_SOCKET_CREATE_CMD, strlen(GSM_SOCKET_CREATE_CMD)) == 0) {
                uint8_t sockeId = atoi(response + strlen(GSM_SOCKET_CREATE_CMD) + 2);
                GSMSocket *sock = new GSMSocket(this, sockeId);
                socketArray->Append(sock);
                if (socketHandler != NULL) {
                    socketHandler->OnSocketOpen(sock);
                }
            }
        } else if (type == MODEM_RESPONSE_OK) {
            // Nothing to do here?
        } else {
            if (socketHandler != NULL) {
                socketHandler->OnSocketCreateError();
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_CONNECT_CMD) == 0) {
        SocketConnectCMD *usoco = (SocketConnectCMD *)request;
        GSMSocket *sock = socketArray->PeekSocket(usoco->socketId);

        if (type == MODEM_RESPONSE_OK) {
            if (sock != NULL) sock->OnConnectionConfirm(0);

            if (socketHandler != NULL) {
                socketHandler->OnSocketConnected(sock, 0);
            }
        } else {
            // TODO: create auto close timer if +UUSOCL hasnt been triggered
            if (sock != NULL) sock->OnConnectionConfirm(1);

            if (socketHandler != NULL) {
                socketHandler->OnSocketConnected(sock, 1);
            }
            // There mustn't be any data event, otherwise response is corrupted
            if (type == MODEM_RESPONSE_DATA) {
                DestroySocket(usoco->socketId);
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_CONFIG_CMD) == 0) {
        ULong4ModemCMD *usoso = (ULong4ModemCMD*)request;
        if (type != MODEM_RESPONSE_DATA) {
            GSMSocket *sock = socketArray->PeekSocket(usoso->valueData);
            if (type == MODEM_RESPONSE_OK) {
                if (usoso->valueData2 == 65535ul && usoso->valueData3 == 8ul) { // Activate keep alive setting
                    // Set keepidle duration
                    return gsmHandler->AddCommand(new ULong4ModemCMD(sock->GetId(), 6u, 2u, 30000ul, GSM_SOCKET_CONFIG_CMD));
                } else { // keepalive option
                    if (sock != NULL) {
                        sock->OnKeepAliveConfirm(true);
                    }
                }
            } else {
                if (sock != NULL) {
                    sock->OnKeepAliveConfirm(false);
                }
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_TYPE_CMD) == 0) {
        Byte3ModemCMD *usosec = (Byte3ModemCMD *)request;

        if (type != MODEM_RESPONSE_DATA) {
            GSMSocket *sock = socketArray->PeekSocket(usosec->byteData);
            if (type == MODEM_RESPONSE_OK) {
                if (sock != NULL) {
                    sock->OnSSLConfirm(true);
                }
            } else {
                if (sock != NULL) {
                    sock->OnSSLConfirm(false);
                }
            }
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_READ_CMD) == 0) {
        ByteShortModemCMD *usordCMD = (ByteShortModemCMD*)request;
        GSMSocket *sock = socketArray->PeekSocket(usordCMD->byteData);
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
            if (socketHandler != NULL) {
                socketHandler->OnSocketData(sock, data, decoded);
            }
        } else if (type == MODEM_RESPONSE_OK) {
            // TODO: No need to handle next read, event +UUSORD will be triggered again if there is something left
        } else {
            // TODO: Handle error?
        }
        return true;
    }
    if (strcmp(request->cmd, GSM_SOCKET_WRITE_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            SendNextAvailableData();
        } else if (type > MODEM_RESPONSE_OK) {
            // TODO: Error ?
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
            DestroySocket(uusocl->byteData);
        }
        return true;
    }
    return false;
}

bool GSMSocketHandler::OnGSMEvent(char * data, size_t dataLen)
{
    if (strncmp(data, GSM_SOCKET_CONNECT_EVENT, strlen(GSM_SOCKET_CONNECT_EVENT)) == 0) {
        char *uusoco = data + strlen(GSM_SOCKET_CONNECT_EVENT) + 2;
        char *uusocoData[2];
        SplitString(uusoco, ',', uusocoData, 2, false);
        uint8_t socketId = atoi(uusocoData[0]);
        int error = atoi(uusocoData[1]);

        GSMSocket *sock = socketArray->PeekSocket(socketId);
        if (sock != NULL) {
            sock->OnConnectionConfirm(error);
            if (socketHandler != NULL) {
                socketHandler->OnSocketConnected(sock, error);
            }
        }
        // TODO: Error?
        return true;
    }
    if (strncmp(data, GSM_SOCKET_CLOSE_EVENT, strlen(GSM_SOCKET_CLOSE_EVENT)) == 0) {
        size_t pLen = strlen(GSM_SOCKET_CLOSE_EVENT) + 2;
        if (pLen >= dataLen) {
            return true;
        }
        char *pSockID = data + pLen;

        if (pSockID[0] == 0) {
            return true;
        }

        uint8_t socketId = atoi(pSockID);

        DestroySocket(socketId);
        return true;
    }
    if (strncmp(data, GSM_SOCKET_READ_EVENT, strlen(GSM_SOCKET_READ_EVENT)) == 0) {

        char* uusord = data + strlen(GSM_SOCKET_READ_EVENT) + 2;
        char* uusordArgs[2];

        SplitString(uusord, ',', uusordArgs, 2, false);
        uint8_t socketId = atoi(uusordArgs[0]);
        uint16_t available = atoi(uusordArgs[1]);

        if (available > 0) {
            gsmHandler->ForceCommand(new ByteShortModemCMD(socketId, GSM_SOCKET_BUFFER_SIZE, GSM_SOCKET_READ_CMD));
        }

        // TODO: Error?
        return true;
    }
    return false;
}

bool GSMSocketHandler::CreateSocket()
{
    if (socketArray->Size() >= MAX_SOCKET_AMOUNT) {
        return false;
    }
    return gsmHandler->AddCommand(new ByteModemCMD(6, GSM_SOCKET_CREATE_CMD, SOCKET_CMD_TIMEOUT));
}

bool GSMSocketHandler::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    return gsmHandler->AddCommand(new SocketConnectCMD(sock->GetId(), sock->GetIp(), sock->GetPort(), GSM_SOCKET_CONNECT_CMD, SOCKET_CONNECTION_TIMEOUT));
}

bool GSMSocketHandler::SetKeepAlive(GSMSocket *sock)
{
    if (sock == NULL) return false;
    //char cmd[32];
    // AT+USOSO=2,6,2,30000
    // 2: keepidle option: send keepidle probes when it is idle for <opt_val> milliseconds
    // <opt_val>: signed 32 bit numeric parameter representing the milliseconds for
    // "keepidle" option. The range is 0-2147483647. The default value is 7200000 (2 hours)
    return gsmHandler->AddCommand(new ULong4ModemCMD(sock->GetId(), 65535ul, 8u, 1u, GSM_SOCKET_CONFIG_CMD));
}

bool GSMSocketHandler::SetSSL(GSMSocket *sock)
{
    if (sock == NULL) return false;
    // AT+USOSEC=0,1,1
    // 0 (default value): disable the SSL/TLS/DTLS on the socket.
    // 1: enable the socket security; a USECMNG profile can be specified with the
    // <usecmng_profile_id> parameter.
    
    int ssl = (int)sock->SSLType();
    ssl--;
    return gsmHandler->AddCommand(new Byte3ModemCMD(sock->GetId(), 1, ssl, GSM_SOCKET_TYPE_CMD));

}

bool GSMSocketHandler::Close(GSMSocket *socket)
{
    if (socket == NULL) return false;
    // TODO: Async close, SARAU2 does not have async close support
    return gsmHandler->AddCommand(new ByteModemCMD(socket->GetId(), GSM_SOCKET_CLOSE_CMD, SOCKET_CONNECTION_TIMEOUT));
}

size_t GSMSocketHandler::Send(GSMSocket *socket)
{
    // We have pending send data command, wait till it finished to not overflow command buffer
    if (pendingSockTransmission != 255) return 0;
    if (socket == NULL) return 0;
    
    ByteArray *packet = socket->outgoingMessageStack.Peek();

    if (packet == NULL || packet->length == 0) return 0;

    pendingSockTransmission = socket->GetId();
    if (!gsmHandler->AddCommand(new SocketWriteModemCMD(socket->GetId(), packet, GSM_SOCKET_WRITE_CMD, SOCKET_CMD_TIMEOUT))){
        return 0;
    }
    // If send added to stack, unshift packet from pending array
    socket->outgoingMessageStack.UnshiftFirst();
    return packet->length;
}

bool GSMSocketHandler::CreateServer()
{
    // TODO:
    return false;
}
bool GSMSocketHandler::CloseSocket(uint8_t socketId)
{
    GSMSocket *sock = socketArray->PeekSocket(socketId);
    if (sock == NULL) return false;

    return sock->Close();
}

size_t GSMSocketHandler::SendNextAvailableData()
{
    GSMSocket *sock = NULL;
    pendingSockTransmission++; // Switch to the next socket
    if (pendingSockTransmission >= MAX_SOCKET_AMOUNT) {
        pendingSockTransmission = 0;
    }

    // Check next available socket
    for(uint8_t i = pendingSockTransmission; i < MAX_SOCKET_AMOUNT; i++) {
        sock = socketArray->PeekSocket(i);
        if (sock == NULL) {
            continue;
        }
        if (sock->outgoingMessageStack.Size() > 0) {
            pendingSockTransmission = 255;
            return Send(sock);
        }
    }
    // Check next socket with lower ID
    for(uint8_t i = 0; i <= pendingSockTransmission; i++) {
        sock = socketArray->PeekSocket(i);
        if (sock == NULL) {
            continue;
        }
        if (sock->outgoingMessageStack.Size() > 0) {
            pendingSockTransmission = 255;
            return Send(sock);
        }
    }
    // No data to send
    pendingSockTransmission = 255;
    return 0;
}

bool GSMSocketHandler::DestroySocket(uint8_t socketId)
{
    GSMSocket *sock = socketArray->UnshiftSocket(socketId);
    if (sock == NULL) {
        return false;
    }
    if (socketHandler != NULL) {
        socketHandler->OnSocketClose(socketId, true);
    }
    socketArray->FreeItem(sock);
    return true;
}

void GSMSocketHandler::OnModemReboot()
{
    pendingSockTransmission = 255;
    while(socketArray->Size() > 0) {
        DestroySocket(socketArray->Peek(0)->GetId());
    }
}

GSMSocket * GSMSocketHandler::GetSocket(uint8_t socketId)
{
    return socketArray->PeekSocket(socketId);
}