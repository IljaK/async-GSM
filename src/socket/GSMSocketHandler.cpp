#include "GSMSocketHandler.h"

GSMSocketHandler::GSMSocketHandler(BaseGSMHandler *gsmHandler):socketArray(MAX_SOCKET_AMOUNT)
{
    this->gsmHandler = gsmHandler;
}
GSMSocketHandler::~GSMSocketHandler()
{
    while (socketArray.Size() > 0)
    {
        GSMSocket *sock = socketArray.Unshift(socketArray.Size() - 1);
        delete sock;
    }
}

GSMSocket *GSMSocketHandler::GetSocket(uint8_t socketId)
{
    for(uint8_t i = 0; i < MAX_SOCKET_AMOUNT; i++) {
        GSMSocket *sock = socketArray.Peek(i);
        if (sock == NULL) {
            return NULL;
        }
        if (sock->GetId() == socketId) {
            return sock;
        }
    }
    return NULL;
}
GSMSocket *GSMSocketHandler::UnshiftSocket(uint8_t socketId)
{
    for(uint8_t i = 0; i < MAX_SOCKET_AMOUNT; i++) {
        GSMSocket *sock = socketArray.Peek(i);
        if (sock == NULL) {
            return NULL;
        }
        if (sock->GetId() == socketId) {
            return socketArray.Unshift(i);
        }
    }
    return NULL;
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

bool GSMSocketHandler::OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type)
{
    if (strlen(request) > 2) {
        // Create socket
        request+=2;
        if (strncmp(request, GSM_SOCKET_CREATE_CMD, strlen(GSM_SOCKET_CREATE_CMD)) == 0) {
            if (type == MODEM_RESPONSE_DATA) {
                // +USOCR: 2
                if (strncmp(response, GSM_SOCKET_CREATE_CMD, strlen(GSM_SOCKET_CREATE_CMD)) == 0) {
                    uint8_t sockeId = atoi(response + strlen(GSM_SOCKET_CREATE_CMD) + 2);
                    GSMSocket *sock = new GSMSocket(this, sockeId);
                    socketArray.Append(sock);
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
        if (strncmp(request, GSM_SOCKET_OPEN_CMD, strlen(GSM_SOCKET_OPEN_CMD)) == 0) {

            char socoReqData[128];
            strcpy(socoReqData, request + strlen(GSM_SOCKET_CONFIG_CMD) + 1);
            char *socoArgs[3];
            SplitString(socoReqData, ',', socoArgs, 4, false);

            // AT+USOCO=0,"195.34.89.241",7
            GSMSocket *sock = GetSocket(atoi(socoArgs[0]));

            if (type == MODEM_RESPONSE_OK) {
                sock->OnConnectionConfirm(0);
                if (socketHandler != NULL) {
                    socketHandler->OnSocketConnected(sock, 0);
                }
            } else if (type > MODEM_RESPONSE_OK) {
                sock->OnConnectionConfirm(1);
                if (socketHandler != NULL) {
                    socketHandler->OnSocketConnected(sock, 1);
                }
            }
            return true;
        }
        if (strncmp(request, GSM_SOCKET_CONFIG_CMD, strlen(GSM_SOCKET_CONFIG_CMD)) == 0) {
            char sosReqData[128];
            strcpy(sosReqData, request + strlen(GSM_SOCKET_CONFIG_CMD) + 1);
            char *sosoArgs[4];
            SplitString(sosReqData, ',', sosoArgs, 4, false);
            GSMSocket *sock = GetSocket(atoi(sosoArgs[0]));
            if (type == MODEM_RESPONSE_OK) {
                // AT+USOSO=0,6,2,30000
                // +USOSO: (0-6),(0,6,65535)
                if (sock != NULL) {
                    sock->OnKeepAliveConfirm(true);
                }
            } else if (type != MODEM_RESPONSE_DATA) {
                if (sock != NULL) {
                    sock->OnKeepAliveConfirm(false);
                }
            }
            return true;
        }
        if (strncmp(request, GSM_SOCKET_TYPE_CMD, strlen(GSM_SOCKET_TYPE_CMD)) == 0) {
            char sosecReqData[128];
            strcpy(sosecReqData, request + strlen(GSM_SOCKET_CONFIG_CMD) + 1);
            char *sosoArgs[3];
            SplitString(sosecReqData, ',', sosoArgs, 2, false);
            uint8_t socketId = atoi(sosoArgs[0]);
            GSMSocket *sock = GetSocket(socketId);
            if (type == MODEM_RESPONSE_OK) {
                // AT+USOSO=0,6,2,30000
                // +USOSO: (0-6),(0,6,65535)
                if (sock != NULL) {
                    sock->OnSSLConfirm(true);
                }
            } else if (type != MODEM_RESPONSE_DATA) {
                if (sock != NULL) {
                    sock->OnSSLConfirm(false);
                }
            }
            return true;
        }
        if (strncmp(request, GSM_SOCKET_READ_CMD, strlen(GSM_SOCKET_READ_CMD)) == 0) {
            uint8_t socketId = 0;
            GSMSocket *sock = NULL;
            uint16_t size = 0;
            if (type == MODEM_RESPONSE_DATA) {
                char *usord = response + strlen(GSM_SOCKET_READ_CMD) + 2;
                char *usordArgs[3];
                SplitString(usord, ',', usordArgs, 3, false);
                usordArgs[2] = ShiftQuotations(usordArgs[2]);
                socketId = atoi(usordArgs[0]);
                size = atoi(usordArgs[1]);
                sock = GetSocket(socketId);
                uint8_t *data = DecodeHexData(usordArgs[2], size);
                if (socketHandler != NULL) {
                    socketHandler->OnSocketData(sock, data, size);
                }
            } else if (type == MODEM_RESPONSE_OK) {
                // TODO: No need to handle next read, event +UUSORD will be triggered again if there is something left
            } else {
                // TODO: Handle error?
            }
            return true;
        }
        if (strncmp(request, GSM_SOCKET_WRITE_CMD, strlen(GSM_SOCKET_WRITE_CMD)) == 0) {
            uint8_t socketId = 0;

            if (type == MODEM_RESPONSE_OK) {
                char *usowr = request + strlen(GSM_SOCKET_WRITE_CMD) + 1;
                char *usowrArgs[2];
                SplitString(usowr, ',', usowrArgs, 2, false);
                socketId = atoi(usowrArgs[0]);
                uint16_t written = atoi(usowrArgs[1]);
                GSMSocket *sock = GetSocket(socketId);

                SendNextAvailableData();

                if (sock != NULL) {
                    sock->OnSendData(written);
                }

            } else if (type > MODEM_RESPONSE_OK) {
                // TODO: Error ?
            }
            return true;
        }
        if (strncmp(request, GSM_SOCKET_CLOSE_CMD, strlen(GSM_SOCKET_CLOSE_CMD)) == 0) {
            if (type >= MODEM_RESPONSE_OK) {
                char *usocl = request + strlen(GSM_SOCKET_READ_CMD) + 1;
                char *usoclArgs[2] = {0,0};
                // No need to copy request, we are procwssing string on last state
                SplitString(usocl, ',', usoclArgs, 2, false);

                uint8_t isAsync = atoi(usoclArgs[1]);
                if (isAsync == 1) {
                    if (type != MODEM_RESPONSE_OK) {
                        // TODO: ?
                    }
                    // No need to process state, async response will be triggered
                    return true;
                }
                uint8_t socketId = atoi(usoclArgs[0]);
                DestroySocket(socketId);
            }
            return true;
        }
    }
    return false;
}
bool GSMSocketHandler::OnGSMEvent(char * data)
{
    if (strncmp(data, GSM_SOCKET_OPEN_EVENT, strlen(GSM_SOCKET_OPEN_EVENT)) == 0) {
        char *uusoco = data + strlen(GSM_SOCKET_OPEN_EVENT) + 2;
        char *uusocoData[2];
        SplitString(uusoco, ',', uusocoData, 2, false);
        uint8_t socketId = atoi(uusocoData[0]);
        int error = atoi(uusocoData[1]);

        GSMSocket *sock = GetSocket(socketId);
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
        uint8_t socketId = atoi(data + strlen(GSM_SOCKET_CLOSE_EVENT) + 2);
        DestroySocket(socketId);
        // TODO: Error?
        return true;
    }
    if (strncmp(data, GSM_SOCKET_READ_EVENT, strlen(GSM_SOCKET_READ_EVENT)) == 0) {

        char* uusord = data + strlen(GSM_SOCKET_READ_EVENT) + 2;
        char* uusordArgs[2];

        SplitString(uusord, ',', uusordArgs, 2, false);
        uint8_t socketId = atoi(uusordArgs[0]);
        uint16_t available = atoi(uusordArgs[1]);

        if (available > 0) {
            char cmd[32];
            snprintf(cmd, 32, "%s%s=%u,%d", GSM_PREFIX_CMD, GSM_SOCKET_READ_CMD, socketId, GSM_SOCKET_BUFFER_SIZE);
            gsmHandler->ForceCommand(cmd);
        }

        // TODO: Error?
        return true;
    }
    return false;
}


bool GSMSocketHandler::CreateSocket()
{
    if (socketArray.Size() >= MAX_SOCKET_AMOUNT) {
        return false;
    }
    
    char cmd[32];
    snprintf(cmd, 32, "%s%s=6", GSM_PREFIX_CMD, GSM_SOCKET_CREATE_CMD); // 6 - TCP; 16 - UDP;
    return gsmHandler->AddCommand(cmd, SOCKET_CMD_TIMEOUT);
}

bool GSMSocketHandler::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    
    // TODO: Check ip

    char cmd[128];
    IPAddr ip = sock->GetIp();

    snprintf(cmd, 128, "%s%s=%u,\"%u.%u.%u.%u\",%u", GSM_PREFIX_CMD, GSM_SOCKET_OPEN_CMD, sock->GetId(), ip.octet[0], ip.octet[1], ip.octet[2], ip.octet[3], sock->GetPort()); //  "AT+USOCO=%d,\"%s\",%d", _socket, _host, _port);
    gsmHandler->AddCommand(cmd, SOCKET_CONNECTION_TIMEOUT);

    return false;
}

bool GSMSocketHandler::SetKeepAlive(GSMSocket *sock)
{
    if (sock == NULL) return false;
    char cmd[32];
    // AT+USOSO=2,6,1,1
    // 2: keepidle option: send keepidle probes when it is idle for <opt_val> milliseconds
    // <opt_val>: signed 32 bit numeric parameter representing the milliseconds for
    // "keepidle" option. The range is 0-2147483647. The default value is 7200000 (2 hours)
    snprintf(cmd, 32, "%s%s=%u,6,2,30000", GSM_PREFIX_CMD, GSM_SOCKET_CONFIG_CMD, sock->GetId());
    return gsmHandler->AddCommand(cmd);
}

bool GSMSocketHandler::SetSSL(GSMSocket *sock)
{
    if (sock == NULL) return false;
    char cmd[32];
    // AT+USOSEC=0,1,1
    // 0 (default value): disable the SSL/TLS/DTLS on the socket.
    // 1: enable the socket security; a USECMNG profile can be specified with the
    // <usecmng_profile_id> parameter.
    
    int ssl = (int)sock->SSLType();
    ssl--;

    snprintf(cmd, 32, "%s%s=%u,1,%d", GSM_PREFIX_CMD, GSM_SOCKET_TYPE_CMD, sock->GetId(), ssl);
    return gsmHandler->AddCommand(cmd);
}

bool GSMSocketHandler::Close(GSMSocket *socket)
{
    if (socket == NULL) return false;
    char cmd[32];
    snprintf(cmd, 32, "%s%s=%u", GSM_PREFIX_CMD, GSM_SOCKET_CLOSE_CMD, socket->GetId());
    // Async close, SARAU2 does not have async close support
    //snprintf(cmd, 32, "%s%s=%u,1", GSM_PREFIX_CMD, GSM_SOCKET_CLOSE_CMD, socket->GetId());
    return gsmHandler->AddCommand(cmd, SOCKET_CONNECTION_TIMEOUT);
}

size_t GSMSocketHandler::Send(GSMSocket *socket)
{
    // We have pending send data command, wait till it finished to not overflow command buffer
    if (pendingSockTransmission != 255) return 0;
    if (socket == NULL) return 0;
    ByteArray *packet = socket->outgoingMessageStack.UnshiftFirst();

    if (packet == NULL) return 0;

    pendingSockTransmission = socket->GetId();

    const int sz = GSM_SOCKET_BUFFER_SIZE * 2 + 20;
    char cmd[sz];
    snprintf(cmd, sz, "%s%s=%u,%zu,\"", GSM_PREFIX_CMD, GSM_SOCKET_WRITE_CMD, socket->GetId(), packet->length);

    size_t wrote = strlen(cmd);
    char *pBuff = cmd;

    for (size_t i = 0; i < packet->length; i++) {
        
        pBuff = cmd + wrote;      
        uint8_t b = packet->array[i];
        uint8_t n1 = (b >> 4) & 0x0f;
        uint8_t n2 = (b & 0x0f);
        pBuff[0] = (char)(n1 > 9 ? 'A' + n1 - 10 : '0' + n1);
        pBuff[1] = (char)(n2 > 9 ? 'A' + n2 - 10 : '0' + n2);
        pBuff[2] = 0;

        wrote += 2;

        // pBuff[0] + pBuff[1] + " + \r + 0 = 5 chars; 

        if (wrote + 5 >= MODEM_SERIAL_BUFFER_SIZE) {
            break;
        }
    }

    strcat(cmd, "\"");

    if (!gsmHandler->AddCommand(cmd, SOCKET_CMD_TIMEOUT)) {
        return 0;
    }
    return wrote;
}

bool GSMSocketHandler::CreateServer()
{
    // TODO:
    return false;
}
bool GSMSocketHandler::CloseSocket(uint8_t socketId)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) return false;

    return sock->Close();
}

