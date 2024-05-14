#include "GSMModemManager.h"

class UbloxGSMModemManager: public GSMModemManager
{
public:
    UbloxGSMModemManager(HardwareSerial *serial, int8_t resetPin, uint32_t targetBaudRate = 921600ul);
    virtual ~UbloxGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
};