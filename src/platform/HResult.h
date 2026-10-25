#pragma once

#include <system_error>
#include <windows.h>

// Throws for a failed Windows call the app can't continue without (device creation, window setup).
inline void check(HRESULT result)
{
    if (FAILED(result))
        throw std::system_error(result, std::system_category());
}
