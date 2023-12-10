#include "SimComGPRSManager.h"
#include "command/ULong2ModemCMD.h"
#include "command/IPResolveModemCMD.h"
#include "command/Byte2Char2ModemCMD.h"
#include "command/ByteChar2ModemCMD.h"

SimComGPRSManager::SimComGPRSManager(GSMModemManager *gsmManager):GPRSManager(gsmManager)
{

}
SimComGPRSManager::~SimComGPRSManager()
{

}

void SimComGPRSManager::FlushAuthData()
{

}


bool SimComGPRSManager::ConnectInternal()
{
    HandleAPNUpdate(GSM_APN_ACTIVATING);
    // Set apn
    gsmManager->ForceCommand(new ByteChar2ModemCMD(1, "IP", apn, SIMCOM_DEFINE_PDP));
    return false;
}

bool SimComGPRSManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    if (strcmp(request->cmd, SIMCOM_RESOLVE_DNS_CMD) == 0) {
        IPResolveModemCMD *ipResolve = (IPResolveModemCMD *)request;
        if (type == MODEM_RESPONSE_DATA) {
            // +CDNSGIP: 1,"ac.spinn.ee","139.162.130.136"

            char *cdnsgip = response + strlen(SIMCOM_RESOLVE_DNS_CMD) + 2;
            char *cdnsgipArgs[3];
            SplitString(cdnsgip, ',', cdnsgipArgs, 3, false);
            GetAddr(ShiftQuotations(cdnsgipArgs[2]), &ipResolve->ipAddr);
        } else {
            HandleHostNameResolve(ipResolve->charData, ipResolve->ipAddr);
        }
        return true;
    }
    if (strcmp(request->cmd, SIMCOM_DEFINE_PDP) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            // Set apn auth
            if (login != NULL && login[0] != 0 && password != NULL && password[0] != 0) {
                // login + password
                // AT+CGAUTH=1,1,"PASSWORD","LOGIN"
                gsmManager->ForceCommand(new Byte2Char2ModemCMD(1, 1, password, login, SIMCOM_AUTH_PDP, 9000000ul));
            } else if (password != NULL && password[0] != 0) {
                // password only
                // AT+CGAUTH=1,2,"PASSWORD"
                gsmManager->ForceCommand(new Byte2CharModemCMD(1, 2, password, SIMCOM_AUTH_PDP, 9000000ul));
            } else {
                // no auth
                // AT+CGAUTH=1,0
                gsmManager->ForceCommand(new Byte2ModemCMD(1, 0, SIMCOM_AUTH_PDP, 9000000ul));
            }
        } else if (type > MODEM_RESPONSE_OK) {
            ForceDeactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, SIMCOM_AUTH_PDP) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            // TODO: Activate
            gsmManager->ForceCommand(new Byte2ModemCMD(1, 1, SIMCOM_ACTIVATE_PDP, 9000000ul));
        } else if (type > MODEM_RESPONSE_OK) {
            ForceDeactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, SIMCOM_ACTIVATE_PDP) == 0) {
        if (type == MODEM_RESPONSE_OK) {
            gsmManager->ForceCommand(new ByteModemCMD(1, SIMCOM_GET_PDP_IP, 9000000ul));
        } else if (type > MODEM_RESPONSE_OK) {
            ForceDeactivate();
        }
        return true;
    }
    if (strcmp(request->cmd, SIMCOM_GET_PDP_IP) == 0) {
        if (type == MODEM_RESPONSE_DATA) {
            char *cgpaddr = response + strlen(SIMCOM_GET_PDP_IP) + 2;
            char *cgpaddrArgs[2];
            SplitString(cgpaddr, ',', cgpaddrArgs, 2, false);
            GetAddr(ShiftQuotations(cgpaddrArgs[1]), &deviceAddr);
        } else if (type == MODEM_RESPONSE_OK) {

            gsmManager->ForceCommand(new BaseModemCMD(GSM_ACTIVATE_IP_CONTEXT_CMD, 10000000ul, true));

            //HandleAPNUpdate(GSM_APN_ACTIVE);
            // TODO: check

        } else if (type >= MODEM_RESPONSE_ERROR) {
            ForceDeactivate();
        }
        return true;
    }

    if (strcmp(request->cmd, GSM_ACTIVATE_IP_CONTEXT_CMD) == 0) {
        if (request->GetIsCheck()) {
            // Verify that IP functionality is active
            if (type == MODEM_RESPONSE_DATA) {
                // +NETOPEN: <net_state>
                // Integer type, indicates the state of PDP context activation. 0 network close (deactivated)
                // 1 network open(activated)
                char *stateChar = response + strlen(GSM_ACTIVATE_IP_CONTEXT_CMD) + 2;
                uint8_t state = atoi(stateChar);
                if (state == 1) {
                    // Enabled, continue to create socket
                    HandleAPNUpdate(GSM_APN_ACTIVE);
                } else {
                    gsmManager->ForceCommand(new BaseModemCMD(GSM_ACTIVATE_IP_CONTEXT_CMD));
                }
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // Failed to fetch status?
                ForceDeactivate();
                
            }
        } else {
            //ByteModemCMD *enableSocket = (ByteModemCMD *)request;
            if (type == MODEM_RESPONSE_OK) {
                // TODO: Continue socket connection
                // HandleAPNUpdate(GSM_APN_ACTIVE);
                // Wait for apn activation event!
                
            } else if (type >= MODEM_RESPONSE_ERROR) {
                // Failed to enable socket service
                ForceDeactivate();
                
            }
        }
        return true;
    }


    return GPRSManager::OnGSMResponse(request, response, respLen, type);
}
bool SimComGPRSManager::OnGSMEvent(char * data, size_t dataLen)
{
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
    }
    if (IsEvent(GSM_SIMCOM_NETWORK_CLOSE_EVENT, data, dataLen)) {
        ForceDeactivate();
        return true;
    }
    return GPRSManager::OnGSMEvent(data, dataLen);
}

bool SimComGPRSManager::ResolveHostNameCMD(const char *hostName)
{
    return gsmManager->AddCommand(new IPResolveModemCMD(hostName, SIMCOM_RESOLVE_DNS_CMD, APN_CONNECT_CMD_TIMEOUT));
}