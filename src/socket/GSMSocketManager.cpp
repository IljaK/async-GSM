#include "GSMSocketManager.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectCMD.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketHEXWriteModemCMD.h"
#include "command/SockeCreateModemCMD.h"

GSMSocketManager::GSMSocketManager(GSMModemManager *gsmManager, GPRSManager *gprsManager, uint8_t socketsAmount)
{
    socketArray = new SocketArray(socketsAmount);
    this->gsmManager = gsmManager;
    this->gprsManager = gprsManager;
}
GSMSocketManager::~GSMSocketManager()
{
    socketArray->Clear();
    delete socketArray;
}

void GSMSocketManager::SetSocketHandler(IGSMSocketManager *socketHandler)
{
    this->socketHandler = socketHandler;
}


GSMSocket * GSMSocketManager::OnSocketCreated(uint8_t socketId)
{
    // Unshift first not created
    GSMSocket *socket = socketArray->PeekInitialisingSocket();
    if (socket == NULL) {
        // No awaiting to create sockets?!
        // Socket will be destroyed after close commans completes!
        Close(socketId);
    } else {
        socket->OnSocketID(socketId);
    }
    return socket;
    
    // Invalid socket ID, now what?!
    //if (socketId == GSM_SOCKET_ERROR_ID) {
        //return;
    //}
}
void GSMSocketManager::OnSocketConnection(uint8_t socketId, GSM_SOCKET_ERROR error)
{
    Serial.print("OnSocketConnection: ");
    Serial.print((int)socketId);
    Serial.print(" ");
    Serial.println((int)error);


    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) {
        // Shit happens?
        if (socketId != GSM_SOCKET_ERROR_ID) {
            Close(socketId);
        } else {
            DestroySocket(socketId);
        }
        return;
    }
    sock->OnSocketConnection(error);

    if (error == GSM_SOCKET_ERROR_NONE) {
        if (socketHandler != NULL) {
            socketHandler->OnSocketConnected(sock);
        }
    }
}

GSMSocket *GSMSocketManager::ConnectSocket(IPAddr ip, uint16_t port)
{
    GSMSocket *socket = socketArray->PeekSocket(ip, port);
    if (socket != NULL) {
        return socket;
    }
    if (socketArray->IsFull()) {
        return NULL;
    }

    socket = new GSMSocket(this, ip, port);
    socketArray->Append(socket);

    if (!ConnectSocketInternal(socket)) {
        socketArray->Unshift(socket);
        delete socket;
        socket = NULL;
    }
    return socket;
}

GSMSocket * GSMSocketManager::GetInitialisingSocket()
{
    for (size_t i = 0; i < socketArray->Size(); i++)
    {
        GSMSocket * socket = socketArray->Peek(i);
        if (socket == NULL) {
            return NULL;
        }
        if (socket->IsInitialising()) {
            return socket;
        }
    }
    return NULL;
}

/*
bool GSMSocketManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    return false;
}

bool GSMSocketManager::OnGSMEvent(char * data, size_t dataLen)
{
    return false;
}
*/

size_t GSMSocketManager::Send(GSMSocket *socket)
{
    // We have pending send data command, wait till it finished to not overflow command buffer
    if (pendingSockTransmission != GSM_SOCKET_ERROR_ID) return 0;
    if (socket == NULL) return 0;
    
    ByteArray *packet = socket->outgoingMessageStack.Peek();

    if (packet == NULL || packet->GetLength() == 0) return 0;

    pendingSockTransmission = socket->GetId();

    if (!SendInternal(socket, packet)){
        pendingSockTransmission = GSM_SOCKET_ERROR_ID;
        return 0;
    }
    // If send added to stack, unshift packet from pending array
    socket->outgoingMessageStack.Unshift(packet);
    return packet->GetLength();
}

bool GSMSocketManager::CloseSocket(uint8_t socketId)
{
    GSMSocket *sock = socketArray->PeekSocket(socketId);
    if (sock == NULL) return false;

    return sock->Close();
}

