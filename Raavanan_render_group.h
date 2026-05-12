#ifndef RAAVANAN_RENDER_GROUP_H
#include "Raavanan.h"

struct render_basis
{
    v3 Pos;
};

struct entity_visible_piece
{
    render_basis* Basis;
	loaded_bitmap* Bitmap;
	v2 Offset;
	float OffsetZ;
	float EntityZCofficient;
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

inline void* PushRenderElement(render_group* Group, uint32 Size)
{
    void* Result = 0;
    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
        Result = (Group->PushBufferBase + Group->PushBufferSize);
        Group->PushBufferSize += Size;
    }
    else
    {
        InvalidCodePath;
    }
    return Result;
}

inline void PushPiece(render_group* Group, loaded_bitmap* Bitmap, v2 Offset, float OffsetZ, v2 Align, 
						v2 Dim, v4 Color, float EntityZCofficient)
{
	entity_visible_piece* Piece = (entity_visible_piece*)PushRenderElement(Group, sizeof(entity_visible_piece));
	Piece->Basis = Group->DefaultBasis;
	Piece->Bitmap = Bitmap;
	Piece->Offset = Group->MetersToPixels * V2(Offset.X, -Offset.Y) - Align;
	Piece->OffsetZ = OffsetZ;
	Piece->EntityZCofficient = EntityZCofficient;
	Piece->R = Color.R;
	Piece->G = Color.G;
	Piece->B = Color.B;
	Piece->A = Color.A;
	Piece->Dim = Dim;
}

inline void PushBitmap(render_group* Group, loaded_bitmap* Bitmap, v2 Offset, float OffsetZ, v2 Align, float Alpha = 1.0f, float EntityZCofficient = 1.0f)
{
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZCofficient);
}

inline void PushRect(render_group* Group, v2 Offset, float OffsetZ, v2 Dim, v4 Color, float EntityZCofficient = 1.0f)
{
	PushPiece(Group, 0, Offset, OffsetZ, V2(0,0), Dim, Color, EntityZCofficient);
}

inline void PushRectOutline(render_group* Group, v2 Offset, float OffsetZ, v2 Dim, v4 Color, float EntityZCofficient = 1.0f)
{
	float Thickness = 0.1f;
	PushPiece(Group, 0, Offset - V2(0, 0.5f * Dim.Y), OffsetZ, V2(0,0), V2(Dim.X, Thickness), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset + V2(0, 0.5f * Dim.Y), OffsetZ, V2(0,0), V2(Dim.X, Thickness), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset - V2(0.5f * Dim.X, 0), OffsetZ, V2(0,0), V2(Thickness, Dim.Y), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset + V2(0.5f * Dim.X, 0), OffsetZ, V2(0,0), V2(Thickness, Dim.Y), Color, EntityZCofficient);
}

#define RAAVANAN_RENDER_GROUP_H
#endif