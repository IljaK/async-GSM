#pragma once
#include "serial/SerialCharResponseHandler.h"

#define MODEM_SERIAL_BUFFER_SIZE 1024

constexpr char GSM_PREFIX_CMD[] = "AT";
constexpr char GSM_OK_RESPONSE[] = "OK";
constexpr char GSM_ERROR_RESPONSE[] = "ERROR";
constexpr char GSM_ABORTED_RESPONSE[] = "ABORTED";

enum MODEM_RESPONSE_TYPE {
    MODEM_RESPONSE_EVENT,
    MODEM_RESPONSE_DATA,
    MODEM_RESPONSE_OK,
    MODEM_RESPONSE_ERROR,
    MODEM_RESPONSE_ABORTED,
    MODEM_RESPONSE_TIMEOUT,
    MODEM_RESPONSE_OVERFLOW,
};

class GSMSerialModem : public SerialCharResponseHandler
{
private:
    bool isWaitingConfirm = false;
protected:
    Print *debugPrint = NULL;
	void OnResponseReceived(bool IsTimeOut, bool isOverFlow = false) override;
	virtual void OnModemResponse(char *data, MODEM_RESPONSE_TYPE type) = 0;
public:
    GSMSerialModem();
    virtual ~GSMSerialModem();

	//void FlushData() override;
	bool IsBusy() override;
    void SetDebugPrint(Print *debugPrint);

    bool Sendf(const char * cmd, unsigned long timeout, bool isCheck = false, bool isSet = false, char *data = NULL, bool dataQuotations = false, bool semicolon = false);
    bool Send(char *data, unsigned long timeout);
};