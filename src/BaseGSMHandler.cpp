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
    Flush();
    this->baudRate = baudRate;
    modemBootState = MODEM_BOOT_RESET;

    SerialGSM.end();
    SerialGSM.begin(baudRate > MODEM_MAX_AUTO_BAUD_RATE ? MODEM_MAX_AUTO_BAUD_RATE : baudRate);

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

        // TODO: Software reset:
        // send("AT+CFUN=16");
    }
#endif

    modemBootState = MODEM_BOOT_CONNECTING;
    Timer::Stop(connectionTimer);
    connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
    Serial.println("MODEM_BOOT_RESET + MODEM_BOOT_CONNECTING");
    Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
    
}

bool BaseGSMHandler::AddCommand(const char *command, unsigned long timeout)
{
    Serial.print("AddCommand: ");
    Serial.println(command);

    if (!commandStack.Append((char *)command, timeout)) {
        Serial.println("AddCommand => FAIL");
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommand(const char *command, unsigned long timeout)
{
    Serial.print("ForceCommand: ");
    Serial.println(command);

    if (!commandStack.Insert((char *)command, timeout, 0)) {
        Serial.println("ForceCommand => FAIL");
        return false;
    }

    Serial.print("Stack: ");
    for (size_t i = 0; i < commandStack.Size(); i++) {
        ModemCommand *cmd = commandStack.Peek(i);
        if (cmd != NULL) {
            Serial.print(cmd->command);
            Serial.print(" ");
        }
    }
    Serial.println("");
    return true;
}

void BaseGSMHandler::Loop()
{
    GSMSerialModem::Loop();
    if (!IsBusy() && modemBootState == MODEM_BOOT_COMPLETED && pendingCommand == NULL) {
        if (commandStack.Size() > 0) {
            pendingCommand = commandStack.UnshiftFirst();
            Serial.print("Send command: ");
            Serial.println(pendingCommand->command);
            Send(pendingCommand->command, pendingCommand->timeout);
        }
    }
}

void BaseGSMHandler::OnModemResponse(char *data, MODEM_RESPONSE_TYPE type)
{
    switch (type)
    {
    case MODEM_RESPONSE_EVENT:
        OnGSMEvent(data);
        break;
    case MODEM_RESPONSE_DATA:
    case MODEM_RESPONSE_OK:
    case MODEM_RESPONSE_ERROR:
    case MODEM_RESPONSE_ABORTED:
    case MODEM_RESPONSE_TIMEOUT:
    case MODEM_RESPONSE_OVERFLOW:
        OnGSMResponseInternal(data, type);
        break;
    
    default:
        break;
    }
}

void BaseGSMHandler::OnGSMResponseInternal(char * response, MODEM_RESPONSE_TYPE type)
{
    Serial.print("OnGSMResponseInternal: ");
    
    if (pendingCommand == NULL ) {
        Serial.print("[");
    } else {
        Serial.print(pendingCommand->command);
        Serial.print(" [");
    }
    Serial.print(buffer);
    Serial.print("] ");
    Serial.println((int)type);

    switch (modemBootState)
    {
    case MODEM_BOOT_COMPLETED:
        OnGSMResponse(pendingCommand->command, response, type);
        if (type >= MODEM_RESPONSE_OK) {
            if (pendingCommand != NULL) {
                commandStack.FreeItem(pendingCommand);
                pendingCommand = NULL;
            }
        }
        break;
    case MODEM_BOOT_CONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            Serial.println("MODEM_BOOT_CONNECTING OK");
            //MODEM.flush();
            FlushData();
            Timer::Stop(connectionTimer);

            if (baudRate > MODEM_MAX_AUTO_BAUD_RATE) {
                Serial.println("MODEM_BOOT_SPEED_CHAGE");
                modemBootState = MODEM_BOOT_SPEED_CHAGE;
                char baud[32];
                snprintf(baud, 32, "%ld", baudRate);
                Sendf(GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT, false, true, baud, false, false);
                //MODEM.setResponseDataStorage(&response);
                //MODEM.sendf("AT+IPR=%ld", baudRate);
            } else {
                Serial.println("MODEM_BOOT_COMPLETED");
                modemBootState = MODEM_BOOT_COMPLETED;
                OnModemBooted();
            }
        } else {
            // TODO: Resend AT?
            Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
        }
        break;
    case MODEM_BOOT_SPEED_CHAGE:
        // reboot?
        if (type == MODEM_RESPONSE_OK) {
            Serial.println("MODEM_BOOT_SPEED_CHAGE OK");
            modemBootState = MODEM_BOOT_RECONNECTING;
            SerialGSM.end();
            SerialGSM.begin(baudRate);
            // Do not use FlushData here! will trigger serial->flush and freeze!
            ResetBuffer();
            Timer::Stop(connectionTimer);
            connectionTimer = Timer::Start(this, GSM_MODEM_CONNECTION_TIME);
            Send((char *)GSM_PREFIX_CMD, MODEM_BOOT_COMMAND_TIMEOUT);
            // TODO: delay?
        } else {
            // TODO: ?
        }

        break;
    case MODEM_BOOT_RECONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            Serial.println("MODEM_BOOT_RECONNECTING OK");
            //MODEM.flush();
            FlushData();
            Timer::Stop(connectionTimer);
            modemBootState = MODEM_BOOT_COMPLETED;
            OnModemBooted();
            // connected?
        } else {
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
        // TODO: 
        FlushData();
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