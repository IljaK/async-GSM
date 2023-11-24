#pragma once
#include "common/GSMUtils.h"

class IGPRSHandler
{
public:
    virtual void OnGPRSActivated() = 0;
    virtual void OnGPRSDeactivated() = 0;
    virtual void OnHostNameResolve(const char *hostName, IPAddr ip, bool isSuccess) = 0;
    //virtual void OnDynDnsActivated(bool isSuccess) = 0;
    //virtual void OnDynDnsDeactivated(bool isSuccess) = 0;
};