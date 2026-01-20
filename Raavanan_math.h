#ifndef RAAVANAN_MATH_H

// struct 
// {
union v2
{
    struct
    {
        float X, Y;
    };
    float E[2];
};
    //float &operator[] (int index) { return ((&X)[index]);}
//};

inline v2 V2(float X, float Y)
{
    v2 Result;
    Result.X = X;
    Result.Y = Y;
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

struct rectangle2
{
    v2 Min;
    v2 Max;
};

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