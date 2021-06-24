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
        if (bufferLength >= sz && strncmp(buffer, GSM_PREFIX_CMD, sz) == 0) {
            return;
        }
    }

    if (debugPrint != NULL) {
        debugPrint->print("Response: [");
        debugPrint->print(buffer);
        debugPrint->print("] IsTimeOut: ");
        debugPrint->print(IsTimeOut);
        debugPrint->print(" isOverFlow: ");
        debugPrint->print(isOverFlow);
        debugPrint->print(" isWaitingConfirm: ");
        debugPrint->println(pendingCMD != NULL);
    }


    if (pendingCMD != NULL) {
        MODEM_RESPONSE_TYPE type = MODEM_RESPONSE_DATA;
        BaseModemCMD *cmd = pendingCMD;
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
            pendingCMD = NULL;
        } else {
            StartTimeoutTimer(pendingCMD->timeout);
        }
        OnModemResponse(cmd, buffer, bufferLength, type);
    } else {
        OnModemResponse(NULL, buffer, bufferLength, MODEM_RESPONSE_EVENT);
    }
}

void GSMSerialModem::Loop()
{
    size_t prevBuffAmount = bufferLength;
    SerialCharResponseHandler::Loop();
    if (pendingCMD != NULL) {
        if (eventBufferTimeout != 0) {
            Timer::Stop(eventBufferTimeout);
        }
    } else if (prevBuffAmount != bufferLength) {
        Timer::Stop(eventBufferTimeout);
        if (bufferLength > 0) {
            if (debugPrint != NULL) {
                debugPrint->println("Start GSM_EVENT_BUFFER_TIMEOUT");
            }
            eventBufferTimeout = Timer::Start(this, GSM_EVENT_BUFFER_TIMEOUT);
        }
    } else if (eventBufferTimeout != 0) {
        if (bufferLength == 0) {
            Timer::Stop(eventBufferTimeout);
        }
    }
}

bool GSMSerialModem::IsBusy()
{
    return pendingCMD != NULL || 
        bufferLength != 0 || 
        serial->available() != 0 || 
        SerialCharResponseHandler::IsBusy();
}

bool GSMSerialModem::Send(BaseModemCMD *modemCMD)
{
    if (IsBusy()) return false;
    if (modemCMD == NULL) return false;

    ResetBuffer();
    this->pendingCMD = modemCMD;
	if (serial) {
        if (debugPrint != NULL) debugPrint->print(GSM_PREFIX_CMD);
		serial->write(GSM_PREFIX_CMD);
		
		if (modemCMD->cmd != NULL) {
            if (debugPrint != NULL) debugPrint->print(modemCMD->cmd);
			serial->write(modemCMD->cmd);
		}
		if (modemCMD->IsSet()) {
            if (debugPrint != NULL) debugPrint->print('=');
			serial->write('=');
		}
		if (modemCMD->IsCheck()) {
            if (debugPrint != NULL) debugPrint->print('?');
			serial->write('?');
		}
        if (modemCMD->InQuatations()) {
            if (debugPrint != NULL) debugPrint->print('\"');
            serial->write('\"');
        }
        if (debugPrint != NULL) modemCMD->WriteStream(debugPrint);
        modemCMD->WriteStream(serial);

        if (modemCMD->InQuatations()) {
            if (debugPrint != NULL) debugPrint->print('\"');
            serial->write('\"');
        }
		if (modemCMD->EndSemicolon()) {
            if (debugPrint != NULL) debugPrint->print(';');
			serial->write(';');
		}
        if (debugPrint != NULL) {
            debugPrint->println();
        }
		serial->println();
	}
	StartTimeoutTimer(modemCMD->timeout);
    return true;
}

/*
bool GSMSerialModem::Send(char *data, unsigned long timeout)
{
    if (IsBusy()) return false;
    if (data == NULL || data[0] == 0) return false;
    ResetBuffer();
    this->timeout = timeout;
    isWaitingConfirm = true;
	StartTimeoutTimer(timeout);
    serial->println(data);
    return true;
}

bool GSMSerialModem::Sendf(const char * cmd, unsigned long timeout, bool isCheck, bool isSet, char *data, bool dataQuotations, bool semicolon)
{
    if (IsBusy()) return false;
    if (data == NULL || data[0] == 0) return false;

    ResetBuffer();
    this->timeout = timeout;
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
		serial->write(CR_ASCII_SYMBOL);
	}
	StartTimeoutTimer(timeout);
    return true;
}
*/

void GSMSerialModem::SetDebugPrint(Print *debugPrint)
{
    this->debugPrint = debugPrint;
}
void GSMSerialModem::FlushIncoming()
{
    if (pendingCMD != NULL) {
        delete pendingCMD;
        pendingCMD = NULL;
    }
    while (serial->available()>0)
    {
        serial->read();
    }
    ResetBuffer();
}

void GSMSerialModem::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == eventBufferTimeout) {
        eventBufferTimeout = 0;
        if (debugPrint != NULL) {
            debugPrint->println("OnTimerComplete GSM_EVENT_BUFFER_TIMEOUT");
        }
        OnModemResponse(NULL, buffer, bufferLength, MODEM_RESPONSE_EVENT);
        ResetBuffer();
    } else {
        SerialCharResponseHandler::OnTimerComplete(timerId, data);
    }
}

void GSMSerialModem::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == eventBufferTimeout) {
        eventBufferTimeout = 0;
    } else {
        SerialCharResponseHandler::OnTimerStop(timerId, data);
    }
}
