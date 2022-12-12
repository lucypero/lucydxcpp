// it's mathhhh

namespace math {
    const f32 INF = FLT_MAX;
    const f32 PI = 3.1415926535f;

    static f32 clamp(const f32& x, const f32& low, const f32& high)
    {
        return x < low ? low : (x > high ? high : x);
    }
}
