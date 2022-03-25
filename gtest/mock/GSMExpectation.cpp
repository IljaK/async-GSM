#include <gtest/gtest.h>
#include "GSMExpectation.h"

void GSMExpectation::SetExpectation(const char *expectation) {
    if (expectation == NULL) {
        isHandled = false;
        isWaiting = false;
        if (this->expectation != NULL) {
            free(this->expectation);
            this->expectation = NULL;
        }
        return;
    }
    isHandled = false;
    isWaiting = true;
    size_t len = strlen(result);
    this->expectation = (char *)malloc(len+1);
    strcpy(this->expectation, result);
}

void GSMExpectation::HandleResult(char *result) {
    if (!isWaiting) return;

    isHandled = true;
    isWaiting = false;

    if (this->result != NULL) {
        free(this->result);
        this->result = NULL;
    }
    size_t len = strlen(result);
    this->result = (char *)malloc(len+1);
    strcpy(this->result, result);
}

bool GSMExpectation::IsHandled() {
    return isHandled;
}

bool GSMExpectation::IsWaiting() {
    return isWaiting;
}

char *GSMExpectation::GetResult() {
    return this->result;
}