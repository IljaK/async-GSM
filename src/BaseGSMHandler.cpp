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
    Flush();
    this->baudRate = baudRate;
    modemBootState = MODEM_BOOT_RESET;

    ((Uart *)serial)->end();
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
    Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
}

bool BaseGSMHandler::AddCommand(const char *command, unsigned long timeout)
{
    if (!commandStack.Append((char *)command, timeout)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("AddCommand FAIL! "));
            debugPrint->println(command);
        }
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommand(const char *command, unsigned long timeout)
{
    if (!commandStack.Insert((char *)command, timeout, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(F("ForceCommand FAIL! "));
            debugPrint->println(command);
        }
        return false;
    }
    /*
    debugPrint->print("Stack: ");
    for (size_t i = 0; i < commandStack.Size(); i++) {
        ModemCommand *cmd = commandStack.Peek(i);
        if (cmd != NULL) {
            debugPrint->print(cmd->command);
            debugPrint->print(" ");
        }
    }
    debugPrint->println("");
    */
    return true;
}

void BaseGSMHandler::Loop()
{
    GSMSerialModem::Loop();
    if (!IsBusy() && modemBootState == MODEM_BOOT_COMPLETED && pendingCommand == NULL) {
        if (commandStack.Size() > 0) {
            pendingCommand = commandStack.UnshiftFirst();
            if (debugPrint != NULL) {
                debugPrint->print("> ");
                debugPrint->println(pendingCommand->command);
            }
            Send(pendingCommand->command, pendingCommand->timeout);
        }
    }
}

void BaseGSMHandler::OnModemResponse(char *data, MODEM_RESPONSE_TYPE type)
{
    switch (type)
    {
    case MODEM_RESPONSE_EVENT:
        if (modemBootState == MODEM_BOOT_COMPLETED) {
            OnGSMEvent(data);
            break;
        }
    default:
        OnGSMResponseInternal(data, type);
        break;
    }
}

void BaseGSMHandler::OnGSMResponseInternal(char * response, MODEM_RESPONSE_TYPE type)
{
    switch (modemBootState)
    {
    case MODEM_BOOT_COMPLETED:
        if (pendingCommand != NULL) {
            OnGSMResponse(pendingCommand->command, response, type);
            if (type >= MODEM_RESPONSE_OK) {
                if (pendingCommand != NULL) {
                    commandStack.FreeItem(pendingCommand);
                    pendingCommand = NULL;
                }
            }
        }
        break;
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
                char baud[32];
                snprintf(baud, 32, "%ld", baudRate);
                Sendf(GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT, false, true, baud, false, false);
            } else {

                if (debugPrint != NULL) {
                    debugPrint->println(F("MODEM_BOOT_COMPLETED"));
                }
                modemBootState = MODEM_BOOT_COMPLETED;
                OnModemBooted();
            }
        } else {
            // TODO: Resend AT?
            FlushIncoming();
            Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
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
            ResetBuffer();
            ((Uart *)serial)->begin(baudRate);
            Timer::Stop(connectionTimer);
            connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
            Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
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
        } else {
            FlushIncoming();
            Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
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
    if (pendingCommand != NULL) {
        commandStack.FreeItem(pendingCommand);
        pendingCommand = NULL;
    }
    commandStack.Clear();
    ResetBuffer();
}

Stream *BaseGSMHandler::GetModemStream()
{
    return serial;
}