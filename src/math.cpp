namespace math {
    const f32 INF = FLT_MAX;
    const f32 PI = 3.1415926535f;

    static f32 clamp(f32 x, f32 low, f32 high) {
        return x < low ? low : (x > high ? high : x);
    }
}
