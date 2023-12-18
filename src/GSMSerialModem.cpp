#include "GSMSerialModem.h"

GSMSerialModem::GSMSerialModem(HardwareSerial *serial, int8_t resetPin):SerialCharResponseHandler(MODEM_SERIAL_BUFFER_SIZE, "\r\n", serial),cmdReleaseTimer(this)
{
    this->resetPin = resetPin;
}

GSMSerialModem::~GSMSerialModem()
{

}

void GSMSerialModem::ResetSerial(uint32_t baud, uint32_t config)
{
    HardwareSerial * serial = GetSerial();

    if (serial == NULL) return;
    serial->end();
    delay(10);
    serial->begin(baud, config);
    ResetBuffer();
    delayMicroseconds(50);
}

void GSMSerialModem::OnResponseReceived(bool isTimeOut, bool isOverFlow)
{
    cmdReleaseTimer.Stop();
    if (!isTimeOut) {
        cmdReleaseTimer.StartMicros(GSM_DATA_COLLISION_DELAY);
    }

    if (isExpectingFixedSize) {
        debugPrint->print("EXPECTED: ");
        debugPrint->print((int)bufferLength);
        debugPrint->print(" ");
        debugPrint->print((int)isTimeOut);
        debugPrint->print(" ");
        debugPrint->print((int)isOverFlow);
        debugPrint->println(" ");
        isExpectingFixedSize = false;
        OnModemResponse(NULL, buffer, bufferLength, MODEM_RESPONSE_EXPECT_DATA);
        return;
    }

    if (!isTimeOut && !isOverFlow) {

        if (pendingCMD != NULL) {
            //StartTimeoutTimer(pendingCMD->timeout);
            if (bufferLength == 0) {
                if (debugPrint != NULL) {
                    debugPrint->println("[]");
                }
                return;
            }
            if (!pendingCMD->GetIsRespStarted()) {
                if (bufferLength >= 2 + pendingCMD->cmdLen) {
                    if (strncmp(buffer, GSM_PREFIX_CMD, strlen(GSM_PREFIX_CMD)) == 0 && 
                        (pendingCMD->cmdLen == 0 || 
                            strncmp(buffer + strlen(GSM_PREFIX_CMD), pendingCMD->cmd, pendingCMD->cmdLen) == 0)) {
                    //if (buffer[bufferLength - 1] == 13) {
                        // Got cmd trace back
                        pendingCMD->SetIsRespStarted(true);
                        if (debugPrint != NULL) {
                            debugPrint->print("CMD: [");
                            if (buffer[bufferLength - 1] == 13) {
                                debugPrint->write(buffer, bufferLength - 1);
                            } else {
                                debugPrint->write(buffer, bufferLength);
                            }
                            debugPrint->println("]");
                        }
                        return;
                    }

                    // Looks like we received an event, during CMD transfer
                    /*if (debugPrint != NULL) {
                        debugPrint->print("CMD: [");
                        debugPrint->write(buffer, bufferLength);
                        debugPrint->println("]");
                    }
                    BaseModemCMD *cmd = pendingCMD;
                    pendingCMD = NULL;

                    StopTimeoutTimer();
                    StartEventBufferTimer();

                    OnModemResponse(cmd, (char *)GSM_ERROR_RESPONSE, strlen(GSM_ERROR_RESPONSE), MODEM_RESPONSE_ERROR);
                    if (cmd != NULL) {
                        delete cmd;
                    }
                    return;
                    */
                    if (debugPrint != NULL) {
                        debugPrint->print("EVENT collided: [");
                        debugPrint->write(buffer, bufferLength);
                        debugPrint->println("]");
                    }
                    StartTimeoutTimer(pendingCMD->timeout);
                    OnModemResponse(NULL, (char *)GSM_ERROR_RESPONSE, strlen(GSM_ERROR_RESPONSE), MODEM_RESPONSE_EVENT);
                    return;
                }
            }
        }
    }

    if (debugPrint != NULL) {
        if (pendingCMD == NULL) {
            debugPrint->print("Event: [");
        } else {
            debugPrint->print("Response: [");
        }
        debugPrint->write((uint8_t *)buffer, bufferLength);
        debugPrint->print("] (");
        debugPrint->print((int)bufferLength);
        debugPrint->print(") ");
        debugPrint->print(isTimeOut);
        debugPrint->print(" ");
        debugPrint->print(isOverFlow);
        debugPrint->print(" ");
        debugPrint->print(pendingCMD != NULL);
        debugPrint->print(" RAM: ");
        debugPrint->println(remainRam());
    }

    if (pendingCMD != NULL && (pendingCMD->GetIsRespStarted() || isTimeOut || isOverFlow)) {
        MODEM_RESPONSE_TYPE type = GetResponseType(pendingCMD, isTimeOut, isOverFlow);
        BaseModemCMD *cmd = pendingCMD;
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


MODEM_RESPONSE_TYPE GSMSerialModem::GetResponseType(BaseModemCMD *cmd, bool isTimeOut, bool isOverFlow)
{
    //Serial.println("GSMSerialModem::GetResponseType");
    MODEM_RESPONSE_TYPE type = MODEM_RESPONSE_DATA;
    if (isTimeOut) {
        type = MODEM_RESPONSE_TIMEOUT;
    } else if (isOverFlow) {
        type = MODEM_RESPONSE_OVERFLOW;
    } else if (strncmp(buffer, GSM_OK_RESPONSE, strlen(GSM_OK_RESPONSE)) == 0) {
        type = MODEM_RESPONSE_OK;
    } else if (strncmp(buffer, GSM_ERROR_RESPONSE, strlen(GSM_ERROR_RESPONSE)) == 0) {
        type = MODEM_RESPONSE_ERROR;
    } else if (strncmp(buffer, GSM_ABORTED_RESPONSE, strlen(GSM_ABORTED_RESPONSE)) == 0) {
        type = MODEM_RESPONSE_ABORTED;
    } else if (strncmp(buffer, GSM_CME_ERROR_RESPONSE, strlen(GSM_CME_ERROR_RESPONSE)) == 0) {
        type = MODEM_RESPONSE_ERROR;
        char *code = buffer + strlen(GSM_CME_ERROR_RESPONSE) + 1;
        cmd->cme_error = atoi(code);
    } else if (strncmp(buffer, GSM_CMS_ERROR_RESPONSE, strlen(GSM_CMS_ERROR_RESPONSE)) == 0) {
        type = MODEM_RESPONSE_ERROR;
        char *code = buffer + strlen(GSM_CMS_ERROR_RESPONSE) + 1;
        cmd->cme_error = atoi(code);
    }
    return type;
}

void GSMSerialModem::Loop()
{
    SerialCharResponseHandler::Loop();
    if (pendingCMD != NULL) {
        if (pendingCMD->GetHasExtraTrigger() && pendingCMD->GetIsRespStarted()) {
            size_t extraLen = strlen(pendingCMD->ExtraTriggerValue());
            if (bufferLength >= extraLen) {
                if (strncmp(buffer, pendingCMD->ExtraTriggerValue(), extraLen) == 0) {
                    pendingCMD->SetHasExtraTrigger(false);
                    OnModemResponse(pendingCMD, buffer, bufferLength, MODEM_RESPONSE_EXTRA_TRIGGER);
                    ResetBuffer();
                }
            }
        }
    } else if (bufferLength > 0 && !responseTimeoutTimer.IsRunning()) {
        /* For collision testing
        serial->println(".");
        if (debugPrint != NULL) {
            debugPrint->print(".[");
            debugPrint->print(bufferLength);
            debugPrint->print("/");
            debugPrint->print(serial->available());
            debugPrint->println("]");
        }
        */
        // Event buffer timer
        StartTimeoutTimer(GSM_BUFFER_FILL_TIMEOUT);
    }
}

bool GSMSerialModem::IsBusy()
{
    return pendingCMD != NULL || 
        cmdReleaseTimer.IsRunning() ||
        bufferLength != 0 || 
        responseTimeoutTimer.IsRunning() ||
        serial->available() > 0 || 
        SerialCharResponseHandler::IsBusy();
}

bool GSMSerialModem::Send(BaseModemCMD *modemCMD)
{
    if (modemCMD == NULL) return false;
    // No need to double check here
    // if (IsBusy()) return false;

    //ResetBuffer();

    if (serial == NULL) {
        return false;
    }
    this->pendingCMD = modemCMD;
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

    // NB! This is not working, some time buffer get filled partally with response!
    // serial->flush();
    // if (serial->available() > 0) {
    //    // We send CMD and being received URC data, what modem will do?!
    //    this->pendingCMD = NULL;
    //    return false;
    // }
	StartTimeoutTimer(modemCMD->timeout);
    return true;
}

void GSMSerialModem::SetDebugPrint(Print *debugPrint)
{
    this->debugPrint = debugPrint;
}
void GSMSerialModem::FlushIncoming()
{
    cmdReleaseTimer.Stop();
    StopTimeoutTimer();
    if (pendingCMD != NULL) {
        delete pendingCMD;
        pendingCMD = NULL;
    }
    FlushData();
}

void GSMSerialModem::OnTimerComplete(Timer * timer)
{
    if (timer == &cmdReleaseTimer) {
        // Nothing todo here
        return;
    }
    SerialCharResponseHandler::OnTimerComplete(timer);
}

HardwareSerial *GSMSerialModem::GetSerial()
{
    return (HardwareSerial *)serial;
}


void GSMSerialModem::SetExpectFixedLength(size_t expectFixedLength, uint32_t timeout)
{
    isExpectingFixedSize = true;
    SerialCharResponseHandler::SetExpectFixedLength(expectFixedLength, timeout);
}