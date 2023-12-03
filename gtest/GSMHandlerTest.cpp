#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketManager.h"
#include "mock/UbloxGSMHandlerMock.h"

void urcCollisionCooldownCheck(GSMSerialModem *gsmManager) {
    ASSERT_TRUE(gsmManager->IsBusy());

    timeOffset += GSM_DATA_COLLISION_DELAY - 1;

    //Timer::Loop();
    gsmManager->Loop();

    ASSERT_TRUE(gsmManager->IsBusy());

    timeOffset += 1;

    Timer::Loop();
    gsmManager->Loop();

    ASSERT_FALSE(gsmManager->IsBusy());
}

TEST(GSMHandlerTest, EventReadTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"+TEST_EVENT: blah\r\n");

    ASSERT_STREQ(gsmManager.GetLastEvent(), "+TEST_EVENT: blah");

    urcCollisionCooldownCheck(&gsmManager);
}

TEST(GSMHandlerTest, EventFillTimeoutTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"\r\n+TEST_EVENT: ");

    ASSERT_TRUE(gsmManager.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT / 2;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_TRUE(gsmManager.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_STREQ(gsmManager.GetLastEvent(), "+TEST_EVENT: ");

    ASSERT_FALSE(gsmManager.IsBusy());
}

TEST(GSMHandlerTest, ResponseReadTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_EQ(gsmManager.GetCommandStackCount(), 0);
    ASSERT_TRUE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"AT+TEST_CMD\r\r\n+TEST_CMD: blah\r\n");

    ASSERT_STREQ(gsmManager.GetLastResponse(), "+TEST_CMD: blah");

    gsmManager.ReadResponse((char*)"\r\nOK\r\n");

    urcCollisionCooldownCheck(&gsmManager);
}

TEST(GSMHandlerTest, EventTimeoutAndResponseTest)
{
    timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"\r\n+TEST_EVENT: ");

    Timer::Loop();
    gsmManager.Loop();
    ASSERT_TRUE(gsmManager.IsBusy());

    gsmManager.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_EQ(gsmManager.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"blah");

    ASSERT_EQ(gsmManager.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmManager.IsBusy());

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_EQ(gsmManager.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmManager.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT / 2;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_EQ(gsmManager.GetCommandStackCount(), 1);
    ASSERT_TRUE(gsmManager.IsBusy());

    timeOffset = GSM_BUFFER_FILL_TIMEOUT;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_TRUE(gsmManager.IsBusy());
    ASSERT_EQ(gsmManager.GetCommandStackCount(), 0);

    gsmManager.ReadResponse((char*)"AT+TEST_CMD\r\r\nOK\r\n");

    Timer::Loop();
    gsmManager.Loop();

    urcCollisionCooldownCheck(&gsmManager);
}

TEST(GSMHandlerTest, TimeoutResponseTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 1));

    Timer::Loop();
    gsmManager.Loop();

    EXPECT_TRUE(gsmManager.IsBusy());

    BaseModemCMD *cmd = gsmManager.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=1;

    Timer::Loop();
    // Will trigger next command
    // gsmManager.Loop();
    
    EXPECT_FALSE(gsmManager.IsBusy());
}

TEST(GSMHandlerTest, TimeoutResponseDataTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ForceCommand(new ByteModemCMD(1, "+TEST_CMD", 10));

    Timer::Loop();
    gsmManager.Loop();

    EXPECT_TRUE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"AT+TEST_CMD\r\r\n+TEST_CMD: some data\r\n");

    BaseModemCMD *cmd = gsmManager.GetPendingCMD();

    EXPECT_TRUE(cmd != NULL);

    timeOffset+=5;

    gsmManager.ReadResponse((char*)"\r\n");

    Timer::Loop();
    gsmManager.Loop();
    EXPECT_TRUE(gsmManager.IsBusy());

    // CMD timer has been restarted, so +10
    timeOffset += 10;

    Timer::Loop();
    EXPECT_TRUE(gsmManager.IsBusy());

    // Prevent reboot modem and send AT cmd after timeout
    // gsmManager.Loop();
    gsmManager.ReadResponse((char*)"OK\r\n");
    gsmManager.Loop();
    timeOffset += 10;
    Timer::Loop();

    EXPECT_FALSE(gsmManager.IsBusy());
}

TEST(GSMHandlerTest, CMDResponseEventCollisionTest)
{
	timeOffset = 0;

    UbloxGSMHandlerMock gsmManager;
    gsmManager.SetReady();

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());

    gsmManager.ForceCommand(new ByteModemCMD(1, "+TEST_CMD"));

    Timer::Loop();
    gsmManager.Loop();

    EXPECT_TRUE(gsmManager.IsBusy());

    gsmManager.ReadResponse((char*)"\r\n+UREG: 6\r\n");

    ASSERT_STREQ(gsmManager.GetLastEvent(), "+UREG: 6");

    ASSERT_STREQ(gsmManager.GetLastResponse(), NULL);

    BaseModemCMD *cmd = gsmManager.GetPendingCMD();
    
    EXPECT_TRUE(gsmManager.IsBusy());

    Timer::Loop();
    EXPECT_TRUE(gsmManager.IsBusy());

    timeOffset += GSM_BUFFER_FILL_TIMEOUT - 1;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_TRUE(gsmManager.IsBusy());

    timeOffset += 1;

    Timer::Loop();
    gsmManager.Loop();

    ASSERT_FALSE(gsmManager.IsBusy());
}