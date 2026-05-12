#include "Raavanan_render_group.h"

internal void RenderRectangle(loaded_bitmap* Buffer, v2 vMin, v2 vMax, float R, float G, float B)
{
	int32 MinX = RoundFloatToInt32(vMin.X);
	int32 MinY = RoundFloatToInt32(vMin.Y);
	int32 MaxX = RoundFloatToInt32(vMax.X);
	int32 MaxY = RoundFloatToInt32(vMax.Y);
	
	MinX = (MinX < 0) ? 0 : MinX;
	MinY = (MinY < 0) ? 0 : MinY;
	MaxX = (MaxX > Buffer->Width) ? Buffer->Width : MaxX;
	MaxY = (MaxY > Buffer->Height) ? Buffer->Height : MaxY;

	uint32 Color = ((RoundFloatToUInt32(R * 255.0f) << 16 | RoundFloatToUInt32(G * 255.0f) << 8 | RoundFloatToUInt32(B * 255.0f)));
	uint8 *Row = ((uint8 *)Buffer->Memory + MinX * BITMAP_BYTES_PER_PIXEL + MinY * Buffer->Pitch);
	
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int x = MinX; x < MaxX; ++x)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void RenderRectOutline(loaded_bitmap* Buffer, v2 Min, v2 Max, v3 Color, float T = 2.0f)
{
	float Thickness = 0.1f;
	RenderRectangle(Buffer, V2(Min.X - T, Min.Y - T), V2(Max.X + T, Min.Y + T), Color.R, Color.G, Color.B);
	RenderRectangle(Buffer, V2(Min.X - T, Max.Y - T), V2(Max.X + T, Max.Y + T), Color.R, Color.G, Color.B);

	RenderRectangle(Buffer, V2(Min.X - T, Min.Y - T), V2(Min.X + T, Max.Y + T), Color.R, Color.G, Color.B);
	RenderRectangle(Buffer, V2(Max.X - T, Min.Y - T), V2(Max.X + T, Max.Y + T), Color.R, Color.G, Color.B);
}

internal void RenderBitMap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap, float realX, float realY, float CAlpha)
{
	int32 MinX = RoundFloatToInt32(realX);
	int32 MinY = RoundFloatToInt32(realY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;
	
	int32 SrcOffsetX = 0;
	if (MinX < 0)
	{
		SrcOffsetX = -MinX;
		MinX = 0;
	}
	int32 SrcOffsetY = 0;
	if (MinY < 0)
	{
		SrcOffsetY = -MinY;
		MinY = 0;
	}

	MaxX = (MaxX > Buffer->Width) ? Buffer->Width : MaxX;
	MaxY = (MaxY > Buffer->Height) ? Buffer->Height : MaxY;

	uint8 *SrcRow = (uint8 *)Bitmap->Memory + SrcOffsetY * Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL * SrcOffsetX; 
	uint8 *DstRow = ((uint8 *)Buffer->Memory + MinX * BITMAP_BYTES_PER_PIXEL + MinY * Buffer->Pitch);
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Dst = (uint32 *)DstRow;
		uint32 *Src = (uint32 *)SrcRow;
		for (int x = MinX; x < MaxX; ++x)
		{
			float SA = (float)((*Src >> 24) & 0xFF);
			float RSA = (SA / 255.0f) * CAlpha;
			float SR = CAlpha * (float)((*Src >> 16) & 0xFF);
			float SG = CAlpha * (float)((*Src >> 8) & 0xFF);
			float SB = CAlpha * (float)((*Src >> 0) & 0xFF);
			
			float DA = (float)((*Dst >> 24) & 0xFF);
			float DR = (float)((*Dst >> 16) & 0xFF);
			float DG = (float)((*Dst >> 8) & 0xFF);
			float DB = (float)((*Dst >> 0) & 0xFF);
			float RDA = (DA / 255.0f);

			float OneMinusRSA = (1.0f - RSA);
			float A = 255.0f * (RSA + RDA - RSA* RDA);
			float R = OneMinusRSA * DR + SR;
			float G = OneMinusRSA * DG + SG;
			float B = OneMinusRSA * DB + SB;

			*Dst = (((uint32)(A + 0.5f) << 24) |
					((uint32)(R + 0.5f) << 16) |
					((uint32)(G + 0.5f) << 8) |
					((uint32)(B + 0.5f) << 0));
			++Dst;
			++Src;
		}
		DstRow += Buffer->Pitch;
		SrcRow += Bitmap->Pitch;
	}
}

