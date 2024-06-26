#pragma once
#include <Arduino.h>
#include "command/ModemCommandStack.h"
#include "common/Timer.h"
#include "GSMSerialModem.h"
#include "command/BaseModemCMD.h"
#include "command/ULongModemCMD.h"
#include "command/ByteModemCMD.h"

#define OUT_MESSAGE_STACK_SIZE 12

constexpr char GSM_MODEM_SPEED_CMD[] = "+IPR"; // AT+IPR=9600
constexpr char GSM_MODEM_CME_ERR_CMD[] = "+CMEE"; // ERROR reporting mode

#if defined(MODEM_DEBUG_DETAILS)

// constexpr char GSM_MODEM_I_CMD[] = "I"; // Returns some module information as the module type number and some details about the firmware version.
constexpr char GSM_MODEM_GMI_CMD[] = "+GMI"; // Manufacturer identification
constexpr char GSM_MODEM_CGMI_CMD[] = "+CGMI"; // Manufacturer identification
constexpr char GSM_MODEM_GMM_CMD[] = "+GMM"; // Model identification
constexpr char GSM_MODEM_CGMM_CMD[] = "+CGMM"; // Model identification
constexpr char GSM_MODEM_GMR_CMD[] = "+GMR"; // Firmware version identification
constexpr char GSM_MODEM_CGMR_CMD[] = "+CGMR"; // Firmware version identification

// Manufacturer identification +CGMI
// Model identification +GMM It is identical with AT+CGMM.
// Model identification +CGMM  It is identical with AT+GMM.
// Firmware version identification +GMR  It is identical with AT+CGMR.
// Firmware version identification +CGMR  It is identical with AT+GMR

// Request product serial number identification +CGSN
// IMEI identification +GSN It is identical with AT+CGSN
#endif


// Timeout for getting proper response for AT command (on modem initialization + modem speed change)
constexpr unsigned long GSM_MODEM_INIT_TIMEOUT = 20000000ul;
constexpr unsigned long GSM_MODEM_SIM_INIT_TIMEOUT = 30000000ul;

constexpr unsigned long GSM_MODEM_SIM_PIN_DELAY = 500000ul;
constexpr unsigned long MODEM_BOOT_COMMAND_TIMEOUT = 100000ul;

constexpr unsigned long GSM_STATUS_CHECK_DELAY = 2000000ul;

class IModemBootHandler
{
public:
    virtual void OnModemStartReboot() = 0;
    virtual void OnModemBooted() = 0;
    virtual void OnModemFailedBoot() = 0;
};

class IBaseGSMHandler
{
public:
    virtual bool OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type) = 0;
    virtual bool OnGSMEvent(char * data, size_t dataLen) = 0;
    virtual bool OnGSMExpectedData(uint8_t * data, size_t dataLen) { return false; };
};

enum MODEM_BOOT_STATE {
    MODEM_BOOT_IDLE,
    MODEM_BOOT_RESET,
    MODEM_BOOT_CONNECTING,

#if defined(MODEM_DEBUG_DETAILS)
    MODEM_BOOT_FIRMWARE_INFO,
#endif

    MODEM_BOOT_SPEED_CHAGE,
    MODEM_BOOT_RECONNECTING,
    MODEM_BOOT_DEBUG_SET,
    MODEM_BOOT_COMPLETED,
    MODEM_BOOT_ERROR,
};

class GSMModemManager: public IModemBootHandler, public IBaseGSMHandler, public GSMSerialModem
{
protected:
    MODEM_BOOT_STATE modemBootState = MODEM_BOOT_IDLE;

    void OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type) override;
    void OnGSMResponseInternal(BaseModemCMD *cmd, char * response, size_t respLen, MODEM_RESPONSE_TYPE type);

    virtual void ReBootModem(bool hardReset = true);
    virtual void BootModem();

private:
    uint32_t initBaudRate = 115200ul;
    uint32_t targetBaudRate = 115200ul;
    uint32_t serialConfig = SERIAL_8N1;

    Timer modemRebootTimer;
    ModemCommandStack commandStack;

    bool ForceCommandInternal(BaseModemCMD *cmd);
    inline void InitSpeedChange();

public:
    GSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~GSMModemManager();

    void SetConfig(uint32_t initBaudRate, uint32_t targetBaudRate, uint32_t serialConfig);
    void StartModem();

	void OnTimerComplete(Timer * timer) override;

    // Append command to request stack
    virtual bool AddCommand(BaseModemCMD *cmd);
    // Insert command to begin of request stack
    virtual bool ForceCommand(BaseModemCMD *cmd);

    Stream *GetModemStream();

    void Loop() override;
    void Flush();

    bool IsBooted();

    size_t GetCommandStackCount();
};