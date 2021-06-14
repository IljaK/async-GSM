#include "GSMSerialModem.h"

GSMSerialModem::GSMSerialModem():SerialCharResponseHandler(MODEM_SERIAL_BUFFER_SIZE, "\r\n", &SerialGSM)
{

}

GSMSerialModem::~GSMSerialModem()
{

}

void GSMSerialModem::OnResponseReceived(bool IsTimeOut, bool isOverFlow)
{
    if (!IsTimeOut && !isOverFlow) {
        if (bufferLength == 0) {
            return;
        }
        size_t sz = strlen(GSM_PREFIX_CMD);
        if (bufferLength < sz) {
            sz = bufferLength;
        }
        if (strncmp(buffer, GSM_PREFIX_CMD, sz) == 0) {
            return;
        }
    }

    if (debugPrint != NULL) {
        debugPrint->print("Response: [");
        debugPrint->print(buffer);
        debugPrint->print("]");
        if (IsTimeOut) {
            debugPrint->print(" true");
        } else {
            debugPrint->print(" false");
        }
        if (isOverFlow) {
            debugPrint->print(" true");
        } else {
            debugPrint->print(" false");
        }
        if (isWaitingConfirm) {
            debugPrint->println(" true ");
        } else {
            debugPrint->println(" false ");
        }
    }


    if (isWaitingConfirm) {
        MODEM_RESPONSE_TYPE type = MODEM_RESPONSE_DATA;
        if (IsTimeOut) {
            type = MODEM_RESPONSE_TIMEOUT;
        } else if (isOverFlow) {
            type = MODEM_RESPONSE_OVERFLOW;
        } else if (strcmp(buffer, GSM_OK_RESPONSE) == 0) {
            type = MODEM_RESPONSE_OK;
        } else if (strcmp(buffer, GSM_ERROR_RESPONSE) == 0) {
            type = MODEM_RESPONSE_ERROR;
        } else if (strcmp(buffer, GSM_ABORTED_RESPONSE) == 0) {
            type = MODEM_RESPONSE_ABORTED;
        }
        if (type >= MODEM_RESPONSE_OK) {
            isWaitingConfirm = false;
        }
        OnModemResponse(buffer, type);
    } else {
        OnModemResponse(buffer, MODEM_RESPONSE_EVENT);
    }
}

bool GSMSerialModem::IsBusy()
{
    return isWaitingConfirm || 
        bufferLength != 0 || 
        serial->available() != 0 || 
        SerialCharResponseHandler::IsBusy();
}

bool GSMSerialModem::Send(char *data, unsigned long timeout)
{
    if (IsBusy()) return false;
    if (data == NULL || data[0] == 0) return false;
    isWaitingConfirm = true;
    ResetBuffer();
	StartTimeoutTimer(timeout);
    serial->println(data);
    return true;
}

bool GSMSerialModem::Sendf(const char * cmd, unsigned long timeout, bool isCheck, bool isSet, char *data, bool dataQuotations, bool semicolon)
{
    if (IsBusy()) return false;
    if (data == NULL || data[0] == 0) return false;

    ResetBuffer();
    isWaitingConfirm = true;
	if (serial) {
		serial->write(GSM_PREFIX_CMD);
		
		if (cmd != NULL) {
			serial->write(cmd);
		}
		if (isCheck) {
			serial->write('?');
		}
		else if (isSet) {
			serial->write('=');
		}
		if (data != NULL) {
			if (dataQuotations) {
				serial->write('\"');
			}
			serial->write(data);
			if (dataQuotations) {
				serial->write('\"');
			}
		}
		if (semicolon) {
			serial->write(';');
		}

		//FlushData();

		serial->write(CR_ASCII_SYMBOL);
	}
	StartTimeoutTimer(timeout);
    return true;
}
