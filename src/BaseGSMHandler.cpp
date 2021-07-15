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
    if (modemBootState == MODEM_BOOT_RESET) {
        return;
    }

    if (debugPrint != NULL) {
        debugPrint->println(F("StartModem"));
    }
    modemBootState = MODEM_BOOT_RESET;
    OnModemReboot();
    this->baudRate = baudRate;

    ((Uart *)serial)->end();
    delay(10);
    ((Uart *)serial)->begin(baudRate > MODEM_MAX_AUTO_BAUD_RATE ? MODEM_MAX_AUTO_BAUD_RATE : baudRate);
    Flush();

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
    Timer::Stop(modemStatusTimer);
    connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
    FlushIncoming();
    ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
}

bool BaseGSMHandler::AddCommand(BaseModemCMD *cmd)
{
    if (!IsBooted() || !commandStack.Append(cmd)) {
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
    if (!IsBooted() || !commandStack.Insert(cmd, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("ForceCommand FAIL! "));
            debugPrint->println(cmd->cmd);
        }
        delete cmd;
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommandInternal(BaseModemCMD *cmd)
{
    if (!commandStack.Insert(cmd, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("ForceCommandInternal FAIL! "));
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
    if (!IsBusy()) {
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
            if (type == MODEM_RESPONSE_TIMEOUT) {
                StartModem(true, GetBaudRate());
            } else {
                if (type >= MODEM_RESPONSE_OK) {
                    modemStatusTimer = Timer::Start(this, GSM_STATUS_CHECK_DELAY);
                }
                OnGSMResponse(cmd, response, respLen, type);
            }
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
                ForceCommandInternal(new ULongModemCMD(baudRate, GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT));
            } else {
                if (debugPrint != NULL) {
                    debugPrint->println(F("MODEM_BOOT_COMPLETED"));
                }
                modemBootState = MODEM_BOOT_COMPLETED;
                OnModemBooted();
            }
        } else if (type == MODEM_RESPONSE_TIMEOUT) {
            FlushIncoming();
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    case MODEM_BOOT_SPEED_CHAGE:
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
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        } else {
            StartModem(true, GetBaudRate());
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
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    default:
        break;
    }
}

void BaseGSMHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == connectionTimer) {
        connectionTimer = 0;
        ResetBuffer();
        modemBootState = MODEM_BOOT_ERROR;
        OnModemFailedBoot();
    }  else if (timerId == modemStatusTimer) {
        modemStatusTimer = 0;
        if (!Send(new BaseModemCMD(NULL))) {
            modemStatusTimer = Timer::Start(this, GSM_STATUS_CHECK_DELAY);
        }
    } else {
        GSMSerialModem::OnTimerComplete(timerId, data);
    }
}

void BaseGSMHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == connectionTimer) {
        connectionTimer = 0;
    } else if (timerId == modemStatusTimer) {
        modemStatusTimer = 0;
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

bool BaseGSMHandler::IsBooted()
{
    return modemBootState == MODEM_BOOT_COMPLETED;
}

unsigned long BaseGSMHandler::GetBaudRate()
{
    return baudRate;
}

bool BaseGSMHandler::Send(BaseModemCMD *modemCMD)
{
    bool status = GSMSerialModem::Send(modemCMD);
    if (status) {
        if (modemStatusTimer != 0) {
            Timer::Stop(modemStatusTimer);
        }
    }
    return status;
}