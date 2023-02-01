#pragma once

#include "lucy_math.h"

const u32 WINDOW_WIDTH = 1280;
const u32 WINDOW_HEIGHT = 720;
const f32 WINDOW_ASPECT_RATIO = (f32) WINDOW_WIDTH / (f32) WINDOW_HEIGHT;

namespace Colors {
    XMGLOBALCONST XMFLOAT4 White = {1.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Black = {0.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Red = {1.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Green = {0.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Blue = {0.0f, 0.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Yellow = {1.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMFLOAT4 Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
}// namespace Colors


// total memory allocated
#define TOTAL_MEM (100 * 1000000) // 100 MB

#define GET_X_LPARAM(lp) ((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int) (short) HIWORD(lp))


