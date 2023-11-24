#pragma once
#include "serial/SerialCharResponseHandler.h"
#include "command/BaseModemCMD.h"

#define MODEM_SERIAL_BUFFER_SIZE 1024

constexpr char GSM_PREFIX_CMD[] = "AT";
constexpr char GSM_OK_RESPONSE[] = "OK";
constexpr char GSM_ERROR_RESPONSE[] = "ERROR";
constexpr char GSM_ABORTED_RESPONSE[] = "ABORTED";
constexpr char GSM_CME_ERROR_RESPONSE[] = "+CME ERROR:";
constexpr unsigned long GSM_BUFFER_FILL_TIMEOUT = 100000ul;

#define CME_ERROR_NOT_ALLOWED 3
#define CME_ERROR_NOT_SUPPORTED 4
#define CME_ERROR_NO_SIM 10
#define CME_ERROR_SIM_PIN 11
#define CME_ERROR_SIM_PUK 12
#define CME_ERROR_SIM_FAIL 13
#define CME_ERROR_SIM_BUSY 14
#define CME_ERROR_SIM_WRONG 15
#define CME_ERROR_WRONG_PASSWORD 16
#define CME_ERROR_SIM_PIN2 17
#define CME_ERROR_SIM_PUK2 18

// The DTE shall wait some time (the recommended value is at least 20 ms) after the reception of an
// AT command final response or URC before issuing a new AT command to give the module the
// opportunity to transmit the buffered URCs. Otherwise the collision of the URCs with the
// subsequent AT command is still possible
constexpr unsigned long GSM_DATA_COLLISION_DELAY = 30000ul;

enum MODEM_RESPONSE_TYPE {
    MODEM_RESPONSE_EVENT,
    MODEM_RESPONSE_DATA,
    MODEM_RESPONSE_OK,
    MODEM_RESPONSE_ERROR,
    MODEM_RESPONSE_ABORTED,
    MODEM_RESPONSE_CANCELED,
    MODEM_RESPONSE_OVERFLOW,
    MODEM_RESPONSE_TIMEOUT
};

class GSMSerialModem : protected SerialCharResponseHandler
{
protected:
    int8_t resetPin = -1;
    BaseModemCMD *pendingCMD = NULL;
    TimerID eventBufferTimeout = 0;
    TimerID cmdReleaseTimer = 0;
    Print *debugPrint = NULL;
    
	void OnResponseReceived(bool IsTimeOut, bool isOverFlow = false) override;
	virtual void OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type) = 0;
    void FlushIncoming();
    void StartEventBufferTimer();
    //void Reboot(uint32_t baud, uint32_t config, bool hardReset);
    HardwareSerial *GetSerial();
    // For Uart pin reassignment
    virtual void ResetSerial(uint32_t baud, uint32_t config);

public:
    GSMSerialModem(HardwareSerial *serial, int8_t resetPin = -1);
    virtual ~GSMSerialModem();

    void Loop() override;

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

	//void FlushData() override;
	bool IsBusy() override;
    void SetDebugPrint(Print *debugPrint);

    bool virtual Send(BaseModemCMD *modemCMD);

    
    // Helper for ublox sara u2 modem
    //void virtual BootSaraU2(bool hardReset);
    //void virtual BootSimA7670(bool hardReset);
};