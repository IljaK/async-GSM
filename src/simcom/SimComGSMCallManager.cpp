#include "SimComGSMCallManager.h"
#include "SimComGSMNetworkManager.h"
#include "common/GSMUtils.h"

SimComGSMCallManager::SimComGSMCallManager(GSMModemManager *gsmManager):GSMCallManager(gsmManager)
{

}

SimComGSMCallManager::~SimComGSMCallManager()
{

}


bool SimComGSMCallManager::OnGSMEvent(char * data, size_t dataLen)
{
    return GSMCallManager::OnGSMEvent(data, dataLen);
}

bool SimComGSMCallManager::OnGSMResponse(BaseModemCMD *request, char * response, size_t respLen, MODEM_RESPONSE_TYPE type)
{
    return GSMCallManager::OnGSMResponse(request, response, respLen, type);
}