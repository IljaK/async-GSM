#include "QuectelGPRSManager.h"
#include "command/BaseModemCMD.h"
#include "command/ByteModemCMD.h"
#include "command/ULong2ModemCMD.h"
#include "command/ByteCharModemCMD.h"
#include "command/Byte2Char2ModemCMD.h"
#include "command/ByteChar2ModemCMD.h"

QuectelGPRSManager::QuectelGPRSManager(GSMModemManager *gsmManager):GPRSManager(gsmManager), gipTimer(this)
{

}
QuectelGPRSManager::~QuectelGPRSManager()
{

}

bool QuectelGPRSManager::ConnectInternal()
{
    HandleAPNUpdate(GSM_APN_ACTIVATING);
    ActivatePDP();
    return true;
}

void QuectelGPRSManager::OnCGACT(bool isCheck, bool value, bool isSuccess)
{
    if (isCheck) {
        GPRSManager::OnCGACT(isCheck, value, isSuccess);
        return;
    }
    if (!isSuccess || !value) {
        GPRSManager::OnCGACT(isCheck, value, isSuccess);
        return;
    }
    ActivatePDP();
}

void QuectelGPRSManager::ActivatePDP()
{
    // Set apn
    // AT+CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<data_comp>,<head_comp>,<IPv4_addr_alloc>,<request_type>
    char content[64];
    content[0] = 0;
    strcat(content, "1,\"IP\",\"");
    strcat(content, apn);
    strcat(content, "\"");

    gsmManager->ForceCommand(new CharModemCMD(content, QUECTEL_DEFINE_PDP));
}

bool QuectelGPRSManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strcmp(request->cmd, QUECTEL_RESOLVE_DNS_CMD) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gipTimer.StartMicros(30000000ul);
            // Wait URC:
            // +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
            // +QIURC: "dnsgip",<hostIPaddr>
        } else if (type >= MODEM_RESPONSE_ERROR) {
            InternalHostNameResolve();
        }
        return true;
    }
    if (strcmp(request->cmd, QUECTEL_DEFINE_PDP) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            // Set apn auth
           // AT+QICSGP=<contextID>[,<context_type>,<APN>[,<username>,<password>)[,<authentication>[,<cdma_pwd>]]]]
            char cgdcontContent[128];
            cgdcontContent[0] = 0;
            strcat(cgdcontContent, "1,1,\"");
            strcat(cgdcontContent, apn);
            strcat(cgdcontContent, "\",\"");
            strcat(cgdcontContent, login);
            strcat(cgdcontContent, "\",\"");
            strcat(cgdcontContent, password);
            strcat(cgdcontContent, "\",");
            if ((password != NULL && password[0]!= 0) || (login != NULL && login[0]!= 0)) {
                // PAP or CHAP
                strcat(cgdcontContent, "3");
            } else {
                strcat(cgdcontContent, "0");
            }

            gsmManager->ForceCommand(new CharModemCMD(cgdcontContent, QUECTEL_AUTH_PDP, 1000000ul));
        } else if (type > MODEM_RESPONSE_OK) {
            ForceDeactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, QUECTEL_AUTH_PDP) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            // Activate
            gsmManager->ForceCommand(new ByteModemCMD(1, QUECTEL_ACTIVATE_PDP, APN_CONNECT_CMD_TIMEOUT));
        } else if (type > MODEM_RESPONSE_OK) {
            ForceDeactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, QUECTEL_ACTIVATE_PDP) == 0) {
        if (request->GetIsCheck()) {
            // Fetching IP
            if (type == MODEM_RESPONSE_OK) {
                if (deviceAddr.dword == 0) {
                    ForceDeactivate();
                } else {
                    HandleAPNUpdate(GSM_APN_ACTIVE);
                }

            } else if (type == MODEM_RESPONSE_DATA) {
                // +QIACT: 1,1,1,"10.5.160.5"
                // +QIACT: 1,<context_state>,<context_type>[,<IP_address>]
                char *qiact = response + strlen(QUECTEL_ACTIVATE_PDP) + 2;
                char *qiactArgs[4];
                SplitString(qiact, ',', qiactArgs, 4, false);
                GetAddr(ShiftQuotations(qiactArgs[3]), &deviceAddr);
            } else if (type > MODEM_RESPONSE_OK) {
                ForceDeactivate();
            }

        } else {
            if (type == MODEM_RESPONSE_OK) {
                // Fetch IP address
                gsmManager->ForceCommand(new BaseModemCMD(QUECTEL_ACTIVATE_PDP, MODEM_COMMAND_TIMEOUT, true));
            } else if (type > MODEM_RESPONSE_OK) {
                ForceDeactivate();
            }
        }
        return true;
    }

    if (strcmp(request->cmd, QUECTEL_DEACTIVATE_PDP) == 0) {
        if (type >= MODEM_RESPONSE_OK) {
            HandleAPNUpdate(GSM_APN_DEACTIVATED);
        }
        return true;
    }

    return GPRSManager::OnGSMResponse(request, response, respLen, type);
}
bool QuectelGPRSManager::OnGSMEvent(char * data, size_t dataLen)
{
    /*
    if (IsEvent(GSM_ACTIVATE_IP_CONTEXT_CMD, data, dataLen)) {
        char *netopen = data + strlen(GSM_ACTIVATE_IP_CONTEXT_CMD) + 2;
        //uint8_t err = atoi(netopen);

        if (GetApnState() == GSM_APN_ACTIVATING) {
            if (atoi(netopen) == 0) {
                // NO ERROR, succeeded
                HandleAPNUpdate(GSM_APN_ACTIVE);
            } else {
                // TODO: ?
                ForceDeactivate();
            }
        }
        return true;
    } */
    return GPRSManager::OnGSMEvent(data, dataLen);
}

