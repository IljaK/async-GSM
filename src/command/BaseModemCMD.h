#pragma once
#include "stdint.h"
#include <common/Util.h>

constexpr unsigned long MODEM_COMMAND_TIMEOUT = 1000000ul;

enum BaseModemCMDBit {
    CMD_BIT_IS_CHECK = 0,
    CMD_BIT_IS_SET,
    CMD_BIT_QUOTATIONS,
    CMD_BIT_SEMICOLON,
    CMD_BIT_EXTRA_TRIGGER,
    CMD_BIT_RESP_BEGIN
};

struct BaseModemCMD
{
protected:
    uint8_t descriptorByte = 0;
public:
    const char *cmd;
    const size_t cmdLen;
    unsigned long timeout;

    BaseModemCMD(const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT, bool isCheck = false, bool isSet = false,
        bool dataQuotations = false, bool semicolon = false):
        cmd(cmd),
        cmdLen(cmd == NULL ? 0 : strlen(cmd))
    {
            this->timeout = timeout;
            SetIsCheck(isCheck);
            SetIsModifier(isSet);
            SetInQuatations(dataQuotations);
            SetEndSemicolon(semicolon);
    }
    virtual ~BaseModemCMD() {}

    bool GetIsCheck() {
        return getBitFromByte(descriptorByte, CMD_BIT_IS_CHECK);
    }
    void SetIsCheck(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_IS_CHECK);
    }
    bool GetIsModifier() {
        return getBitFromByte(descriptorByte, CMD_BIT_IS_SET);
    }
    void SetIsModifier(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_IS_SET);
    }
    bool GetInQuatations() {
        return getBitFromByte(descriptorByte, CMD_BIT_QUOTATIONS);
    }
    void SetInQuatations(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_QUOTATIONS);
    }
    bool GetEndSemicolon() {
        return getBitFromByte(descriptorByte, CMD_BIT_SEMICOLON);
    }
    void SetEndSemicolon(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_EXTRA_TRIGGER);
    }
    bool GetHasExtraTrigger() {
        return getBitFromByte(descriptorByte, CMD_BIT_EXTRA_TRIGGER);
    }
    void SetHasExtraTrigger(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_EXTRA_TRIGGER);
    }
    bool GetIsRespStarted() {
        return getBitFromByte(descriptorByte, CMD_BIT_RESP_BEGIN);
    }
    void SetIsRespStarted(bool value) {
        setBitsValue(&descriptorByte, value, 1, CMD_BIT_RESP_BEGIN);
    }

    virtual void WriteStream(Print *stream) { };

    virtual char * ExtraTriggerValue()
    { 
        return NULL;
    };
};