#pragma once
#include "array/StackArray.h"
#include "BaseModemCMD.h"

class ModemCommandStack: public StackArray<BaseModemCMD *>
{
public:
    void FreeItem(BaseModemCMD * item) override {
        if (item != NULL) {
            delete item;
        }
    }
    ModemCommandStack(const size_t maxSize):StackArray(maxSize) {}

    bool Contains(char * item)
    {
        if (item == NULL) return false;
        for(size_t i = 0; i < maxSize; i++) {
            if (arr[i] == NULL) return false;
            if (strcmp(item, arr[i]->cmd) == 0) {
                return true;
            }
        }
        return false;
    }

    bool IsElementEqual(BaseModemCMD * item1, BaseModemCMD * item2) override
    {
        if (item1 != NULL && item2 != NULL) {
            if (item1->cmd != NULL && item2->cmd != NULL) {
                return strcmp(item1->cmd, item2->cmd) == 0;
            }
        }
        return StackArray::IsElementEqual(item1, item2);
    }
};