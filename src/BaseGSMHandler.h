#pragma once
#include <Arduino.h>
#include "array/StringStackArray.h"
#include "common/Timer.h"
#include "GSMSerialModem.h"

#define OUT_MESSAGE_STACK_SIZE 12

constexpr char GSM_MODEM_SPEED_CMD[] = "+IPR";

constexpr unsigned long GSM_MODEM_CONNECTION_TIME = 30000000ul;
constexpr unsigned long MODEM_COMMAND_TIMEOUT = 200000ul;
constexpr unsigned long MODEM_BOOT_COMMAND_TIMEOUT = 100000ul;

constexpr unsigned long MODEM_MAX_AUTO_BAUD_RATE = 115200ul;
constexpr unsigned long MODEM_BAUD_RATE = 921600ul;

class IModemBootHandler
{
public:
    virtual void OnModemBooted() = 0;
    virtual void OnModemFailedBoot() = 0;
    // TODO: check modem state
};

class IBaseGSMHandler
{
public:
    virtual bool OnGSMResponse(char *request, char * response, MODEM_RESPONSE_TYPE type) = 0;
    virtual bool OnGSMEvent(char * data) = 0;
};

enum MODEM_BOOT_STATE {
    MODEM_BOOT_IDLE,
    MODEM_BOOT_RESET,
    MODEM_BOOT_CONNECTING,
    MODEM_BOOT_SPEED_CHAGE,
    MODEM_BOOT_RECONNECTING,
    MODEM_BOOT_COMPLETED,
    MODEM_BOOT_ERROR,
};


class BaseGSMHandler: public IModemBootHandler, public IBaseGSMHandler, public GSMSerialModem
{
protected:
    MODEM_BOOT_STATE modemBootState = MODEM_BOOT_IDLE;

    void OnModemResponse(char *data, MODEM_RESPONSE_TYPE type) override;
    void OnGSMResponseInternal(char * response, MODEM_RESPONSE_TYPE type);

private:
    char *pengingRequest = NULL;
    unsigned long baudRate = 0;
    TimerID connectionTimer = 0;
    StringStackArray commandStack;
public:
    BaseGSMHandler();
    virtual ~BaseGSMHandler();

    void StartModem(bool restart, unsigned long baudRate = MODEM_BAUD_RATE);

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;
    //void handleUrc(const String& urc) override;

    // Append command to request stack
    bool AddCommand(const char *command);
    // Insert command to begin of request stack
    bool ForceCommand(const char *command);

    void Loop() override;
    void Flush();
};