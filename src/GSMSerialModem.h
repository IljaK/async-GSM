#pragma once
#include "serial/SerialCharResponseHandler.h"
#include "command/BaseModemCMD.h"

#define MODEM_SERIAL_BUFFER_SIZE 1024

constexpr char GSM_PREFIX_CMD[] = "AT";
constexpr char GSM_OK_RESPONSE[] = "OK";
constexpr char GSM_ERROR_RESPONSE[] = "ERROR";
constexpr char GSM_ABORTED_RESPONSE[] = "ABORTED";
constexpr unsigned long GSM_EVENT_BUFFER_TIMEOUT = 100000ul;

enum MODEM_RESPONSE_TYPE {
    MODEM_RESPONSE_EVENT,
    MODEM_RESPONSE_DATA,
    MODEM_RESPONSE_OK,
    MODEM_RESPONSE_ERROR,
    MODEM_RESPONSE_ABORTED,
    MODEM_RESPONSE_TIMEOUT,
    MODEM_RESPONSE_OVERFLOW,
    MODEM_RESPONSE_CANCELED
};

class GSMSerialModem : public SerialCharResponseHandler
{
protected:
    BaseModemCMD *pendingCMD = NULL;
    TimerID eventBufferTimeout = 0;
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

    bool Send(BaseModemCMD *modemCMD);
};