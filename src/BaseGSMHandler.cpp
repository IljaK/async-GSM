#include "BaseGSMHandler.h"

BaseGSMHandler::BaseGSMHandler(HardwareSerial *serial, int8_t resetPin)
    :GSMSerialModem(serial, resetPin), commandStack(OUT_MESSAGE_STACK_SIZE)
{
}

BaseGSMHandler::~BaseGSMHandler()
{
    Flush();
}

void BaseGSMHandler::StartModem(bool restart, uint32_t baudRate)
{
    if (modemBootState == MODEM_BOOT_RESET) {
        return;
    }

    if (debugPrint != NULL) {
        debugPrint->println(PSTR("StartModem"));
    }
    modemBootState = MODEM_BOOT_RESET;
    OnModemReboot();
    this->baudRate = baudRate;

    Reboot(baudRate, SERIAL_8N1, true);
    Flush();

    modemBootState = MODEM_BOOT_CONNECTING;
    Timer::Stop(modemRebootTimer);
    modemRebootTimer = Timer::Start(this, GSM_MODEM_INIT_TIMEOUT);
    FlushIncoming();
    ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
}

bool BaseGSMHandler::AddCommand(BaseModemCMD *cmd)
{
    if (cmd == NULL) return false;

    if (!IsBooted() || !commandStack.Append(cmd)) {
        if (debugPrint != NULL) {
            debugPrint->print(PSTR("AddCommand FAIL! "));
        }
        delete cmd;
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommand(BaseModemCMD *cmd)
{
    if (cmd == NULL) return false;

    if (!IsBooted() || !commandStack.Insert(cmd, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(PSTR("ForceCommand FAIL! "));
        }
        delete cmd;
        return false;
    }
    return true;
}

bool BaseGSMHandler::ForceCommandInternal(BaseModemCMD *cmd)
{
    if (cmd == NULL) return false;
    
    if (!commandStack.Insert(cmd, 0)) {
        if (debugPrint != NULL) {
            debugPrint->print(PSTR("ForceCommandInternal FAIL! "));
        }
        delete cmd;
        return false;
    }
    return true;
}

void BaseGSMHandler::Loop()
{
    GSMSerialModem::Loop();
    if (!IsBusy() && commandStack.Size() > 0) {
        if (Send(commandStack.Peek())) {
            commandStack.UnshiftFirst();
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
                OnGSMResponse(cmd, response, respLen, type);
            }
        }
        return;
    case MODEM_BOOT_CONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(PSTR("MODEM_BOOT_CONNECTING OK"));
            }
            FlushData();
            Timer::Stop(modemRebootTimer);
            if (baudRate > MODEM_MAX_AUTO_BAUD_RATE) {
                if (debugPrint != NULL) {
                    debugPrint->println(PSTR("MODEM_BOOT_SPEED_CHAGE"));
                }
                modemBootState = MODEM_BOOT_SPEED_CHAGE;
                ForceCommandInternal(new ULongModemCMD(baudRate, GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT));
            } else {
                if (debugPrint != NULL) {
                    debugPrint->println(PSTR("MODEM_BOOT_COMPLETED"));
                }
                modemBootState = MODEM_BOOT_DEBUG_SET;
                ForceCommandInternal(new ByteModemCMD(1, GSM_MODEM_CME_ERR_CMD));
                //modemBootState = MODEM_BOOT_COMPLETED;
                //OnModemBooted();
            }
        } else if (type > MODEM_RESPONSE_OK) {
            FlushIncoming();
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    case MODEM_BOOT_SPEED_CHAGE:
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(PSTR("MODEM_BOOT_SPEED_CHAGE OK"));
            }
            modemBootState = MODEM_BOOT_RECONNECTING;
            ResetSerial(baudRate, SERIAL_8N1);
            Timer::Stop(modemRebootTimer);
            modemRebootTimer = Timer::Start(this, GSM_MODEM_INIT_TIMEOUT);
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        } else {
            StartModem(true, GetBaudRate());
        }
        break;
    case MODEM_BOOT_RECONNECTING:
        if (type == MODEM_RESPONSE_OK) {
            if (debugPrint != NULL) {
                debugPrint->println(PSTR("MODEM_BOOT_RECONNECTING OK"));
            }
            modemBootState = MODEM_BOOT_DEBUG_SET;
            ForceCommandInternal(new ByteModemCMD(1, GSM_MODEM_CME_ERR_CMD));
        } else if (type > MODEM_RESPONSE_OK) {
            FlushIncoming();
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        }
        break;
    case MODEM_BOOT_DEBUG_SET:
        if (type == MODEM_RESPONSE_OK) {
            ResetBuffer();
            Timer::Stop(modemRebootTimer);
            modemBootState = MODEM_BOOT_COMPLETED;
            OnModemBooted();
        } else {
            StartModem(true, GetBaudRate());
        }
        break;
    default:
        break;
    }
}

void BaseGSMHandler::OnTimerComplete(TimerID timerId, uint8_t data)
{
    if (timerId == modemRebootTimer) {
        modemRebootTimer = 0;
        ResetBuffer();
        modemBootState = MODEM_BOOT_ERROR;
        OnModemFailedBoot();
    } else {
        GSMSerialModem::OnTimerComplete(timerId, data);
    }
}

void BaseGSMHandler::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == modemRebootTimer) {
        modemRebootTimer = 0;
    } else {
        GSMSerialModem::OnTimerStop(timerId, data);
    }
}

void BaseGSMHandler::Flush()
{
    if (eventBufferTimeout != 0) {
        Timer::Stop(eventBufferTimeout);
        eventBufferTimeout = 0;
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

uint32_t BaseGSMHandler::GetBaudRate()
{
    return baudRate;
}

size_t BaseGSMHandler::GetCommandStackCount()
{
    return commandStack.Size();
}