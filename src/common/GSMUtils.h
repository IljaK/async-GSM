#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

union IPAddr {
	uint8_t octet[4];  // IPv4 address
	uint32_t dword = 0;
};

extern bool GetAddr(const char *addr, IPAddr * ip);

inline bool IsEvent(const char *evName, char *data, size_t dataLen)
{
    const size_t evLen = strlen(evName);
    if (evLen >= dataLen) {
        return false;
    }
    return strncmp(data, evName, evLen) == 0 && data[evLen] == ':';
}
