#include "QuectelGSMModemManager.h"
#include "QuectelGSMSocketManager.h"

QuectelGSMModemManager::QuectelGSMModemManager(HardwareSerial *serial, int8_t resetPin):GSMModemManager(serial, resetPin)
{
    SetConfig(115200ul, 921600ul, SERIAL_8N1);
}

QuectelGSMModemManager::~QuectelGSMModemManager()
{

}

void QuectelGSMModemManager::ReBootModem(bool hardReset)
{
    // Quectel EC25E hardware reset
    if (resetPin > 0) {
        pinMode(resetPin, OUTPUT);
        digitalWrite(resetPin, HIGH);
        if (hardReset) {
            // Hardware reset
            // The RESET_N pin can be used to reset the module. 
            // The module can be reset by driving RESET_N low for 150–460 ms
            digitalWrite(resetPin, LOW);
            delay(300);
            digitalWrite(resetPin, HIGH);
            // Just in case
            delayMicroseconds(50);
        }
    }
    GSMModemManager::BootModem();
}


MODEM_RESPONSE_TYPE QuectelGSMModemManager::GetResponseType(BaseModemCMD *cmd, bool isTimeOut, bool isOverFlow)
{
    //Serial.println("QuectelGSMModemManager::GetResponseType");
    if (cmd != NULL && cmd->cmd != NULL && !isTimeOut && !isOverFlow && bufferLength > 0) {
        if (strcmp(cmd->cmd, GSM_QUECTEL_SOCKET_WRITE_CMD) == 0) {
            if (bufferLength >= strlen(QUECTEL_GSM_SOCKET_SEND_OK) && strncmp(buffer, QUECTEL_GSM_SOCKET_SEND_OK, strlen(QUECTEL_GSM_SOCKET_SEND_OK)) == 0) {
                return MODEM_RESPONSE_OK;
            }
            else if (bufferLength >= strlen(QUECTEL_GSM_SOCKET_SEND_FAIL) && strncmp(buffer, QUECTEL_GSM_SOCKET_SEND_FAIL, strlen(QUECTEL_GSM_SOCKET_SEND_FAIL)) == 0) {
                return MODEM_RESPONSE_ERROR;
            }
        }
    }
    return GSMSerialModem::GetResponseType(cmd, isTimeOut, isOverFlow);
}
