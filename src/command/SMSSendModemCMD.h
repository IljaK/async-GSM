#pragma once
#include "BaseModemCMD.h"
#include "common/Util.h"

struct SMSSendModemCMD: public BaseModemCMD
{

    char *phoneNumber = NULL;

    SMSSendModemCMD(char *phoneNumber, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->phoneNumber = (char *)malloc(strlen(phoneNumber) + 1);
        strcpy(this->phoneNumber, phoneNumber);
    }

    virtual ~SMSSendModemCMD() {
        if (phoneNumber != NULL) {
            free(phoneNumber);
            phoneNumber = NULL;
        }
    }

    void WriteStream(Print *stream) override {
        //stream->print(byteData, 10);
    }
};