internal void
DrawMatte(loaded_bitmap *Buffer, loaded_bitmap *Bitmap, float RealX, float RealY, float CAlpha = 1.0f)
{
	int32 MinX = RoundFloatToInt32(RealX);
	int32 MinY = RoundFloatToInt32(RealY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;

    int32 SourceOffsetX = 0;
    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32 SourceOffsetY = 0;
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8 *SourceRow = (uint8 *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
    uint8 *DestRow = ((uint8 *)Buffer->Memory +
                      MinX*BITMAP_BYTES_PER_PIXEL +
                      MinY*Buffer->Pitch);
    for(int Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Dest = (uint32 *)DestRow;
        uint32 *Source = (uint32 *)SourceRow;
        for(int X = MinX;
            X < MaxX;
            ++X)
        {
            float SA = (float)((*Source >> 24) & 0xFF);
            float RSA = (SA / 255.0f) * CAlpha;            
            float SR = CAlpha*(float)((*Source >> 16) & 0xFF);
            float SG = CAlpha*(float)((*Source >> 8) & 0xFF);
            float SB = CAlpha*(float)((*Source >> 0) & 0xFF);

            float DA = (float)((*Dest >> 24) & 0xFF);
            float DR = (float)((*Dest >> 16) & 0xFF);
            float DG = (float)((*Dest >> 8) & 0xFF);
            float DB = (float)((*Dest >> 0) & 0xFF);
            float RDA = (DA / 255.0f);
            
            float InvRSA = (1.0f-RSA);
            // TODO(casey): Check this for math errors
//            float A = 255.0f*(RSA + RDA - RSA*RDA);
            float A = InvRSA*DA;
            float R = InvRSA*DR;
            float G = InvRSA*DG;
            float B = InvRSA*DB;

            *Dest = (((uint32)(A + 0.5f) << 24) |
                     ((uint32)(R + 0.5f) << 16) |
                     ((uint32)(G + 0.5f) << 8) |
                     ((uint32)(B + 0.5f) << 0));
            
            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}
inline v2 GetRenderEntityBasisPos(render_group* RenderGroup, render_entity_basis* EntityBasis, v2 ScreenCenter)
{
    v3 EntityBaseP = EntityBasis->Basis->Pos;
    float ZFudge = (1.0f + 0.1f * (EntityBaseP.Z + EntityBasis->OffsetZ));
    float EntityGroundPointX = ScreenCenter.X + RenderGroup->MetersToPixels * ZFudge * EntityBaseP.X;
    float EntityGroundPointY = ScreenCenter.Y - RenderGroup->MetersToPixels * ZFudge * EntityBaseP.Y;
    float EntityZ = -RenderGroup->MetersToPixels * EntityBaseP.Z;
    
    v2 Center = v2{EntityGroundPointX + EntityBasis->Offset.X,
                            EntityGroundPointY + EntityBasis->Offset.Y + EntityBasis->EntityZCofficient * EntityZ};

    return Center;
}

internal void RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget)
{
    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
	{
        v2 ScreenCenter = V2(0.5f * (float)OutputTarget->Width, 0.5f * (float)OutputTarget->Height);
		render_group_entry_header* Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);
		
        switch (Header->Type)
        {
        case RenderGroupEntryType_render_entry_clear:
        {
            render_entry_clear* Entry = (render_entry_clear *)Header;
            BaseAddress += sizeof(*Entry);
        }
        break;
        case RenderGroupEntryType_render_entry_bitmap:
        {
            render_entry_bitmap* Entry = (render_entry_bitmap *)Header;
            v2 Pos = GetRenderEntityBasisPos(RenderGroup, &Entry->EntityBasis, ScreenCenter);
            
            Assert(Entry->Bitmap);
            RenderBitMap(OutputTarget, Entry->Bitmap, Pos.X, Pos.Y, Entry->A);
            BaseAddress += sizeof(*Entry);
        }
        break;
        case RenderGroupEntryType_render_entry_rectangle:
        {
            render_entry_rectangle* Entry = (render_entry_rectangle *)Header;
            v2 Pos = GetRenderEntityBasisPos(RenderGroup, &Entry->EntityBasis, ScreenCenter);
            RenderRectangle(OutputTarget, Pos, Pos, Entry->R, Entry->G, Entry->B);
            BaseAddress += sizeof(*Entry);
        }
        break;
        InvalildDefaultCase;
        }
	}
}

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

#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)
inline render_group_entry_header* PushRenderElement_(render_group* Group, uint32 Size, render_group_entry_type Type)
{
    render_group_entry_header* Result = 0;
    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
        Result = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
        Result->Type = Type;
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
	render_entry_bitmap* Piece = PushRenderElement(Group, render_entry_bitmap);
    if(Piece)
    {
        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->Bitmap = Bitmap;
        Piece->EntityBasis.Offset = Group->MetersToPixels * V2(Offset.X, -Offset.Y) - Align;
        Piece->EntityBasis.OffsetZ = OffsetZ;
        Piece->EntityBasis.EntityZCofficient = EntityZCofficient;
        Piece->R = Color.R;
        Piece->G = Color.G;
        Piece->B = Color.B;
        Piece->A = Color.A;
    }
}

inline void PushBitmap(render_group* Group, loaded_bitmap* Bitmap, v2 Offset, float OffsetZ, v2 Align, float Alpha = 1.0f, float EntityZCofficient = 1.0f)
{
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZCofficient);
}

inline void PushRect(render_group* Group, v2 Offset, float OffsetZ, v2 Dim, v4 Color, float EntityZCofficient = 1.0f)
{
    render_entry_rectangle* Piece = PushRenderElement(Group, render_entry_rectangle);
    if(Piece)
    {
        v2 HalfDim = 0.5f * Group->MetersToPixels * Dim;
        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->EntityBasis.Offset = Group->MetersToPixels * V2(Offset.X, -Offset.Y) - HalfDim;
        Piece->EntityBasis.OffsetZ = OffsetZ;
        Piece->EntityBasis.EntityZCofficient = EntityZCofficient;
        Piece->R = Color.R;
        Piece->G = Color.G;
        Piece->B = Color.B;
        Piece->A = Color.A;
        Piece->Dim = Group->MetersToPixels * Dim;
    }
}

inline void PushRectOutline(render_group* Group, v2 Offset, float OffsetZ, v2 Dim, v4 Color, float EntityZCofficient = 1.0f)
{
	float Thickness = 0.1f;
	PushPiece(Group, 0, Offset - V2(0, 0.5f * Dim.Y), OffsetZ, V2(0,0), V2(Dim.X, Thickness), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset + V2(0, 0.5f * Dim.Y), OffsetZ, V2(0,0), V2(Dim.X, Thickness), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset - V2(0.5f * Dim.X, 0), OffsetZ, V2(0,0), V2(Thickness, Dim.Y), Color, EntityZCofficient);
	PushPiece(Group, 0, Offset + V2(0.5f * Dim.X, 0), OffsetZ, V2(0,0), V2(Thickness, Dim.Y), Color, EntityZCofficient);
}
