#include "SimComGSMModemManager.h"

SimComGSMModemManager::SimComGSMModemManager(HardwareSerial *serial, int8_t resetPin):GSMModemManager(serial, resetPin)
{

}

SimComGSMModemManager::~SimComGSMModemManager()
{

}

void SimComGSMModemManager::ReBootModem(bool hardReset)
{
    // SimCom A7670 hardware reset
    if (resetPin > 0) {
        pinMode(resetPin, OUTPUT);
        digitalWrite(resetPin, LOW);
        if (hardReset) {
            // Hardware reset
            // The active low level time impulse on RESET pin to reset module
            // 2-2.5 sec
            digitalWrite(resetPin, HIGH);
            delay(2200);
            digitalWrite(resetPin, LOW);
            // Just in case
            delayMicroseconds(50);
        }
    }
    GSMModemManager::BootModem();
}
