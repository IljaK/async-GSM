#include "UbloxGSMModemManager.h"

UbloxGSMModemManager::UbloxGSMModemManager(HardwareSerial *serial, int8_t resetPin, uint32_t targetBaudRate):GSMModemManager(serial, resetPin)
{
    SetConfig(115200ul, targetBaudRate, SERIAL_8N1);
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
            // The RESET_N input line has to be driven as described in Figure 5 to perform an abrupt “external” or
            // “hardware” reset (reboot) of the SARA-U201 modules:
            // RESET_N line has to be set to the LOW level for 50 ms (minimum)

            // inverted signal
            delayMicroseconds(50);
            digitalWrite(resetPin, HIGH);
            delay(500);
            digitalWrite(resetPin, LOW);

            // The RESET_N input line has to be driven as described in Figure 6 to perform an abrupt “external” or
            // “hardware” reset (reboot) of the SARA-U260, SARA-U270 and SARA-U280 modules:
            // * First, RESET_N line has to be set to the LOW level for 100 µs (minimum) to 200 µs (maximum)
            // * Then, RESET_N line has to be released to the HIGH level for 2 ms (minimum) to 4 ms (maximum)
            // * Then, RESET_N line has to be set to the LOW level for 500 ms (minimum)
            
            /*
            digitalWrite(resetPin, HIGH);
            delayMicroseconds(150);
            digitalWrite(resetPin, LOW);
            delay(3);
            digitalWrite(resetPin, HIGH);
            delay(600);
            digitalWrite(resetPin, LOW);
            */
            // Just in case?
            delayMicroseconds(50);
        }
    }
    GSMModemManager::BootModem();
}
