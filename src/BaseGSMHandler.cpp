#include "BaseGSMHandler.h"

BaseGSMHandler::BaseGSMHandler():GSMSerialModem(), commandStack(OUT_MESSAGE_STACK_SIZE)
{
}

BaseGSMHandler::~BaseGSMHandler()
{
    Flush();
}

void BaseGSMHandler::StartModem(bool restart, unsigned long baudRate)
{
    if (debugPrint != NULL) {
        debugPrint->println(F("StartModem"));
    }
    OnModemReboot();
    this->baudRate = baudRate;
    modemBootState = MODEM_BOOT_RESET;

    ((Uart *)serial)->end();
    delay(10);
    Flush();
    ((Uart *)serial)->begin(baudRate > MODEM_MAX_AUTO_BAUD_RATE ? MODEM_MAX_AUTO_BAUD_RATE : baudRate);

#ifdef GSM_RESETN
    pinMode(GSM_RESETN, OUTPUT);
    digitalWrite(GSM_RESETN, LOW);
#endif

#ifdef GSM_RTS
    pinMode(GSM_RTS, OUTPUT);
    digitalWrite(GSM_RTS, LOW);
#endif

#ifdef GSM_CTS
    pinMode(GSM_CTS, INPUT);
#endif

#ifdef GSM_RESETN
    if (restart) {
        // Hardware reset
        pinMode(GSM_RESETN, OUTPUT);
        digitalWrite(GSM_RESETN, HIGH);
        delayMicroseconds(150);
        digitalWrite(GSM_RESETN, LOW);
        delay(3);
        digitalWrite(GSM_RESETN, HIGH);
        delay(600);
        digitalWrite(GSM_RESETN, LOW);

        // TODO: Software reset?
        // send("AT+CFUN=16");
    }
#endif

    modemBootState = MODEM_BOOT_CONNECTING;
    Timer::Stop(connectionTimer);
    connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
    FlushIncoming();
    Send(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
}

bool BaseGSMHandler::AddCommand(BaseModemCMD *cmd)
{
    if (!commandStack.Append(cmd)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("AddCommand FAIL! "));
            debugPrint->println(cmd->cmd);
        }
        delete cmd;
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommand(BaseModemCMD *cmd)
{
    if (!commandStack.Insert(cmd, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("ForceCommand FAIL! "));
            debugPrint->println(cmd->cmd);
        }
        delete cmd;
        return false;
    }
    return true;
}

void BaseGSMHandler::Loop()
{
    GSMSerialModem::Loop();
    if (!IsBusy() && modemBootState == MODEM_BOOT_COMPLETED) {
        if (commandStack.Size() > 0) {
            Send(commandStack.UnshiftFirst());
        }
    }
}

void BaseGSMHandler::OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type)
{
    switch (type)
    {
    case MODEM_RESPONSE_EVENT:
        if (modemBootState == MODEM_BOOT_COMPLETED) {
            OnGSMEvent(data, dataLen);
            break;
        }
    default:
        OnGSMResponseInternal(cmd, data, dataLen, type);
        break;
    }
}

void BaseGSMHandler::OnGSMResponseInternal(BaseModemCMD *cmd, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    switch (modemBootState)
    {
    case MODEM_BOOT_COMPLETED:
        if (cmd != NULL) {
            OnGSMResponse(cmd, response, respLen, type);
        }
        return;
    case MODEM_BOOT_CONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(F("MODEM_BOOT_CONNECTING OK"));
            }
            FlushData();
            Timer::Stop(connectionTimer);
            if (baudRate > MODEM_MAX_AUTO_BAUD_RATE) {

                if (debugPrint != NULL) {
                    debugPrint->println(F("MODEM_BOOT_SPEED_CHAGE"));
                }
                modemBootState = MODEM_BOOT_SPEED_CHAGE;
                Send(new ULongModemCMD(baudRate, GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT));
            } else {
                if (debugPrint != NULL) {
                    debugPrint->println(F("MODEM_BOOT_COMPLETED"));
                }
                modemBootState = MODEM_BOOT_COMPLETED;
                OnModemBooted();
            }
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            FlushIncoming();
            Send(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    case MODEM_BOOT_SPEED_CHAGE:
        // reboot?
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(F("MODEM_BOOT_SPEED_CHAGE OK"));
            }
            modemBootState = MODEM_BOOT_RECONNECTING;
            ((Uart *)serial)->end();
            delay(10);
            ResetBuffer();
            ((Uart *)serial)->begin(baudRate);
            Timer::Stop(connectionTimer);
            connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
            Send(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        } else {
            // TODO: ?
        }
        break;
    case MODEM_BOOT_RECONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(F("MODEM_BOOT_RECONNECTING OK"));
            }
            ResetBuffer();
            Timer::Stop(connectionTimer);
            modemBootState = MODEM_BOOT_COMPLETED;
            OnModemBooted();
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            FlushIncoming();
            Send(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    default:
        break;
    }
}

void BaseGSMHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == connectionTimer) {
        ResetBuffer();
        modemBootState = MODEM_BOOT_ERROR;
        OnModemFailedBoot();
    } else {
        GSMSerialModem::OnTimerComplete(timerId, data);
    }
}

void BaseGSMHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == connectionTimer) {
        connectionTimer = 0;
    } else {
        GSMSerialModem::OnTimerStop(timerId, data);
    }
}

void BaseGSMHandler::Flush()
{
    if (eventBufferTimeout != 0) {
        Timer::Stop(eventBufferTimeout);
    }
    if (pendingCMD != NULL) {
        delete pendingCMD;
        pendingCMD = NULL;
    }
    commandStack.Clear();
    ResetBuffer();
}

Stream *BaseGSMHandler::GetModemStream()
{
    return serial;
}