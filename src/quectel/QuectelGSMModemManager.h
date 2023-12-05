#include "GSMModemManager.h"

constexpr char QUECTEL_GSM_SOCKET_SEND_OK[] = "SEND OK";
constexpr char QUECTEL_GSM_SOCKET_SEND_FAIL[] = "SEND FAIL";

class QuectelGSMModemManager: public GSMModemManager
{
public:
    QuectelGSMModemManager(HardwareSerial *serial, int8_t resetPin);
    virtual ~QuectelGSMModemManager();

    void ReBootModem(bool hardReset = true) override;
    MODEM_RESPONSE_TYPE GetResponseType(BaseModemCMD *cmd, bool isTimeOut, bool isOverFlow) override;
};