size_t GSMSocketManager::SendNextAvailableData()
{
    GSMSocket *sock = NULL;
    pendingSockTransmission++; // Switch to the next socket
    if (pendingSockTransmission >= socketArray->MaxSize()) {
        pendingSockTransmission = 0;
    }

    // Check next available socket
    for(uint8_t i = pendingSockTransmission; i < socketArray->MaxSize(); i++) {
        sock = socketArray->PeekSocket(i);
        if (sock == NULL || !sock->IsConnected()) {
            continue;
        }
        if (sock->outgoingMessageStack.Size() > 0) {
            pendingSockTransmission = GSM_SOCKET_ERROR_ID;
            return Send(sock);
        }
    }
    // Check next socket with lower ID
    for(uint8_t i = 0; i <= pendingSockTransmission; i++) {
        sock = socketArray->PeekSocket(i);
        if (sock == NULL || !sock->IsConnected()) {
            continue;
        }
        if (sock->outgoingMessageStack.Size() > 0) {
            pendingSockTransmission = GSM_SOCKET_ERROR_ID;
            return Send(sock);
        }
    }
    // No data to send
    pendingSockTransmission = GSM_SOCKET_ERROR_ID;
    return 0;
}

bool GSMSocketManager::DestroySocket(uint8_t socketId)
{
    GSMSocket *sock = socketArray->UnshiftSocket(socketId);
    Serial.print("GSMSocketManager::DestroySocket ");
    if (sock == NULL) {
        Serial.println(" NULL");
        return false;
    }
    Serial.println(" OK");
    if (socketHandler != NULL && sock != NULL) {
        socketHandler->OnSocketClose(sock);
    }
    if (sock->GetState() != GSM_SOCKET_STATE_CLOSING) {
        Close(sock->GetId());
    }
    socketArray->FreeItem(sock);
    return true;
}

void GSMSocketManager::OnModemReboot()
{
    pendingSockTransmission = GSM_SOCKET_ERROR_ID;
    while(socketArray->Size() > 0) {
        DestroySocket(socketArray->Peek(0)->GetId());
    }
}

GSMSocket * GSMSocketManager::GetSocket(uint8_t socketId)
{
    return socketArray->PeekSocket(socketId);
}

void GSMSocketManager::PrintDebug(Print *stream)
{
    stream->print(PSTR("SH pendTR: "));
    stream->print(pendingSockTransmission);
    stream->print('\n');
    
    //stream->print('\n');
    //stream->print(PSTR("SH sock amount: "));
    //stream->print(socketArray->Size());

    for (size_t i = 0; i < socketArray->Size(); i++)
    {
        stream->print('\n');
        GSMSocket *sock = socketArray->Peek(i);
        stream->print((int)i+1);
        stream->print(PSTR(") SH sock: "));
        stream->print(sock->GetId());
        stream->print("->");
        stream->print((int)sock->GetState());
    }
}

void GSMSocketManager::OnSocketClosed(uint8_t socketId)
{
    Serial.println("GSMSocketManager::OnSocketClosed");
    DestroySocket(socketId);
    if (pendingSockTransmission == socketId) {
        SendNextAvailableData();
    }
}


void GSMSocketManager::OnSocketData(uint8_t socketId, uint8_t *data, size_t len)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) return;

    if (socketHandler != NULL) {
        socketHandler->OnSocketData(sock, data, len);
    }
}

void GSMSocketManager::OnKeepAliveConfirm(uint8_t socketId)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) return;
    sock->OnKeepAliveConfirm();
}
void GSMSocketManager::OnSSLConfirm(uint8_t socketId)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) return;
    sock->OnSSLConfirm();
}

bool GSMSocketManager::IsMaxCreated() { 
    return socketArray->IsFull(); 
}


uint8_t GSMSocketManager::GetNextAvailableSocketIndex()
{
    for (size_t i = 0; i < socketArray->MaxSize(); i++)
    {
        GSMSocket *sock = socketArray->PeekSocket(i);
        if (sock == NULL) {
            return i;
        }
    }
    return GSM_SOCKET_ERROR_ID;
}

void GSMSocketManager::CloseAll()
{
    for (int i = (int)socketArray->Size() - 1; i >= 0 ; i--)
    {
        GSMSocket *sock = socketArray->Peek(i);
        if (sock != NULL) {
            CloseSocket(sock->GetId());
        }
    }
}

void GSMSocketManager::SetExpectingResponse(uint8_t socketId, size_t length)
{
    gsmManager->SetExpectFixedLength(length, 100000ul);
    receivingSocketId = socketId;
}

bool GSMSocketManager::OnGSMExpectedData(uint8_t * data, size_t dataLen)
{
    if (receivingSocketId != GSM_SOCKET_ERROR_ID) {
        uint8_t id = receivingSocketId;
        receivingSocketId = GSM_SOCKET_ERROR_ID;
        OnSocketData(id, data, dataLen);
        return true;
    }
    return false;
}