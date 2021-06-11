#pragma once
#include "array/StackArray.h"

union MessageSize {
    uint8_t data[8];
    uint64_t l = 0;
};

struct SocketMessage{
    size_t size = 0;
    size_t filled = 0;
    uint8_t * data = NULL;

    bool IsFull() {
        return size > 0 && filled == size;
    }
};

class SocketMessageBuffer: public StackArray<SocketMessage *> {

private:
    const uint8_t lengthBytes;

    uint64_t GetLength(uint8_t * data, size_t length, uint8_t * data2 = NULL, size_t length2 = 0) {

        if ((uint64_t)length + (uint64_t)length2 < (uint64_t)lengthBytes) {
            return 0;
        }
        
        MessageSize msz;
        for (uint8_t i = 0; i < lengthBytes; i++) {
            if (i < length) {
                msz.data[i] = data[i];
            } else {
                msz.data[i] = data2[i - length];
            }
        }

        Serial.print("GetLength: ");
        Serial.println((uint32_t)msz.l);
        return msz.l;
    }

    size_t AppendToItem(SocketMessage *item , uint8_t * data, size_t length) {

        Serial.print("SocketMessageBuffer::AppendToItem ");
        Serial.println(length);

        // TODO: fetch item size from buffer
        if (item->size == 0) {
            // We havent received enough data for length
            item->size = GetLength(item->data, item->filled, data, length);
            if (item->size == 0) {
                // still not enough bytes for length
                if (item->data == NULL) { // nothing been written
                    // Allocate memory for storing "data length size"
                    item->data = (uint8_t *)malloc(lengthBytes);
                    item->filled = length;
                    memcpy(item->data, data, length);
                } else {
                    // We already have allocated memory for length bytes
                    item->data = (uint8_t *)realloc(item->data, item->filled + length);
                    memcpy(item->data + item->filled, data, length);
                    item->filled += length;
                }
                return length;
            } else {
                if (item->data == NULL) {
                    // No data has been stored for length, but content has full of it
                    item->data = (uint8_t *)malloc(item->size);
                    item->filled = length - lengthBytes;
                    if (item->filled > item->size) {
                        item->filled = item->size;
                    }
                    if (item->filled > 0) {
                        memcpy(item->data, data + lengthBytes, item->filled);
                    }
                    return item->filled + lengthBytes;
                } else {
                    // Partly length bytes has been stored, so need to fill proper values
                    size_t sizeRemainPart = lengthBytes - item->filled;
                    size_t dataLen = length - sizeRemainPart;

                    free(item->data); // Flush old stored data
                    item->data = (uint8_t *)malloc(item->size); // Create new buffer with proper size
                    if (item->size >= dataLen) {
                        memcpy(item->data, data + sizeRemainPart, dataLen);
                        item->filled = dataLen;
                    } else {
                        memcpy(item->data, data + sizeRemainPart, item->size);
                        item->filled = item->size;
                    }
                    return item->filled + sizeRemainPart;
                }
            }
        }
        // Append unfilled data with known message size
        size_t append = item->size - item->filled;
        if (append > length) append = length;
        item->filled += append;
        memcpy(item->data + item->filled, data, append);

        return append;

    }

    bool Append(SocketMessage *item) override {
        // TODO:
        return false;
    }

    SocketMessage * InsertNewItem(size_t index) {
        arr[index] = (SocketMessage *)malloc(sizeof(SocketMessage));
        arr[index]->size = 0;
        arr[index]->data = NULL;
        arr[index]->filled = 0;
        size++;
        return arr[index];
    }

    size_t AppendInternal(uint8_t *data, size_t length) {
        if (length == 0) return 0;
        if (data == NULL) return 0;

        if (Size() == 0) {
            return AppendToItem(InsertNewItem(0), data, length);
        }

        SocketMessage *lastItem = arr[Size() - 1];

        if (!lastItem->IsFull()) {
            return AppendToItem(lastItem, data, length);
        } 
        
        // Last element full -> chech if whole stack is filled
        if (Size() == MaxSize()) {
            return 0;
        }
        
        return AppendToItem(InsertNewItem(Size()), data, length);
    }
public:
    void FreeItem(SocketMessage * item) override {
        if (item != NULL) {
            if (item->data != NULL) {
                free(item->data);
            }
            free(item);
        }
    }
    SocketMessageBuffer(const uint8_t lengthBytes, const size_t maxSize):
        StackArray(maxSize),
        lengthBytes(lengthBytes) {
    }

    size_t Append(const uint8_t *item, size_t length) {
        size_t remain = length;
        while (remain > 0) {
            size_t added = AppendInternal((uint8_t *)item + (length - remain), remain);
            if (added == 0) {
                break;
            }
            remain -= added;
        }
        return (length - remain);
    }

    bool HasMessage() {
        if (Size() == 1) {
            return arr[0]->IsFull();
        }
        return Size() > 0;
    }

    size_t TotalLength() {
        size_t totalLength = 0;
        for (size_t i = 0; i < Size(); i++) {
            if (arr[i] != NULL) {
                totalLength += arr[i]->filled;
            }
        }
        return totalLength;
    }

    SocketMessage * Unshift(size_t index) override
    {
        if (size == 0) {
            return NULL;
        }
        if (index >= maxSize) {
            return NULL;
        }
        if (arr[index]->IsFull()) {
            return StackArray::Unshift(index);
        }
        return NULL;
    }
};