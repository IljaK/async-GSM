#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketHandler.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"


TEST(GSMModemTest, TimeoutResponseTest)
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
    gsmHandler.Loop();

    EXPECT_FALSE(gsmHandler.IsBusy());
}

TEST(GSMModemTest, CMDTimeoutConfirmTest)
{
	timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler;
    gsmHandler.SetReady();

    ASSERT_FALSE(gsmHandler.IsBusy());

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 10));

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_TRUE(gsmHandler.IsBusy());

    gsmHandler.ReadResponse((char*)"\r\n+TEST_CMD: some data\r\n");

    BaseModemCMD *cmd = gsmHandler.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=5;

    gsmHandler.ReadResponse((char*)"\r\n");

    TimerMock::Loop();
    gsmHandler.Loop();
    EXPECT_TRUE(gsmHandler.IsBusy());

    timeOffset += GSM_BUFFER_FILL_TIMEOUT;

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_FALSE(gsmHandler.IsBusy());
}