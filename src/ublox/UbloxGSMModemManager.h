#include "GSMModemManager.h"

class UbloxGSMModemManager: public GSMModemManager
{
public:
    UbloxGSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~UbloxGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
};