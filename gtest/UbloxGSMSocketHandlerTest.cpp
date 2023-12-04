#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "ublox/UbloxGSMSocketManager.h"
#include "mock/UbloxGSMHandlerMock.h"
#include "IGSMNetworkManager.h"

void createConfigSocket(UbloxGSMSocketManager *socketManager, UbloxGSMHandlerMock *gsmHandler) {
    
    IPAddr ip;
    ip.octet[0] = 123;
    ip.octet[1] = 123;
    ip.octet[2] = 123;
    ip.octet[3] = 123;
    
    socketManager->ConnectSocket(ip, 123);
    // AT+USOCR=6
    gsmHandler->Loop();

    ASSERT_TRUE(gsmHandler->IsBusy());

    gsmHandler->ReadResponse((char*)"AT+USOCR=6\r\r\n+USOCR: 0\r\n");
    gsmHandler->ReadResponse((char*)"\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    GSMSocket *socket = socketManager->GetSocket(0);
    ASSERT_FALSE(socket == NULL);

    EXPECT_EQ(socket->GetId(), 0);

    // TODO:
    // socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSO=0,65535,8,1
    gsmHandler->ReadResponse((char*)"AT+USOSO=0,65535,8,1\r\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();
    gsmHandler->Loop();

    // AT+USOSO=0,6,2,30000
    gsmHandler->ReadResponse((char*)"AT+USOSO=0,6,2,30000\r\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();
    gsmHandler->Loop();
}

void createConnectSocket(UbloxGSMSocketManager *socketManager, UbloxGSMHandlerMock *gsmHandler) {
    createConfigSocket(socketManager, gsmHandler);

    //AT+USOCO=0,"127.0.0.1",2234
    gsmHandler->ReadResponse((char*)"AT+USOCO=0,\"127.0.0.1\",2234\r\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();
    gsmHandler->Loop();

    GSMSocket *socket = socketManager->GetSocket(0);

    EXPECT_TRUE(socket->IsConnected());

    gsmHandler->ReadResponse((char*)"\r\n+UUSOCO: 0,0\r\n");

    EXPECT_TRUE(socket->IsConnected());
}

TEST(UbloxGSMSocketManagerTest, SocketConnectionTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmHandler;
    UbloxGSMSocketManager *socketManager;
    socketManager = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketManager, &gsmHandler);
}

TEST(UbloxGSMSocketManagerTest, SocketConnectionFailTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmHandler;
    UbloxGSMSocketManager *socketManager;
    socketManager = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    
    createConnectSocket(socketManager, &gsmHandler);

    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: 0\r\n");

    //timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    GSMSocket *socket = socketManager->GetSocket(0);
    EXPECT_TRUE(socket == NULL);
}

TEST(UbloxGSMSocketManagerTest, SocketConnectionCorruptionFailTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmHandler;
    UbloxGSMSocketManager *socketManager;
    socketManager = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    createConfigSocket(socketManager, &gsmHandler);

    // Corrupted response
    gsmHandler.ReadResponse((char*)"AT+USOCO=0,\"127.0.0.1\",2234\r\r\nERROR\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: \r\n");
    Timer::Loop();
    gsmHandler.Loop();

    timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    GSMSocket *socket = socketManager->GetSocket(0);

    EXPECT_TRUE(socket == NULL);
}

TEST(UbloxGSMSocketManagerTest, SocketWriteErrorTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmHandler;
    UbloxGSMSocketManager *socketManager;
    socketManager = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketManager, &gsmHandler);
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    GSMSocket * socket = socketManager->GetSocket(0);

    ASSERT_FALSE(socket == NULL);

    uint8_t data[256] = { 128 };

    socket->SendData(data, 256);

    // Write socket error
    gsmHandler.ReadResponse((char*)"AT+USOWR=0,2,\"9291\"\r\r\nERROR\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    // Close socket OK
    gsmHandler.ReadResponse((char*)"AT+USOCL=0\r\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    socket = socketManager->GetSocket(0);

    ASSERT_TRUE(socket == NULL);
    
}


TEST(UbloxGSMSocketManagerTest, SocketWriteErrorTimeoutCloseTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmHandler;
    UbloxGSMSocketManager *socketManager;
    socketManager = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketManager, &gsmHandler);

    GSMSocket * socket = socketManager->GetSocket(0);

    ASSERT_FALSE(socket == NULL);

    uint8_t data[256] = { 128 };

    socket->SendData(data, 256);

    // Write socket error
    gsmHandler.ReadResponse((char*)"AT+USOWR=0,2,\"9291\"\r\r\nERROR\r\n");
    gsmHandler.ReadResponse((char*)"AT+USOCL=0\r\r\nOK\r\n");
    gsmHandler.ReadResponse((char*)"+UUSOCL: 0\r\n");

    timeOffset += GSM_DATA_COLLISION_DELAY;

    // Close socket timeout
    //timeOffset += SOCKET_CONNECTION_TIMEOUT;
    //Timer::Loop();
    //gsmHandler.Loop();

    socket = socketManager->GetSocket(0);

    ASSERT_TRUE(socket == NULL);
    
}