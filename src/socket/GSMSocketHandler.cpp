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
        if (strncmp(request, GSM_SOCKET_CONFIG_CMD, strlen(GSM_SOCKET_CONFIG_CMD)) == 0) {
            char *soso = request + strlen(GSM_SOCKET_CONFIG_CMD) + 1;
            char *sosoArgs[4];
            SplitString(soso, ',', sosoArgs, 4, false);
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
            char *soso = request + strlen(GSM_SOCKET_TYPE_CMD) + 1;
            char *sosoArgs[3];
            SplitString(soso, ',', sosoArgs, 2, false);
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
                socketId = atoi(usordArgs[0]);
                size = atoi(usordArgs[1]);
                sock = GetSocket(socketId);
                sock->OnReadData(ShiftQuotations(usordArgs[2]), size);
            } else {
                char *usord1 = request + strlen(GSM_SOCKET_READ_CMD) + 1;
                char *usordArgs1[2];
                SplitString(usord1, ',', usordArgs1, 3, false);
                socketId = atoi(usordArgs1[0]);
                size = atoi(usordArgs1[1]);
                sock = GetSocket(socketId);

                if (type == MODEM_RESPONSE_OK) {
                    // AT+USOSO=0,6,2,30000
                    // +USOSO: (0-6),(0,6,65535)
                    if (sock != NULL) {
                        socketHandler->OnSocketData(sock);
                    }
                } else {
                    // TODO:
                }
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
                if (sock != NULL) {
                    sock->OnSendData(written);
                }

            } else if (type > MODEM_RESPONSE_OK) {

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
                socketHandler->OnSocketConnection(sock, error);
            }
        }
        // TODO: Error?
        return true;
    }
    if (strncmp(data, GSM_SOCKET_CLOSE_EVENT, strlen(GSM_SOCKET_CLOSE_EVENT)) == 0) {
        uint8_t socketId = atoi(data + strlen(GSM_SOCKET_CLOSE_EVENT) + 2);
        GSMSocket *sock = UnshiftSocket(socketId);
        if (sock != NULL) {
            if (socketHandler != NULL) {
                socketHandler->OnSocketClose(socketId);
            }
            delete sock;
        }
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
    return gsmHandler->AddCommand(cmd);
}

bool GSMSocketHandler::Connect(GSMSocket *sock)
{
    if (sock == NULL) return false;
    
    // TODO: Check ip

    char cmd[128];
    IPAddr ip = sock->GetIp();

    snprintf(cmd, 128, "%s%s=%u,\"%u.%u.%u.%u\",%u", GSM_PREFIX_CMD, GSM_SOCKET_OPEN_CMD, sock->GetId(), ip.octet[0], ip.octet[1], ip.octet[2], ip.octet[3], sock->GetPort()); //  "AT+USOCO=%d,\"%s\",%d", _socket, _host, _port);
    gsmHandler->AddCommand(cmd);

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
    snprintf(cmd, 32, "%s%s=%u,1", GSM_PREFIX_CMD, GSM_SOCKET_CLOSE_CMD, socket->GetId());
    return gsmHandler->AddCommand(cmd);
}

uint16_t GSMSocketHandler::Send(uint8_t socketId, uint8_t *data, uint16_t len)
{
    char cmd[MODEM_SERIAL_BUFFER_SIZE];
    snprintf(cmd, MODEM_SERIAL_BUFFER_SIZE, "%s%s=%u,%u,\"", GSM_PREFIX_CMD, GSM_SOCKET_WRITE_CMD, socketId, len);

    uint16_t wrote = strlen(cmd);
    char *pBuff = cmd;

    for (uint16_t i = 0; i < len; i++) {
        
        pBuff = cmd + wrote;      

        byte b = data[i];
        byte n1 = (b >> 4) & 0x0f;
        byte n2 = (b & 0x0f);
        pBuff[0] = (char)(n1 > 9 ? 'A' + n1 - 10 : '0' + n1);
        pBuff[1] = (char)(n2 > 9 ? 'A' + n2 - 10 : '0' + n2);
        pBuff[2] = 0;

        wrote += 2;

        // pBuff[0] + pBuff[1] + " + \r + 0 = 5 chars; 

        if (wrote + 5 >= MODEM_SERIAL_BUFFER_SIZE) {
            break;
        }
    }
    if (!gsmHandler->AddCommand(cmd)) {
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