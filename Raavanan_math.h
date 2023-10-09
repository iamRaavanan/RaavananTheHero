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
#define RAAVANAN_MATH_H
#endif