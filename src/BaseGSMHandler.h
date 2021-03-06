#pragma once
#include <Arduino.h>
#include "command/ModemCommandStack.h"
#include "common/Timer.h"
#include "GSMSerialModem.h"
#include "command/BaseModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ByteModemCMD.h"

#define OUT_MESSAGE_STACK_SIZE 12
#define MAX_PHONE_LENGTH 20

constexpr char GSM_MODEM_SPEED_CMD[] = "+IPR"; // AT+IPR=9600
constexpr char GSM_MODEM_CME_ERR_CMD[] = "+CMEE"; // ERROR reporting mode

constexpr unsigned long GSM_MODEM_CONNECTION_TIME = 30000000ul;
constexpr unsigned long MODEM_BOOT_COMMAND_TIMEOUT = 100000ul;

constexpr unsigned long MODEM_MAX_AUTO_BAUD_RATE = 115200ul;
constexpr unsigned long MODEM_BAUD_RATE = 921600ul;

constexpr unsigned long GSM_STATUS_CHECK_DELAY = 2000000ul;

class IModemBootHandler
{
public:
    virtual void OnModemBooted() = 0;
    virtual void OnModemFailedBoot() = 0;
    virtual void OnModemReboot() = 0;
};

class IBaseGSMHandler
{
public:
    virtual bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) = 0;
    virtual bool OnGSMEvent(char * data, size_t dataLen) = 0;
};

enum MODEM_BOOT_STATE {
    MODEM_BOOT_IDLE,
    MODEM_BOOT_RESET,
    MODEM_BOOT_CONNECTING,
    MODEM_BOOT_SPEED_CHAGE,
    MODEM_BOOT_RECONNECTING,
    MODEM_BOOT_DEBUG_SET,
    MODEM_BOOT_COMPLETED,
    MODEM_BOOT_ERROR,
};

class BaseGSMHandler: public IModemBootHandler, public IBaseGSMHandler, public GSMSerialModem
{
protected:
    MODEM_BOOT_STATE modemBootState = MODEM_BOOT_IDLE;

    void OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type) override;
    void OnGSMResponseInternal(BaseModemCMD *cmd, char * response, size_t respLen, MODEM_RESPONSE_TYPE type);

private:
    unsigned long baudRate = 0;
    TimerID connectionTimer = 0;
    ModemCommandStack commandStack;

    bool ForceCommandInternal(BaseModemCMD *cmd);

public:
    BaseGSMHandler();
    virtual ~BaseGSMHandler();

    void StartModem(bool restart, unsigned long baudRate = MODEM_BAUD_RATE);

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

    // Append command to request stack
    virtual bool AddCommand(BaseModemCMD *cmd);
    // Insert command to begin of request stack
    virtual bool ForceCommand(BaseModemCMD *cmd);

    Stream *GetModemStream();

    void Loop() override;
    void Flush();

    bool IsBooted();

    unsigned long GetBaudRate();
    size_t GetCommandStackCount();
};