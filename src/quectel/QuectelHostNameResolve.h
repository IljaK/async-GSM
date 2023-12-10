#pragma once
#include "common/GSMUtils.h"
#include "common/Util.h"

class QuectelHostNameResolve
{
private:
    char *hostName = NULL;
    uint8_t totalCount = 0;
    uint8_t addedCount = 0;
    IPAddr ipAddr;
    uint16_t error = 0;
public:
    QuectelHostNameResolve(char *hostName) {
        this->hostName = makeNewCopy(hostName);
    }

    ~QuectelHostNameResolve() {
        if (this->hostName != NULL) {
            free(this->hostName);
            this->hostName = NULL;
        }
    }

    char *GetHostName() {
        return this->hostName;
    }

    uint16_t GetError() {
        return this->error;
    }

    void SetCount(uint8_t count, uint16_t error) {
        if (totalCount != 0) {
            // Allow reuse?
            return;
        }
        totalCount = count;
        this->error = error;
    }

    void AddIP(IPAddr ip) {
        if (addedCount >= totalCount) return;
        if (addedCount == 0) {
            ipAddr = ip;
        }
        addedCount++;
    }
    
    void AddIP(char *ip) {
        IPAddr addr;
        if (GetAddr(ip, &addr)) {
            AddIP(addr);
        }
    }

    IPAddr GetIP() {
        return ipAddr;
    }

    bool IsFilled() {
        if (error != 0 && (totalCount == 0 || addedCount == totalCount)) return true;
        if (addedCount == 0) return false;
        return addedCount == totalCount;
    }
};