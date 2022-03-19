#pragma once
#include "serial/SerialCharResponseHandler.h"
#include "command/BaseModemCMD.h"

// For debugging modem command errors
#define GSM_DEBUG_ERROR_CMD

#define MODEM_SERIAL_BUFFER_SIZE 1024

constexpr char GSM_PREFIX_CMD[] = "AT";
constexpr char GSM_OK_RESPONSE[] = "OK";
constexpr char GSM_ERROR_RESPONSE[] = "ERROR";
constexpr char GSM_ABORTED_RESPONSE[] = "ABORTED";
#ifdef GSM_DEBUG_ERROR_CMD
constexpr char GSM_CME_ERROR_RESPONSE[] = "+CME ERROR:";
#endif
constexpr unsigned long GSM_BUFFER_FILL_TIMEOUT = 100000ul;

// The DTE shall wait some time (the recommended value is at least 20 ms) after the reception of an
// AT command final response or URC before issuing a new AT command to give the module the
// opportunity to transmit the buffered URCs. Otherwise the collision of the URCs with the
// subsequent AT command is still possible
constexpr unsigned long GSM_CMD_URC_COLLISION_DELAY = 30000ul;

#ifdef ESP32
// Produino GSM board
#define GSM_RTS    (27u)
#define GSM_CTS    (35u)
#define GSM_RESETN (14u)
#define GSM_DTR    (26u)
#define GSM_RX    (34u)
#define GSM_TX    (25u)
#define SerialGSM Serial2
#endif

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

class GSMSerialModem : public SerialCharResponseHandler
{
protected:
    BaseModemCMD *pendingCMD = NULL;
    TimerID eventBufferTimeout = 0;
    TimerID cmdReleaseTimer = 0;
    Print *debugPrint = NULL;
    
	void OnResponseReceived(bool IsTimeOut, bool isOverFlow = false) override;
	virtual void OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type) = 0;
    void FlushIncoming();
public:
    GSMSerialModem();
    virtual ~GSMSerialModem();

    void Loop() override;

	void OnTimerComplete(TimerID timerId, uint8_t data) override;
	void OnTimerStop(TimerID timerId, uint8_t data) override;

	//void FlushData() override;
	bool IsBusy() override;
    void SetDebugPrint(Print *debugPrint);

    bool virtual Send(BaseModemCMD *modemCMD);
};