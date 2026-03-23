#pragma once
#include <windows.h>

namespace GtoMnK {
    bool CreateSingleInstanceMutex();
    void ReleaseSingleInstanceMutex();
}