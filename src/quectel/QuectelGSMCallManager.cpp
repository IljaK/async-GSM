#include "QuectelGSMCallManager.h"
#include "QuectelGSMNetworkManager.h"
#include "common/GSMUtils.h"

QuectelGSMCallManager::QuectelGSMCallManager(GSMModemManager *gsmManager):GSMCallManager(gsmManager)
{

}

QuectelGSMCallManager::~QuectelGSMCallManager()
{

}


bool QuectelGSMCallManager::OnGSMEvent(char * data, size_t dataLen)
{
    return GSMCallManager::OnGSMEvent(data, dataLen);
}

bool QuectelGSMCallManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    return GSMCallManager::OnGSMResponse(request, response, respLen, type);
}