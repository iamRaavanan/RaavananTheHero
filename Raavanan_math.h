#ifndef RAAVANAN_MATH_H

union v2
{
    struct
    {
        float X, Y;
    };
    float E[2];
};

union v3
{
    struct
    {
        float X, Y, Z;
    };
    struct
    {
        float R, G, B;
    };
    float E[3];
};

union v4
{
    struct
    {
        float X, Y, Z, W;
    };
    struct
    {
        float R, G, B, A;
    };
    float E[4];
};

inline v2 V2(float X, float Y)
{
    v2 Result;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

inline v3 V3(float X, float Y, float Z)
{
    v3 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    return Result;
}

inline v4 V4(float X, float Y, float Z, float W)
{
    v4 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
    return Result;
}

inline v2 operator* (float A, v2 B)
{
    v2 Result;
    Result.X = A * B.X;
    Result.Y = A * B.Y;
    return Result;
}

inline v2 operator* (v2 A, float B)
{
    v2 Result = B * A;
    return Result;
}

inline v2 &operator*=(v2 &B, float A)
{
    B = A * B;
    return B;
}

inline v2 operator- (v2 A)
{
    v2 Result;
    Result.X = -A.X;
    Result.Y = -A.Y;
    return Result;
}

inline v2 operator+ (v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}

inline v2 &operator+=(v2 &A, v2 B)
{
    A = A + B;
    return A;
}

inline v2 operator- (v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return Result;
}

inline float Square (float A)
{
    float Result = A * A;
    return Result;
}

inline float Dot (v2 A, v2 B)
{
    float Result = A.X * B.X + A.Y * B.Y;
    return Result;
}

inline float LengthSq(v2 A)
{
    float Result = Dot(A, A);
    return Result;
}
inline float Length(v2 A)
{
    float Result = SquareRoot(LengthSq(A));
    return Result;
}
struct rectangle2
{
    v2 Min;
    v2 Max;
};

inline v2 GetMinCorner(rectangle2 Rect)
{
    v2 Result = Rect.Min;
    return Result;
}

inline v2 GetMaxCorner(rectangle2 Rect)
{
    v2 Result = Rect.Max;
    return Result;
}

inline v2 GetCenter(rectangle2 Rect)
{
    v2 Result = 0.5f * (Rect.Min + Rect.Max);
    return Result;
}

inline rectangle2 RectMinMax (v2 Min, v2 Max)
{
    rectangle2 Result;
    Result.Min = Min;
    Result.Max = Max;
    return Result;
}

inline rectangle2 RectMinDim (v2 Min, v2 Dim)
{
    rectangle2 Result;
    Result.Min = Min;
    Result.Max = Min + Dim;
    return Result;
}

inline rectangle2 RectCenterHalfDim (v2 Center, v2 HalfDim)
{
    rectangle2 Result;
    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;
    return Result;
}

inline rectangle2 RectCenterDim (v2 Center, v2 Dim)
{
    rectangle2 Result = RectCenterHalfDim(Center, 0.5f * Dim);
    return Result;
}

inline bool IsInRectangle (rectangle2 Rect, v2 Test)
{
    bool Result = ((Test.X >= Rect.Min.X) && (Test.Y >= Rect.Min.Y) &&
                    (Test.X < Rect.Max.X) && (Test.Y < Rect.Max.Y));
    return Result;
}
#define RAAVANAN_MATH_H
#endif