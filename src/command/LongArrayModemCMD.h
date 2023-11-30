#pragma once
#include "BaseModemCMD.h"

struct LongArrayModemCMD: public BaseModemCMD
{

    long *values;
    uint8_t valuesCount;

    LongArrayModemCMD(long *values, uint8_t valuesCount, const char *cmd, unsigned long timeout = MODEM_COMMAND_TIMEOUT):BaseModemCMD(cmd, timeout, false, true, false, false) {
        this->valuesCount = valuesCount;
        if (this->valuesCount > 0) {
            this->values = (long*)malloc((size_t)valuesCount * sizeof(long));
            for (uint8_t i = 0; i < valuesCount; i++) {
                this->values[i] = values[i];
            }
        }
    }

    virtual ~LongArrayModemCMD() {}

    void WriteStream(Print *stream) override {
        for (uint8_t i = 0; i < valuesCount; i++) {
            if (i > 0) {
                stream->print(',');
            }
            stream->print(this->values[i], 10);
        }
    }
};