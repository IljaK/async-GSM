#include "QuectelGSMModemManager.h"

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
