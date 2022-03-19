#pragma once
#include "ULongModemCMD.h"

enum SIM_ENTRY_DATA_TYPE {
    SIM_ENTRY_NUMBER = 0,
    SIM_ENTRY_TEXT,
    SIM_ENTRY_GROUP,
    SIM_ENTRY_AD_NUMBER,
    SIM_ENTRY_AD_TYPE,
    SIM_ENTRY_SECOND_TEXT,
    SIM_ENTRY_EMAIL,

    SIM_ENTRY_TOTAL_AMOUNT
};

constexpr char GSM_WRITE_USER_CMD[] = "+CPBW"; // Write phonebook entry command

class SimWriteEntryModemCMD: public ULongModemCMD
{
private:

    char *contentData[SIM_ENTRY_TOTAL_AMOUNT] = {NULL};
    uint8_t contentAmount = 0;

public:
    bool IsHidden = false;

    SimWriteEntryModemCMD(unsigned long valueData, unsigned long timeout = MODEM_COMMAND_TIMEOUT):
        ULongModemCMD(valueData, GSM_WRITE_USER_CMD, timeout) {
    }

    virtual ~SimWriteEntryModemCMD() {
        for (uint8_t i = 0; i < SIM_ENTRY_TOTAL_AMOUNT; i++) {
            if (contentData[i] != NULL) {
                free(contentData[i]);
            }
        }
    }

    void SetContent(char *number, char *text, char *group = NULL, char *adNumber = NULL, char *adType = NULL, char *secondText = NULL, char *email = NULL, bool isHidden = false) {
        this->IsHidden = isHidden;
        contentData[SIM_ENTRY_NUMBER] = makeNewCopy(number);
        contentData[SIM_ENTRY_TEXT] = makeNewCopy(text);
        contentData[SIM_ENTRY_GROUP] = makeNewCopy(group);
        contentData[SIM_ENTRY_AD_NUMBER] = makeNewCopy(adNumber);
        contentData[SIM_ENTRY_AD_TYPE] = makeNewCopy(adType);
        contentData[SIM_ENTRY_SECOND_TEXT] = makeNewCopy(secondText);
        contentData[SIM_ENTRY_EMAIL] = makeNewCopy(email);
        if (email != NULL) {
            contentAmount = 7;
        }
        else if (secondText != NULL) {
            if (contentAmount < 6) {
                contentAmount = 6;
            }
        }
        else if (adType != NULL) {
            if (contentAmount < 5) {
                contentAmount = 5;
            }
        }
        else if (adNumber != NULL) {
            if (contentAmount < 4) {
                contentAmount = 4;
            }
        }
        else if (group != NULL) {
            if (contentAmount < 3) {
                contentAmount = 3;
            }
        }
        else if (text != NULL) {
            if (contentAmount < 2) {
                contentAmount = 2;
            }
        }
        else if (number != NULL) {
            if (contentAmount < 1) {
                contentAmount = 1;
            }
        }
    }

    void SetContent(char *data, SIM_ENTRY_DATA_TYPE type) {
        if (contentAmount < type + 1) {
            contentAmount = (int)type + 1;
        }
        if (contentData[type] != NULL) {
            free(contentData[type]);
        }
        contentData[type] = makeNewCopy(data);
    }

    void WriteStream(Print *stream) override {
        ULongModemCMD::WriteStream(stream);

        for (uint8_t i = 0; i < contentAmount; i++) {
            stream->write(',');
            stream->write('"');
            if (contentData[i] != NULL) {
                stream->write(contentData[i]);
            }
            stream->write('"');
            
            if (i == 0) {
                stream->write(',');
            }
        }
    }

    char *GetContentData(SIM_ENTRY_DATA_TYPE type) {
        return contentData[type];
    }

    bool IsEmpty() {
        return contentAmount == 0;
    }

};