#include "GSMModemManager.h"

GSMModemManager::GSMModemManager(HardwareSerial *serial, int8_t resetPin)
    :GSMSerialModem(serial, resetPin), commandStack(OUT_MESSAGE_STACK_SIZE)
{
}

GSMModemManager::~GSMModemManager()
{
    Flush();
}


void GSMModemManager::SetConfig(uint32_t initBaudRate, uint32_t targetBaudRate, uint32_t serialConfig)
{
    this->initBaudRate = initBaudRate;
    this->targetBaudRate = targetBaudRate;
    this->serialConfig = serialConfig;
}

void GSMModemManager::StartModem()
{
    if (modemBootState == MODEM_BOOT_RESET) {
        return;
    }
    if (initBaudRate == 0) {
        return;
    }

    if (debugPrint != NULL) {
        debugPrint->println(PSTR("StartModem"));
    }
    modemBootState = MODEM_BOOT_RESET;

    OnModemStartReboot();
    ReBootModem();
}
void GSMModemManager::ReBootModem(bool hardReset)
{
    if (debugPrint != NULL) {
        debugPrint->println(PSTR("GSMModemManager::ReBootModem"));
    }
    // Override to perform modedm hard reset
    BootModem();
}

void GSMModemManager::BootModem()
{
    modemBootState = MODEM_BOOT_CONNECTING;

    ResetSerial(initBaudRate, serialConfig);
    Flush();
    FlushIncoming();

    Timer::Stop(modemRebootTimer);
    modemRebootTimer = Timer::Start(this, GSM_MODEM_INIT_TIMEOUT);
    ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
}

bool GSMModemManager::AddCommand(BaseModemCMD *cmd)
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

bool GSMModemManager::ForceCommand(BaseModemCMD *cmd)
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

bool GSMModemManager::ForceCommandInternal(BaseModemCMD *cmd)
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

void GSMModemManager::Loop()
{
    GSMSerialModem::Loop();
    if (!IsBusy() && commandStack.Size() > 0) {
        if (Send(commandStack.Peek())) {
            commandStack.UnshiftFirst();
        }
    }
}

void GSMModemManager::OnModemResponse(BaseModemCMD *cmd, char *data, size_t dataLen, MODEM_RESPONSE_TYPE type)
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

void GSMModemManager::OnGSMResponseInternal(BaseModemCMD *cmd, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    switch (modemBootState)
    {
    case MODEM_BOOT_COMPLETED:
        if (cmd != NULL) {
            if (type == MODEM_RESPONSE_TIMEOUT) {
                if (debugPrint != NULL) {
                    debugPrint->println(PSTR("MODEM_RESPONSE_TIMEOUT"));
                    StartModem();
                }
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
            if (initBaudRate != targetBaudRate) {
                if (debugPrint != NULL) {
                    debugPrint->println(PSTR("MODEM_BOOT_SPEED_CHAGE"));
                }
                modemBootState = MODEM_BOOT_SPEED_CHAGE;
                ForceCommandInternal(new ULongModemCMD(targetBaudRate, GSM_MODEM_SPEED_CMD, MODEM_COMMAND_TIMEOUT));
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
            ResetSerial(((ULongModemCMD*)cmd)->valueData, SERIAL_8N1);
            Timer::Stop(modemRebootTimer);
            modemRebootTimer = Timer::Start(this, GSM_MODEM_INIT_TIMEOUT);
            ForceCommandInternal(new BaseModemCMD(NULL, MODEM_BOOT_COMMAND_TIMEOUT));
        } else {
            if (debugPrint != NULL) {
                debugPrint->println(PSTR("MODEM_BOOT_SPEED_CHAGE FAIL"));
            }
            StartModem();
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
            StartModem();
        }
        break;
    default:
        break;
    }
}

void GSMModemManager::OnTimerComplete(TimerID timerId, uint8_t data)
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

void GSMModemManager::OnTimerStop(TimerID timerId, uint8_t data)
{
    if (timerId == modemRebootTimer) {
        modemRebootTimer = 0;
    } else {
        GSMSerialModem::OnTimerStop(timerId, data);
    }
}

void GSMModemManager::Flush()
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

Stream *GSMModemManager::GetModemStream()
{
    return serial;
}

bool GSMModemManager::IsBooted()
{
    return modemBootState == MODEM_BOOT_COMPLETED;
}

size_t GSMModemManager::GetCommandStackCount()
{
    return commandStack.Size();
}