#ifndef RAAVANAN_INTRINSICS_H
#include "Raavanan.h"
#include "math.h"

inline uint32 RoundFloatToUInt32(float value)
{
	uint32 Result = (uint32)roundf(value);
	return Result;
}

inline int32 RoundFloatToInt32(float value)
{
	int32 Result = (int32)roundf(value);
	return Result;
}

#include "math.h"
inline int32 FloorFloatToInt32(float value)
{
	int32 Result = (int32)floorf(value);
	return Result;
}

inline int32 TruncateFloatToInt32(float value)
{
	int32 Result = (int32)(value);
	return Result;
}

inline float Sin(float Angle)
{
    float Result = sinf(Angle);
    return Result;
}

inline float Cos(float Angle)
{
    float Result = cosf(Angle);
    return Result;
}

inline double ATan2(float Y, float X)
{
    double Result = atan2(Y, X);
    return Result;
}

#define RAAVANAN_INTRINSICS_H
#endif