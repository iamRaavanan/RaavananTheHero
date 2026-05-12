#include "Raavanan_render_group.h"

internal render_group* AllocateRenderGroup(memory_arena* Arena, uint32 MaxPushBufferSize, float MetersToPixels)
{
    render_group* Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);

	Result->DefaultBasis = PushStruct(Arena, render_basis);
    Result->DefaultBasis->Pos = V3(0, 0, 0);
    Result->MetersToPixels = MetersToPixels;
	Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;
    return Result;
}