bool QuectelGPRSManager::ResolveHostNameCMD(const char *hostName)
{
    if (hostnameResolve != NULL) return false;
    hostnameResolve = new QuectelHostNameResolve((char*)hostName);
    return gsmManager->AddCommand(new ByteCharModemCMD(1, hostName, QUECTEL_RESOLVE_DNS_CMD));
}

void QuectelGPRSManager::HandlePDEACT(char **args, size_t argsLen)
{
    // args[0] == "pdpdeact"
    ForceDeactivate();
}
void QuectelGPRSManager::HandleDNSGIP(char **args, size_t argsLen)
{
    // +QIURC: "dnsgip",<err>,<IP_count>,<DNS_ttl>
    // +QIURC: "dnsgip",<hostIPaddr>

    if (hostnameResolve == NULL) {
        // Just do nothing
        return;
    }

    // args[0] == "dnsgip"
    if (argsLen == 4) {
        hostnameResolve->SetCount(atoi(args[2]), atoi(args[1]));
    } else if (argsLen == 2) {
        // Handle IP
        hostnameResolve->AddIP(args[1]);
    } else {
        // ?
        return;
    }

    if (hostnameResolve->IsFilled()) {
        InternalHostNameResolve();
    }
}


void QuectelGPRSManager::OnTimerComplete(Timer *timer)
{
    if (timer == &gipTimer) {
        InternalHostNameResolve();
        return;
    }
    //GPRSManager::OnTimerComplete(timer);
}


void QuectelGPRSManager::InternalHostNameResolve()
{
    gipTimer.Stop();
    QuectelHostNameResolve *h = hostnameResolve;
    hostnameResolve = NULL;

    IPAddr ip = h->GetIP();
    /*
    Serial.print("GetIP: ");
    Serial.print((int)ip.octet[0]);
    Serial.print('.');
    Serial.print((int)ip.octet[1]);
    Serial.print('.');
    Serial.print((int)ip.octet[2]);
    Serial.print('.');
    Serial.println((int)ip.octet[3]);
    */

    HandleHostNameResolve(h->GetHostName(), h->GetIP());
    delete h;
}

void QuectelGPRSManager::FlushAuthData()
{
    if (hostnameResolve != NULL) {
        delete hostnameResolve;
        hostnameResolve = NULL;
    }
    GPRSManager::FlushAuthData();
}

bool QuectelGPRSManager::ForceDeactivate()
{
    HandleAPNUpdate(GSM_APN_DEACTIVATING);
    gsmManager->ForceCommand(new ByteModemCMD(1, QUECTEL_DEACTIVATE_PDP, APN_CONNECT_CMD_TIMEOUT));
    return true;
}
