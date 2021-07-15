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
        if (pendingCMD != NULL) {
            if (bufferLength == 0) {
                if (debugPrint != NULL) {
                    debugPrint->println("[]");
                }
                StartTimeoutTimer(pendingCMD->timeout);
                return;
            }
            size_t sz = strlen(GSM_PREFIX_CMD);
            if (bufferLength >= sz && strncmp(buffer, GSM_PREFIX_CMD, sz) == 0) {
                if (pendingCMD->cmd == NULL) {
                    return;
                }
                if (bufferLength >= sz + pendingCMD->cmdLen && strncmp(buffer+sz, pendingCMD->cmd, pendingCMD->cmdLen) == 0) {
                    if (debugPrint != NULL) {
                        debugPrint->print("CMD RESP: [");
                        if (buffer[bufferLength - 1] == 13) {
                            debugPrint->write(buffer, bufferLength - 1);
                        } else {
                            debugPrint->write(buffer, bufferLength);
                        }
                        debugPrint->println("]");
                    }
                    return;
                }
            }
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
        debugPrint->print(pendingCMD != NULL);
        debugPrint->print(" RAM: ");
        debugPrint->println(remainRam());
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
            if (IsTimeOut) {
                Timer::Stop(cmdReleaseTimer);
                cmdReleaseTimer = Timer::Start(this, GSM_CMD_URC_COLLISION_DELAY);
            }
        } else {
            StartTimeoutTimer(pendingCMD->timeout);
        }
        OnModemResponse(cmd, buffer, bufferLength, type);
        if (type >= MODEM_RESPONSE_OK) {
            delete cmd;
        }
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
        if (pendingCMD->ExtraTrigger()) {
            size_t extraLen = strlen(pendingCMD->ExtraTriggerValue());
            if (bufferLength >= extraLen) {
                if (strncmp(buffer, pendingCMD->ExtraTriggerValue(), extraLen) == 0) {
                    OnResponseReceived(false, false);
                    ResetBuffer();
                }
            }
        }
    } else if (prevBuffAmount != bufferLength) {
        Timer::Stop(eventBufferTimeout);
        if (bufferLength > 0) {
            eventBufferTimeout = Timer::Start(this, GSM_BUFFER_FILL_TIMEOUT);
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
        cmdReleaseTimer != 0 ||
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
		if (debugPrint != NULL) debugPrint->println();
		serial->println();
	}
	StartTimeoutTimer(modemCMD->timeout);
    return true;
}

void GSMSerialModem::SetDebugPrint(Print *debugPrint)
{
    this->debugPrint = debugPrint;
}
void GSMSerialModem::FlushIncoming()
{
    StopTimeoutTimer();
    if (pendingCMD != NULL) {
        delete pendingCMD;
        pendingCMD = NULL;
    }
    while (serial->available() > 0) {
        serial->read();
    }
    ResetBuffer();
}

void GSMSerialModem::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == eventBufferTimeout) {
        eventBufferTimeout = 0;
        OnModemResponse(NULL, buffer, bufferLength, MODEM_RESPONSE_EVENT);
        ResetBuffer();
    } else if (timerId == cmdReleaseTimer) { 
        cmdReleaseTimer = 0;
    } else {
        SerialCharResponseHandler::OnTimerComplete(timerId, data);
    }

}

void GSMSerialModem::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == eventBufferTimeout) {
        eventBufferTimeout = 0;
    } else if (timerId == cmdReleaseTimer) { 
        cmdReleaseTimer = 0;
    } else {
        SerialCharResponseHandler::OnTimerStop(timerId, data);
    }
}
