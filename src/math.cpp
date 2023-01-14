namespace math {
    const f32 INF = FLT_MAX;
    const f32 PI = 3.1415926535f;

    fn f32 clamp(f32 x, f32 low, f32 high) {
        return x < low ? low : (x > high ? high : x);
    }

    fn f32 _min(f32 a, f32 b) {
        return a < b ? a : b;
    }

    fn f32 _max(f32 a, f32 b)
    {
        return a > b ? a : b;
    }

    fn u32 _min(u32 a, u32 b) {
        return a < b ? a : b;
    }

    fn u32 _max(u32 a, u32 b)
    {
        return a > b ? a : b;
    }

    fn i32 _min(i32 a, i32 b) {
        return a < b ? a : b;
    }

    fn i32 _max(i32 a, i32 b)
    {
        return a > b ? a : b;
    }

    f32 angle_from_xy(f32 x, f32 y)
    {
        f32 theta = 0.0f;

        // Quadrant I or IV
        if(x >= 0.0f)
        {
            // If x = 0, then atanf(y/x) = +pi/2 if y > 0
            //                atanf(y/x) = -pi/2 if y < 0
            theta = atanf(y / x); // in [-pi/2, +pi/2]

            if(theta < 0.0f)
                theta += 2.0f*PI; // in [0, 2*pi).
        }

            // Quadrant II or III
        else
            theta = atanf(y/x) + PI; // in [0, 2*pi).

        return theta;
    }

	fn XMMATRIX InverseTranspose(CXMMATRIX M)
	{
		// Inverse-transpose is just applied to normals.  So zero out 
		// translation row so that it doesn't get into our inverse-transpose
		// calculation--we don't want the inverse-transpose of the translation.
		XMMATRIX A = M;
		A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		XMVECTOR det = XMMatrixDeterminant(A);
		return XMMatrixTranspose(XMMatrixInverse(&det, A));
	}
}
