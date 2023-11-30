#include "GSMModemManager.h"

constexpr char SIMCOM_MODEM_BOOT_EVENT[] = "*ATREADY"; 

class SimComGSMModemManager: public GSMModemManager
{
public:
    SimComGSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~SimComGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
};