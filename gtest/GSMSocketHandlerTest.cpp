#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketHandler.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"

void createConfigSocket(GSMSocketHandler *socketHandler, GSMHandlerMock *gsmHandler) {
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler->Loop();

    ASSERT_TRUE(gsmHandler->IsBusy());

    gsmHandler->ReadResponse((char*)"AT+USOCR=6\r\r\n+USOCR: 0\r\n");
    gsmHandler->ReadResponse((char*)"\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);

    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

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

void createConnectSocket(GSMSocketHandler *socketHandler, GSMHandlerMock *gsmHandler) {
    createConfigSocket(socketHandler, gsmHandler);

    //AT+USOCO=0,"127.0.0.1",2234
    gsmHandler->ReadResponse((char*)"AT+USOCO=0,\"127.0.0.1\",2234\r\r\nOK\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();
    gsmHandler->Loop();

    GSMSocket *socket = socketHandler->GetSocket(0);

    EXPECT_TRUE(socket->IsConnected());

    gsmHandler->ReadResponse((char*)"\r\n+UUSOCO: 0,0\r\n");

    EXPECT_TRUE(socket->IsConnected());
}

TEST(GSMSocketHandlerTest, SocketConnectionTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketHandler, &gsmHandler);
}

TEST(GSMSocketHandlerTest, SocketConnectionFailTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    
    createConnectSocket(socketHandler, &gsmHandler);

    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: 0\r\n");

    //timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    GSMSocket *socket = socketHandler->GetSocket(0);
    EXPECT_TRUE(socket == NULL);
}

TEST(GSMSocketHandlerTest, SocketConnectionCorruptionFailTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    createConfigSocket(socketHandler, &gsmHandler);

    // Corrupted response
    gsmHandler.ReadResponse((char*)"AT+USOCO=0,\"127.0.0.1\",2234\r\r\nER\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: \r\n");

    timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    GSMSocket *socket = socketHandler->GetSocket(0);

    EXPECT_TRUE(socket == NULL);
}

TEST(GSMSocketHandlerTest, SocketWriteErrorTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketHandler, &gsmHandler);
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();

    GSMSocket * socket = socketHandler->GetSocket(0);

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

    socket = socketHandler->GetSocket(0);

    ASSERT_TRUE(socket == NULL);
    
}


TEST(GSMSocketHandlerTest, SocketWriteErrorTimeoutCloseTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    createConnectSocket(socketHandler, &gsmHandler);

    GSMSocket * socket = socketHandler->GetSocket(0);

    ASSERT_FALSE(socket == NULL);

    uint8_t data[256] = { 128 };

    socket->SendData(data, 256);

    // Write socket error
    gsmHandler.ReadResponse((char*)"AT+USOWR=0,2,\"9291\"\r\r\nERROR\r\n");
    timeOffset += GSM_DATA_COLLISION_DELAY;
    Timer::Loop();
    gsmHandler.Loop();

    // Close socket timeout
    timeOffset += SOCKET_CONNECTION_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();

    socket = socketHandler->GetSocket(0);

    ASSERT_TRUE(socket == NULL);
    
}