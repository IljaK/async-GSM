#include "UbloxGSMModemManager.h"

UbloxGSMModemManager::UbloxGSMModemManager(HardwareSerial *serial, int8_t resetPin):GSMModemManager(serial, resetPin)
{
    SetConfig(115200ul, 921600ul, SERIAL_8N1);
}

UbloxGSMModemManager::~UbloxGSMModemManager()
{

}

void UbloxGSMModemManager::ReBootModem(bool hardReset)
{
    if (resetPin > 0) {
        pinMode(resetPin, OUTPUT);
        digitalWrite(resetPin, LOW);

        if (hardReset) {
            // Hardware reset
            digitalWrite(resetPin, HIGH);
            delayMicroseconds(150);
            digitalWrite(resetPin, LOW);
            delay(3);
            digitalWrite(resetPin, HIGH);
            delay(600);
            digitalWrite(resetPin, LOW);
            // Just in case?
            delayMicroseconds(50);
        }
    }
    GSMModemManager::BootModem();
}
