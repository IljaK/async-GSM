#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketManager.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"

void urcCollisionCooldownCheck(GSMHandler *gsmHandler) {
    ASSERT_TRUE(gsmHandler->IsBusy());

    timeOffset += GSM_DATA_COLLISION_DELAY - 1;

    TimerMock::Loop();
    gsmHandler->Loop();

    ASSERT_TRUE(gsmHandler->IsBusy());

    timeOffset += 1;

    TimerMock::Loop();
    gsmHandler->Loop();

    ASSERT_FALSE(gsmHandler->IsBusy());
}

TEST(GSMHandlerTest, EventReadTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"+TEST_EVENT: blah\r\n");

    ASSERT_STREQ(gsmHandler.GetLastEvent(), "+TEST_EVENT: blah");

    urcCollisionCooldownCheck(&gsmHandler);
}

TEST(GSMHandlerTest, EventFillTimeoutTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+TEST_EVENT: ");

    ASSERT_TRUE(gsmHandler.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT / 2;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_TRUE(gsmHandler.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_STREQ(gsmHandler.GetLastEvent(), "+TEST_EVENT: ");

    ASSERT_FALSE(gsmHandler.IsBusy());
}

TEST(GSMHandlerTest, ResponseReadTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 0);
    ASSERT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"AT+TEST_CMD\r\r\n+TEST_CMD: blah\r\n");

    ASSERT_STREQ(gsmHandler.GetLastResponse(), "+TEST_CMD: blah");

    gsmHandler.ReadResponse((char*)"\r\nOK\r\n");

    urcCollisionCooldownCheck(&gsmHandler);
}

TEST(GSMHandlerTest, EventTimeoutAndResponseTest)
{
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+TEST_EVENT: ");

    TimerMock::Loop();
    gsmHandler.Loop();
    ASSERT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"blah");

    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmHandler.IsBusy());

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmHandler.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT / 2;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmHandler.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_TRUE(gsmHandler.IsBusy());
    ASSERT_EQ(gsmHandler.GetCommandStackCount(), 0);

    gsmHandler.ReadResponse((char*)"AT+TEST_CMD\r\r\nOK\r\n");

    TimerMock::Loop();
    gsmHandler.Loop();

    urcCollisionCooldownCheck(&gsmHandler);
}

TEST(GSMHandlerTest, TimeoutResponseTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_TRUE(gsmHandler.IsBusy());

    BaseModemCMD *cmd = gsmHandler.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=1;

    TimerMock::Loop();
    // Will trigger next command
    // gsmHandler.Loop();
    
    EXPECT_FALSE(gsmHandler.IsBusy());
}

TEST(GSMHandlerTest, TimeoutResponseDataTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 10));

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"AT+TEST_CMD\r\r\n+TEST_CMD: some data\r\n");

    BaseModemCMD *cmd = gsmHandler.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=5;

    gsmHandler.ReadResponse((char*)"\r\n");

    TimerMock::Loop();
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    // CMD timer has been restarted, so +10
    timeOffset += 10;

    TimerMock::Loop();
    // Prevent reboot modem and send AT cmd after timeout
    // gsmHandler.Loop();

    EXPECT_FALSE(gsmHandler.IsBusy());
}

TEST(GSMHandlerTest, CMDResponseEventCollisionTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD"));

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+UREG: 6\r\n");

    ASSERT_STREQ(gsmHandler.GetLastResponse(), GSM_ERROR_RESPONSE);

    BaseModemCMD *cmd = gsmHandler.GetPendingCMD();
    
    EXPECT_TRUE(gsmHandler.IsBusy());

    EXPECT_TRUE(cmd == NULL);

    TimerMock::Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    timeOffset += GSM_BUFFER_FILL_TIMEOUT - 1;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_TRUE(gsmHandler.IsBusy());

    timeOffset += 1;

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());
}