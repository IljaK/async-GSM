#include "GSMSocketManager.h"
#include "command/ByteModemCMD.h"
#include "command/Byte2ModemCMD.h"
#include "command/Byte3ModemCMD.h"
#include "command/SocketConnectCMD.h"
#include "command/CharModemCMD.h"
#include "command/ULong4ModemCMD.h"
#include "command/ByteShortModemCMD.h"
#include "command/SocketWriteModemCMD.h"
#include "command/SockeCreateModemCMD.h"

GSMSocketManager::GSMSocketManager(GSMModemManager *gsmManager, uint8_t socketsAmount)
{
    socketArray = new SocketArray(socketsAmount);
    this->gsmManager = gsmManager;
}
GSMSocketManager::~GSMSocketManager()
{
    socketArray->Clear();
    delete socketArray;
}

void GSMSocketManager::SetSocketHandler(IGSMSocketManager *socketManager)
{
    this->socketManager = socketManager;
}


void GSMSocketManager::OnSocketCreated(uint8_t socketId)
{
    if (socketId == 255) {
        socketManager->OnSocketCreateError();
        return;
    }

    GSMSocket *sock = new GSMSocket(this, socketId);
    if (!socketArray->Append(sock)) {
        sock->Close();
        delete sock;
    }
    socketManager->OnSocketOpen(sock);
}
void GSMSocketManager::OnSocketConnection(uint8_t socketId, bool isSuccess)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) {
        // Shit happens?
        DestroySocket(socketId);
        return;
    }

    sock->OnConnectionConfirm(isSuccess);
    if (socketManager != NULL) {
        socketManager->OnSocketConnected(sock, isSuccess);
    }
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
    if (pendingSockTransmission != 255) return 0;
    if (socket == NULL) return 0;
    
    ByteArray *packet = socket->outgoingMessageStack.Peek();

    if (packet == NULL || packet->length == 0) return 0;

    pendingSockTransmission = socket->GetId();

    if (!SendInternal(socket, packet)){
        pendingSockTransmission = 255;
        return 0;
    }
    // If send added to stack, unshift packet from pending array
    socket->outgoingMessageStack.UnshiftFirst();
    return packet->length;
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
            pendingSockTransmission = 255;
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
            pendingSockTransmission = 255;
            return Send(sock);
        }
    }
    // No data to send
    pendingSockTransmission = 255;
    return 0;
}

bool GSMSocketManager::DestroySocket(uint8_t socketId)
{
    GSMSocket *sock = socketArray->UnshiftSocket(socketId);
    if (sock == NULL) {
        return false;
    }
    if (socketManager != NULL) {
        socketManager->OnSocketClose(socketId, true);
    }
    socketArray->FreeItem(sock);
    return true;
}

void GSMSocketManager::OnModemReboot()
{
    pendingSockTransmission = 255;
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
    DestroySocket(socketId);
    if (pendingSockTransmission == socketId) {
        SendNextAvailableData();
    }
}


void GSMSocketManager::OnSocketData(uint8_t socketId, uint8_t *data, size_t len)
{
    GSMSocket *sock = GetSocket(socketId);
    if (sock == NULL) return;

    if (socketManager != NULL) {
        socketManager->OnSocketData(sock, data, len);
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