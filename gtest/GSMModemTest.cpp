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

    gsmHandler.ForceCommand(new ByteModemCMD(1, "+CREG", 1));

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_EQ(gsmHandler.IsBusy(), true);

    BaseModemCMD *cmd = gsmHandler.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=1;

    TimerMock::Loop();
    gsmHandler.Loop();

    EXPECT_EQ(gsmHandler.IsBusy(), false);

}