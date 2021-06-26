#pragma once
#include "stdint.h"
#include <common/Util.h>

constexpr unsigned long MODEM_COMMAND_TIMEOUT = 500000ul;

enum BaseModemCMDBit {
    CMD_BIT_IS_CHECK = 0,
    CMD_BIT_IS_SET,
    CMD_BIT_QUOTATIONS,
    CMD_BIT_SEMICOLON,
    CMD_BIT_EXTRA_TRIGGER
};

struct BaseModemCMD
{
protected:
    uint8_t descriptorByte = 0;
public:
    BaseModemCMD(const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT, bool isCheck = false, bool isSet = false,
        bool dataQuotations = false, bool semicolon = false):cmd(cmd) {
            this->timeout = timeout;
            IsCheck(isCheck);
            IsSet(isSet);
            InQuatations(dataQuotations);
            EndSemicolon(semicolon);
    }
    virtual ~BaseModemCMD() {}

    const char *cmd;
    unsigned long timeout;
    bool IsCheck() {
        return getBitFromByte(descriptorByte, CMD_BIT_IS_CHECK);
    }
    void IsCheck(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_IS_CHECK);
    }
    bool IsSet() {
        return getBitFromByte(descriptorByte, CMD_BIT_IS_SET);
    }
    void IsSet(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_IS_SET);
    }
    bool InQuatations() {
        return getBitFromByte(descriptorByte, CMD_BIT_QUOTATIONS);
    }
    void InQuatations(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_QUOTATIONS);
    }
    bool EndSemicolon() {
        return getBitFromByte(descriptorByte, CMD_BIT_SEMICOLON);
    }
    void EndSemicolon(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_EXTRA_TRIGGER);
    }
    bool ExtraTrigger() {
        return getBitFromByte(descriptorByte, CMD_BIT_EXTRA_TRIGGER);
    }
    void ExtraTrigger(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_EXTRA_TRIGGER);
    }

    virtual void WriteStream(Print *stream) { };

    virtual char * ExtraTriggerValue()
    { 
        return NULL;
    };
};