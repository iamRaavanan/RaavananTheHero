#ifndef R_INTRINSICS_H
#include "R.h"
#include "math.h"
#include "stdlib.h"

inline int32 SignOf (int32 value)
{
	int32 Result = (value >= 0) ? 1 : -1;
	return Result;
}

inline float SquareRoot (float value)
{
	float Result = sqrtf (value);
	return Result;
}

inline float AbsoluteValue (float Value)
{
	float Result = (float)fabs(Value);
	return Result;
}

inline uint32 RotateLeft (uint32 Value, int32 Amt)
{
	uint32 Result;
#if COMPILER_MSVC
	Result = _rotl(Value, Amt);
#else
	Amt &= 31;
	Result = (Value << Amt) | (Value >> (32- Amt));
#endif
	return Result;
}

inline uint32 RotateRight (uint32 Value, int32 Amt)
{
	uint32 Result;
#if COMPILER_MSVC
	Result = _rotr(Value, Amt);
#else
	Amt &= 31;
	Result = (Value >> Amt) | (Value << (32- Amt));
#endif
	return Result;
}

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

inline int32 CeilFloatToInt32(float value)
{
	int32 Result = (int32)ceilf(value);
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
struct bit_scan_result
{
	bool Found;
	uint32 Index;
};
inline bit_scan_result FindLSBSetBit (uint32 Value)
{
	bit_scan_result Result = {};
#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
	for (uint32 Test = 0; Test < 32; ++Test)
	{
		if (Value & (1 << Test))
		{
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}
#endif
	return Result;
}

#define R_INTRINSICS_H
#endif