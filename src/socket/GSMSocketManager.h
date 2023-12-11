#pragma once
#include "GSMModemManager.h"
#include "SocketArray.h"
#include "GSMSocket.h"
#include "IGSMSocketManager.h"
#include "command/BaseModemCMD.h"
#include "array/ByteStackArray.h"
#include "common/GSMUtils.h"

constexpr unsigned long SOCKET_CONNECTION_TIMEOUT = 60000000ul;
constexpr unsigned long SOCKET_CMD_TIMEOUT = 10000000ul;


//constexpr uint16_t GSM_SOCKET_BUFFER_SIZE = (MODEM_SERIAL_BUFFER_SIZE / 2) - 20;
#define GSM_SOCKET_BUFFER_SIZE 256

class SocketArray;

class GSMSocketManager: public IBaseGSMHandler
{
private:
    SocketArray *socketArray = NULL;
    IGSMSocketManager *socketHandler = NULL;
    uint8_t pendingSockTransmission = 255; // 255 = NONE
    bool DestroySocket(uint8_t socketId);
protected:
    GSMModemManager *gsmManager = NULL;
    friend class GSMSocket;
    
    virtual bool Connect(GSMSocket *socket) = 0;
    virtual bool SetKeepAlive(GSMSocket *socket) = 0;
    virtual bool SetSSL(GSMSocket *socket) = 0;
    virtual bool Close(uint8_t socketId) = 0;
    size_t Send(GSMSocket *socket);
    virtual bool SendInternal(GSMSocket *socket, ByteArray *packet) = 0;
    size_t SendNextAvailableData();

    void OnSSLConfirm(uint8_t socketId);
    void OnKeepAliveConfirm(uint8_t socketId);
    void OnSocketData(uint8_t socketId, uint8_t *data, size_t len);
    void OnSocketClosed(uint8_t socketId);
    GSMSocket * OnSocketCreated(uint8_t socketId);
    void OnSocketConnection(uint8_t socketId, GSM_SOCKET_ERROR error);

    GSMSocket * GetInitialisingSocket();
    uint8_t GetNextAvailableSocketIndex();

    // Start Socket connection
    virtual bool ConnectSocketInternal(GSMSocket *socket) = 0;

public:
    GSMSocketManager(GSMModemManager *gsmManager, uint8_t socketsAmount);
    virtual ~GSMSocketManager();
    void SetSocketHandler(IGSMSocketManager *socketHandler);

    //bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) override;
    //bool OnGSMEvent(char * data, size_t dataLen) override;

    //virtual bool CreateSocket() = 0;
    virtual bool CloseSocket(uint8_t socketId);

    void OnModemReboot();

    GSMSocket * GetSocket(uint8_t socketId);
    void PrintDebug(Print *stream);
    bool IsMaxCreated();

    GSMSocket *ConnectSocket(IPAddr ip, uint16_t port);
    void CloseAll();
};