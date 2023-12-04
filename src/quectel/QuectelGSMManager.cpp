#include "QuectelGSMManager.h"

QuectelGSMManager::QuectelGSMManager(HardwareSerial *serial, int8_t resetPin):QuectelGSMModemManager(serial, resetPin),
    gprsManager(this),
    gsmManager(this),
    callManager(this),
    socketManager(this)
{

}

QuectelGSMManager::~QuectelGSMManager()
{
    
}

void QuectelGSMManager::ReBootModem(bool hardReset)
{
    gsmManager.OnModemReboot();
    callManager.OnModemReboot();
    gprsManager.OnModemReboot();
    socketManager.OnModemReboot();
    QuectelGSMModemManager::ReBootModem(hardReset);
}

bool QuectelGSMManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (gsmManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (socketManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (callManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    if (gprsManager.OnGSMResponse(request, response, respLen, type)) {
        return true;
    }
    return false;
}

bool QuectelGSMManager::OnGSMEvent(char * data, size_t dataLen)
{
    if (gsmManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (socketManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (callManager.OnGSMEvent(data, dataLen)) {
        return true;
    }
    if (gprsManager.OnGSMEvent(data, dataLen)) {
        return true;
    }

    if (IsEvent(QUECTEL_URC_EVENT, data, dataLen)) {
        char *urc = data + strlen(QUECTEL_URC_EVENT) + 2;
        char *urcArgs[8];
        size_t count = SplitString(urc, ',', urcArgs, 8, false);
        char *action = ShiftQuotations(urcArgs[0]);

        if (debugPrint != NULL) {
            debugPrint->print("QIURC: "); 
            debugPrint->print(action);
            debugPrint->print(", count: "); 
            debugPrint->println((int)count);
        }

        if (strcmp(action, "closed") == 0) {
            // +QIURC: "pdpdeact",<contextID>
            socketManager.HandleURCClosed(urcArgs, count);
        } else if (strcmp(action, "recv") == 0) {
            // Buffer access mode:
            // +QIURC: "recv",<connectID>
            // Indirect push mode:
            // +QIURC: "recv",<connectID>,<currectrecvlength><CR><LF><data>.
            socketManager.HandleURCRecv(urcArgs, count);
        } else if (strcmp(action, "dnsgip") == 0) {
            // +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
            // +QIURC: "dnsgip",<hostIPaddr>
            gprsManager.HandleDNSGIP(urcArgs, count);
        } else if (strcmp(action, "pdpdeact") == 0) {
            // +QIURC: "pdpdeact",<contextID>
            gprsManager.HandlePDEACT(urcArgs, count);
        } else if (strcmp(action, "incoming") == 0) {
            // +QIURC: "incoming",<connectID>,<serverID>,<remoteIP>,<remote_port>
            socketManager.HandleURCIncoming(urcArgs, count);
        } else if (strcmp(action, "incoming full") == 0) {
            // +QIURC: "incoming full"
            // If the incoming connection reaches the limit, or no socket system resources can be allocated.
        }
        //ForceDeactivate();
        return true;
    }


    return false;
}


bool QuectelGSMManager::OnGSMExpectedData(uint8_t * data, size_t dataLen)
{
    if (socketManager.OnGSMExpectedData(data, dataLen)) {
        return true;
    }
    return false;
}