uint8_t *GSMSocketHandler::DecodeHexData(char *hex, uint8_t bytesLen)
{
    // Decode hex data to same buffer
    uint8_t *dcode = (uint8_t *)hex;

    Serial.print("Decoded socket data: ");

    for (size_t i = 0; i < bytesLen; i++) {
        uint8_t n1 = hex[i * 2];
        uint8_t n2 = hex[i * 2 + 1];

        if (n1 > '9') {
            n1 = (n1 - 'A') + 10;
        } else {
            n1 = (n1 - '0');
        }

        if (n2 > '9') {
            n2 = (n2 - 'A') + 10;
        } else {
            n2 = (n2 - '0');
        }
        dcode[i] = (n1 << 4) | n2;
        Serial.print(dcode[i]);
        Serial.print(" ");
    }
    Serial.println("");
    return dcode;
}
size_t GSMSocketHandler::SendNextAvailableData()
{
    //GSMSocket *sock = NULL;
    pendingSockTransmission++; // Switch to the next socket
    if (pendingSockTransmission >= MAX_SOCKET_AMOUNT) {
        pendingSockTransmission = 0;
    }

    // Check next available socket
    for(uint8_t i = pendingSockTransmission; i < MAX_SOCKET_AMOUNT; i++) {
        GSMSocket *sock = GetSocket(i);
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
        GSMSocket *sock = GetSocket(i);
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
    GSMSocket *sock = UnshiftSocket(socketId);
    if (sock == NULL) {
        return false;
    }
    if (socketHandler != NULL) {
        socketHandler->OnSocketClose(socketId, true);
    }
    delete sock;
    return true;
}