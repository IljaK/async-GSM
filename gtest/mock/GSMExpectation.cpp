#include <gtest/gtest.h>
#include "GSMExpectation.h"
#include "common/Util.h"

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
    this->expectation = makeNewCopy(expectation);
}

void GSMExpectation::HandleResult(char *result) {
    if (!isWaiting) return;

    isHandled = true;
    isWaiting = false;

    if (this->result != NULL) {
        free(this->result);
        this->result = NULL;
    }
    this->result = makeNewCopy(result);
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