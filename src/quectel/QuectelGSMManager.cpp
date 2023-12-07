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

    if (IsEvent(GSM_QUECTEL_URC_EVENT, data, dataLen)) {
        char *urc = data + strlen(GSM_QUECTEL_URC_EVENT) + 2;
        char *urcArgs[8];
        size_t count = SplitString(urc, ',', urcArgs, 8, false);
        ShiftQuotations(urcArgs, count);

        if (debugPrint != NULL) {
            debugPrint->print("QIURC: "); 
            debugPrint->print(urcArgs[0]);
            debugPrint->print(", count: "); 
            debugPrint->println((int)count);
        }

        if (strcmp(urcArgs[0], "closed") == 0) {
            // +QIURC: "pdpdeact",<contextID>
            socketManager.HandleURCClosed(urcArgs, count);
        } else if (strcmp(urcArgs[0], "recv") == 0) {
            // Buffer access mode:
            // +QIURC: "recv",<connectID>
            // Indirect push mode:
            // +QIURC: "recv",<connectID>,<currectrecvlength><CR><LF><data>.
            socketManager.HandleURCRecv(urcArgs, count);
        } else if (strcmp(urcArgs[0], "dnsgip") == 0) {
            // +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
            // +QIURC: "dnsgip",<hostIPaddr>
            gprsManager.HandleDNSGIP(urcArgs, count);
        } else if (strcmp(urcArgs[0], "pdpdeact") == 0) {
            // +QIURC: "pdpdeact",<contextID>
            gprsManager.HandlePDEACT(urcArgs, count);
        } else if (strcmp(urcArgs[0], "incoming") == 0) {
            // +QIURC: "incoming",<connectID>,<serverID>,<remoteIP>,<remote_port>
            socketManager.HandleURCIncoming(urcArgs, count);
        } else if (strcmp(urcArgs[0], "incoming full") == 0) {
            // +QIURC: "incoming full"
            // If the incoming connection reaches the limit, or no socket system resources can be allocated.
        } else {
            // TODO: ?
        }
        //ForceDeactivate();
        return true;
    }

    if (IsEvent(GSM_QUECTEL_URC_IND_EVENT, data, dataLen)) {

        char *urci = data + strlen(GSM_QUECTEL_URC_IND_EVENT) + 2;
        char *urciArgs[9];
        size_t count = SplitString(urci, ',', urciArgs, 9, false);
        ShiftQuotations(urciArgs, count);

        if (strcmp(urciArgs[0], "csq") == 0) {
            gsmManager.UpdateSignalQuality(atoi(urciArgs[1]), atoi(urciArgs[2]));
        } else if (strcmp(urciArgs[0], "smsfull") == 0) {

        } else if (strcmp(urciArgs[0], "ring") == 0) {
            
        } else if (strcmp(urciArgs[0], "smsincoming") == 0) {
            
        } else if (strcmp(urciArgs[0], "act") == 0) {
            gsmManager.UpdateNetworkType(urciArgs[1]);
        } else if (strcmp(urciArgs[0], "ccinfo") == 0) {
            // +QIND: "ccinfo",<id>,<dir>,<state>,<mode>,<mpty>,<number>,<type>[,<alpha>]
            // +QIND: "ccinfo",2,1,-1,0,0,"+372132123",145,"Ilja"
            VOICE_CALLSTATE state = VOICE_CALL_STATE_ACTIVE;
            switch (atoi(urciArgs[3]))
            {
            case 1: // 1 CALL_HOLD
                state = VOICE_CALL_STATE_HELD;
                break;
            case 2: // 2 CALL_ORIGINAL
                state = VOICE_CALL_STATE_DIALING;
                break;
            case 3: // 3 CALL_CONNECT
                state = VOICE_CALL_STATE_ACTIVE;
                break;
            case 4: // 4 CALL_INCOMING
                state = VOICE_CALL_STATE_INCOMING;
                break;
            case 5: // 5 CALL_WAITING
                state = VOICE_CALL_STATE_WAITING;
                break;
            case 6: // 6 CALL_END
                state = VOICE_CALL_STATE_DISCONNECTED;
                break;
            case 7: // 7 CALL_ALERTING
                state = VOICE_CALL_STATE_ALERTING;
                break;
            default:
                break;
            }
            callManager.HandleDetailedCallInfo(state, urciArgs[6], urciArgs[8]);

        } else {
            // TODO: ?
        }

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