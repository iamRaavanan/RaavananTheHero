#ifndef RAAVANAN_RENDER_GROUP_H
#include "Raavanan.h"

struct render_basis
{
    v3 Pos;
};

struct render_entity_basis
{
    render_basis* Basis;
	v2 Offset;
	float OffsetZ;
	float EntityZCofficient;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
};

struct render_group_entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    render_group_entry_header Header;
    float R, G, B, A;
};

struct render_entry_bitmap
{
    render_group_entry_header Header;
	loaded_bitmap* Bitmap;
    render_entity_basis EntityBasis;
    float R, G, B, A;
};

struct render_entry_rectangle
{
    render_group_entry_header Header;
    render_entity_basis EntityBasis;
	float R, G, B, A;
    v2 Dim;
};

struct render_group
{
    render_basis* DefaultBasis;
	float MetersToPixels;
    
    uint32 MaxPushBufferSize;
    uint32 PushBufferSize;
    uint8* PushBufferBase;
};

#define RAAVANAN_RENDER_GROUP_H
#endif