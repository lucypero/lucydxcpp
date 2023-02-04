#pragma once    

#include "lucytypes.h"
#include "DirectXMath.h"
#include "DirectXPackedVector.h"
using namespace DirectX;

namespace math {
    const f32 INF = FLT_MAX;
    const f32 PI = 3.1415926535f;
    f32 clamp(f32 x, f32 low, f32 high);
    i32 clamp(i32 x, i32 low, i32 high);
    u32 clamp(u32 x, u32 low, u32 high);
    f32 _min(f32 a, f32 b);
    f32 _max(f32 a, f32 b);
    u32 _min(u32 a, u32 b);
    u32 _max(u32 a, u32 b);
    i32 _min(i32 a, i32 b);
    i32 _max(i32 a, i32 b);
    f32 angle_from_xy(f32 x, f32 y);
    XMMATRIX InverseTranspose(CXMMATRIX M);
}