#pragma once
#include "array/StackArray.h"

struct ModemCommand {
    char *command = NULL;
    unsigned long timeout;
};

class ModemCommandStack: public StackArray<ModemCommand *>
{
protected:
    ModemCommand *CreateItem(char *item, unsigned long timeout) {
        ModemCommand *newItem = (ModemCommand *)malloc(sizeof(ModemCommand));
        if (newItem == NULL) {
            // No more memory?
            return NULL;
        }
        newItem->timeout = timeout;
        newItem->command = (char *)malloc(strlen(item) + 1);
        if (newItem->command == NULL) {
            // No more memory?
            free(newItem);
            return NULL;
        }

        strcpy(newItem->command, item);

        return newItem;
    }
public:
    void FreeItem(ModemCommand * item) override {
        if (item->command != NULL) {
            free(item->command);
        }
        free(item);
    }
    ModemCommandStack(const size_t maxSize):StackArray(maxSize) {}

    bool Contains(char * item)
    {
        if (item == NULL) return false;
        for(size_t i = 0; i < maxSize; i++) {
            if (arr[i] == NULL) return false;
            if (strcmp(item, arr[i]->command) == 0) {
                return true;
            }
        }
        return false;
    }

    bool IsElementEqual(ModemCommand * item1, ModemCommand * item2) override
    {
        if (item1 != NULL && item2 != NULL) {
            if (item1->command != NULL && item2->command != NULL) {
                return strcmp(item1->command, item2->command) == 0;
            }
        }
        return StackArray::IsElementEqual(item1, item2);
    }

    bool Append(char *item, unsigned long timeout)
    {
        if (item == NULL) return NULL;
        if (item[0] == 0) return NULL;
        if (IsFull()) return NULL;
        if (Contains(item)) return NULL;

        ModemCommand *newItem = CreateItem(item, timeout);
        if (newItem == NULL) {
            return false;
        }

        if (!StackArray::Append(newItem)) {
            FreeItem(newItem);
            return false;
        }

        return true;
    }

    bool Insert(char *item, unsigned long timeout, size_t index)
    {
        if (item == NULL) return NULL;
        if (item[0] == 0) return NULL;
        if (IsFull()) return NULL;
        if (Contains(item)) return NULL;

        ModemCommand *newItem = CreateItem(item, timeout);
        if (newItem == NULL) {
            return false;
        }

        if (!StackArray::Insert(newItem, index)) {
            FreeItem(newItem);
            return false;
        }
        return true;
    }

};