#include <gtest/gtest.h>
#include "common/Timer.h"
#include <Arduino.h>
#include "socket/GSMSocketHandler.h"
#include "mock/TimerMock.h"
#include "mock/GSMHandlerMock.h"

char expectedEventResult[1024];
char expectedRespResult[1024];

void eventCb(char * data, size_t dataLen) {
    if (dataLen == 0) return;
    ASSERT_STREQ(expectedEventResult, data);
}
void respCb(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) {
    if (respLen == 0) return;
    ASSERT_STREQ(expectedRespResult, response);
}

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
    expectedEventResult[0] = 0;
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler(eventCb, respCb);
    gsmHandler.SetReady();

    TimerMock::Loop();
    gsmHandler.Loop();

    ASSERT_FALSE(gsmHandler.IsBusy());

    strcpy(expectedEventResult, "+TEST_EVENT: blah");
    gsmHandler.ReadResponse((char*)"+TEST_EVENT: blah\r\n");

    urcCollisionCooldownCheck(&gsmHandler);
}

TEST(GSMHandlerTest, EventFillTimeoutTest)
{
    expectedEventResult[0] = 0;
    timeOffset = 0;
	TimerMock::Reset();

    GSMHandlerMock gsmHandler(eventCb, respCb);
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

    strcpy(expectedEventResult, "+TEST_EVENT: ");
    timeOffset = GSM_BUFFER_FILL_TIMEOUT;

    TimerMock::Loop();
    gsmHandler.Loop();

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

    strcpy(expectedRespResult, "+TEST_CMD: blah");
    gsmHandler.ReadResponse((char*)"AT+TEST_CMD\r\r\n+TEST_CMD: blah\r\n\r\nOK\r\n");

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