#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketHandler.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"


TEST(GSMSocketHandlerTest, SocketConnectionTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler.Loop();

    ASSERT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+USOCR: 0\r\n");
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);

    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSEC=0,1,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // AT+USOSO=0,65535,8,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // AT+USOSO=0,6,2,30000
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");
    

    EXPECT_TRUE(socket->IsConnected());
}

TEST(GSMSocketHandlerTest, SocketConnectionFailTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+USOCR: 0\r\n");
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);
    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSEC=0,1,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // AT+USOSO=0,65535,8,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");
    
    // AT+USOSO=0,6,2,30000
    gsmHandler.ReadResponse((char*)"\r\nERROR\r\n");

    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: 0\r\n");

    //timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    //Timer::Loop();
    
    socket = socketHandler->GetSocket(0);

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
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+USOCR: 0\r\n");
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);
    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSO=0,65535,8,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");
    
    // AT+USOSEC=0,1,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\nER\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: \r\n");

    timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    socket = socketHandler->GetSocket(0);

    EXPECT_TRUE(socket == NULL);
}

TEST(GSMSocketHandlerTest, SocketConnectionCorruptionFailTest2)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+USOCR: 0\r\n");
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);
    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSO=0,65535,8,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");
    
    // AT+USOSEC=0,1,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\n+UUSOCL: \r\n");

    // Socket must be in list before timeout
    socket = socketHandler->GetSocket(0);
    EXPECT_TRUE(socket == NULL);
}

TEST(GSMSocketHandlerTest, SocketConnectionCorruptionFailTest3)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    GSMSocketHandler *socketHandler;
    socketHandler = gsmHandler.GetSocketHandler();

    gsmHandler.SetReady();
    socketHandler->CreateSocket();
    // AT+USOCR=6
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+USOCR: 0\r\n");
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    GSMSocket *socket = socketHandler->GetSocket(0);
    ASSERT_FALSE(socket == NULL);
    EXPECT_EQ(socket->GetId(), 0);

    socket->Connect((char*)"127.0.0.1", 2234, true);

    // AT+USOSO=0,65535,8,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");
    
    // AT+USOSEC=0,1,1
    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    // Corrupted response
    gsmHandler.ReadResponse((char*)"\r\nERROR\r\n");

    // Socket must be in list before timeout
    socket = socketHandler->GetSocket(0);
    EXPECT_FALSE(socket == NULL);

    timeOffset += SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT;
    Timer::Loop();
    gsmHandler.Loop();
    
    socket = socketHandler->GetSocket(0);

    EXPECT_TRUE(socket == NULL);
}