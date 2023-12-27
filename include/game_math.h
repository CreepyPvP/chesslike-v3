#pragma once

#include "include/defines.h"

inline u32 max(u32 a, u32 b)
{
    return a > b? a : b;
}

inline u32 clamp(u32 v, u32 lo, u32 hi)
{
    if (v >= hi)
        return hi;
    else if (v <= lo)
        return lo;

    return v;
}

