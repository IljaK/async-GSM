#include "GSMSerialModem.h"

GSMSerialModem::GSMSerialModem():SerialCharResponseHandler(MODEM_SERIAL_BUFFER_SIZE, "\r\n", &SerialGSM)
{

}

GSMSerialModem::~GSMSerialModem()
{

}

void GSMSerialModem::OnResponseReceived(bool IsTimeOut, bool isOverFlow)
{
    Timer::Stop(cmdReleaseTimer);
    if (!IsTimeOut) {
        cmdReleaseTimer = Timer::Start(this, GSM_DATA_COLLISION_DELAY);
    }

    if (!IsTimeOut && !isOverFlow) {
        if (pendingCMD != NULL) {
            if (bufferLength == 0) {
                if (debugPrint != NULL) {
                    debugPrint->println("[]");
                }
                StartTimeoutTimer(pendingCMD->timeout);
                return;
            }
            if (!pendingCMD->GetIsRespStarted()) {
                size_t sz = strlen(GSM_PREFIX_CMD);
                // CMD echo enabled: (ATE1)
                if (bufferLength > sz && strncmp(buffer, GSM_PREFIX_CMD, sz) == 0) {
                    if (buffer[bufferLength - 1] == 13) {
                        pendingCMD->SetIsRespStarted(true);
                    }
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
                // TODO: In case of echo cmd is disabled: (ATE0)
            }
        }
    }

    if (debugPrint != NULL) {
        if (pendingCMD == NULL) {
            debugPrint->print("Event: [");
        } else {
            debugPrint->print("Response: [");
        }
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

    if (pendingCMD != NULL && (pendingCMD->GetIsRespStarted() || IsTimeOut || isOverFlow)) {
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
    #ifdef GSM_DEBUG_ERROR_CMD
        else if (strncmp(buffer, GSM_CME_ERROR_RESPONSE, strlen(GSM_CME_ERROR_RESPONSE)) == 0) {
            type = MODEM_RESPONSE_ERROR;
        }
    #endif
        if (type >= MODEM_RESPONSE_OK) {
            pendingCMD = NULL;
            StopTimeoutTimer();
        } else {
            StartTimeoutTimer(pendingCMD->timeout);
        }
        OnModemResponse(cmd, buffer, bufferLength, type);
        if (pendingCMD == NULL && cmd != NULL) {
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
        if (pendingCMD->GetHasExtraTrigger()) {
            size_t extraLen = strlen(pendingCMD->ExtraTriggerValue());
            if (bufferLength >= extraLen) {
                if (strncmp(buffer, pendingCMD->ExtraTriggerValue(), extraLen) == 0) {
                    OnResponseReceived(false, false);
                    ResetBuffer();
                }
            }
        }
    } else if (prevBuffAmount != bufferLength) {
        if (bufferLength > 0 && eventBufferTimeout == 0) {
            eventBufferTimeout = Timer::Start(this, GSM_BUFFER_FILL_TIMEOUT);
        }
    }
}

bool GSMSerialModem::IsBusy()
{
    return pendingCMD != NULL || 
        cmdReleaseTimer != 0 ||
        bufferLength != 0 || 
        eventBufferTimeout != 0 ||
        serial->available() > 0 || 
        SerialCharResponseHandler::IsBusy();
}

bool GSMSerialModem::Send(BaseModemCMD *modemCMD)
{
    if (modemCMD == NULL) return false;
    // No need to double check here
    // if (IsBusy()) return false;

    ResetBuffer();
    this->pendingCMD = modemCMD;
	if (serial) {
        if (debugPrint != NULL) debugPrint->print(GSM_PREFIX_CMD);
		serial->write(GSM_PREFIX_CMD);
		
		if (modemCMD->cmd != NULL) {
            if (debugPrint != NULL) debugPrint->print(modemCMD->cmd);
			serial->write(modemCMD->cmd);
		}
		if (modemCMD->GetIsModifier()) {
            if (debugPrint != NULL) debugPrint->print('=');
			serial->write('=');
		}
		if (modemCMD->GetIsCheck()) {
            if (debugPrint != NULL) debugPrint->print('?');
			serial->write('?');
		}
        if (modemCMD->GetInQuatations()) {
            if (debugPrint != NULL) debugPrint->print('\"');
            serial->write('\"');
        }
        if (debugPrint != NULL) modemCMD->WriteStream(debugPrint);
        modemCMD->WriteStream(serial);

        if (modemCMD->GetInQuatations()) {
            if (debugPrint != NULL) debugPrint->print('\"');
            serial->write('\"');
        }
		if (modemCMD->GetEndSemicolon()) {
            if (debugPrint != NULL) debugPrint->print(';');
			serial->write(';');
		}
		if (debugPrint != NULL) debugPrint->println();
		serial->println();
	}
    // TODO: URC collision detection!
    // Waits for the transmission of outgoing serial data to complete.
    serial->flush();
    if (serial->available() > 0) {
        // We send CMD and being received URC data, what modem will do?!
        this->pendingCMD = NULL;
        return false;
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
    if (cmdReleaseTimer != 0) {
        Timer::Stop(cmdReleaseTimer);
        cmdReleaseTimer = 0;
    }
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
