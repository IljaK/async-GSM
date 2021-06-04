#include "GSMUtils.h"


bool GetAddr(char *addr, IPAddr * ip) {

    ip->dword = 0;
    if (addr == NULL || addr[0] == 0 ) {
        return false;
    }

    char octet[4];

    int len = strlen(addr);
    int octetIndex = 0;
    int octetCharIndex = 0;

    for (int i = 0; i <= len; i++) {
        if (addr[i] == '.' || i == len) {
            octet[octetCharIndex] = 0;
            octetCharIndex = 0;
            ip->octet[octetIndex] = atoi(octet);
            octetIndex++;
        } else if (octetCharIndex >= 4) {
            return false;
        } else {
            octet[octetCharIndex] = addr[i];
            octetCharIndex++;
        }
    }

    return true;
}