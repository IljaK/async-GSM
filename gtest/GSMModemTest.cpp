#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketManager.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"


TEST(GSMModemTest, BaseModemCMDTest)
{
    BaseModemCMD *cmd = new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT);

    EXPECT_TRUE(cmd->cmd == NULL);

    delete cmd;
}