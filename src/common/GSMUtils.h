#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

union IPAddr {
	uint8_t octet[4];  // IPv4 address
	uint32_t dword;
};

extern bool GetAddr(char *addr, IPAddr * ip);
