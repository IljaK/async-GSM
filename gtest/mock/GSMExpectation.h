#pragma once
#include <stdio.h>

class GSMExpectation
{
private:
    bool isWaiting = false;
    bool isHandled = false;
    char *expectation = NULL;
    char *result = NULL;

public:

    void SetExpectation(const char *expectation);

    void HandleResult(char *result);

    bool IsHandled();

    bool IsWaiting();

    char *GetResult();
};