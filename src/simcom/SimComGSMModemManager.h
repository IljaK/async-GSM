#include "GSMModemManager.h"

class SimComGSMModemManager: public GSMModemManager
{
public:
    SimComGSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~SimComGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
};