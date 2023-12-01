#include "GSMModemManager.h"

class QuectelGSMModemManager: public GSMModemManager
{
public:
    QuectelGSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~QuectelGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
};