#include "Raavanan_intrinsics.h"
#include "Raavanan_random.h"
#include "Raavanan_tile.cpp"
#include <stdio.h>
#include <windows.h>
static void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer, int ToneHz)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	int16 *SampleOut = SoundBuffer->Samples;
	
	for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
#if 0
		float SineVal = sinf(GameState->tSine);
		int16 Samplevalue = (int16)(SineVal * ToneVolume);
#else
		int16 Samplevalue = 0;
#endif
		*SampleOut++ = Samplevalue;
		*SampleOut++ = Samplevalue;
#if 0
		GameState->tSine += (float)(2.0f *  PI * (float)1.0f/(float)WavePeriod);
		if(GameState->tSine > (2.0f *  PI))
		{
			GameState->tSine -= (float)(2.0f *  PI);
		}
#endif
	}
}

static void RenderGradiant(game_offscreen_buffer *Buffer, int xOffset, int yOffset)
{	
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int y = 0; y < Buffer->Height; ++y) {
		uint32 *pixel = (uint32 *)Row;
		/*
		RGBA to be added in reverse order
		As it's been following the little endian system
		In Memory 	BB GG RR xx 0x
		In Register	0x RR GG BB AA
		*/
		for(int x = 0; x < Buffer->Width; ++x) 
		{
			uint8 Red = (uint8)(x + xOffset);
			uint8 Green = (uint8)(y+ yOffset);
			*pixel++ = ((Green << 16) | (Red));
		}
		Row += Buffer->Pitch;
	}
}

static void RenderRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, float R, float G, float B)
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
	uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	
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

static void RenderBitMap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, float realX, float realY, int32 AlignX = 0, int32 AlignY = 0, float CAlpha = 1.0f)
{
	realX -= AlignX;
	realY -= AlignY;
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

	uint32 *SrcRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	SrcRow += -SrcOffsetY * Bitmap->Width + SrcOffsetX; 
	uint8 *DstRow = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Dst = (uint32 *)DstRow;
		uint32 *Src = SrcRow;
		for (int x = MinX; x < MaxX; ++x)
		{
			float A = (float)((*Src >> 24) & 0xFF) / 255.0f;
			A *= CAlpha;
			float SR = (float)((*Src >> 16) & 0xFF);
			float SG = (float)((*Src >> 8) & 0xFF);
			float SB = (float)((*Src >> 0) & 0xFF);

			float DR = (float)((*Dst >> 16) & 0xFF);
			float DG = (float)((*Dst >> 8) & 0xFF);
			float DB = (float)((*Dst >> 0) & 0xFF);

			float R = (1 - A) * DR + A * SR;
			float G = (1 - A) * DG + A * SG;
			float B = (1 - A) * DB + A * SB;

			*Dst = (((uint32)(R + 0.5f) << 16) |
					((uint32)(G + 0.5f) << 8) |
					((uint32)(B + 0.5f) << 0));
			++Dst;
			++Src;
		}
		DstRow += Buffer->Pitch;
		SrcRow -= Bitmap->Width;
	}
}
//#if RAAVANAN_INTERNAL
#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

static loaded_bitmap DEBUGLoadBMP (thread_context *Thread, debug_read_entire_file *ReadEntireFile, char *FileName)
{
	loaded_bitmap Result = {};
	debug_read_file_result FileResult = ReadEntireFile (Thread, FileName);
	if(FileResult.ContentSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)FileResult.Content;
		uint32 *Pixels = (uint32 *)((uint8 *)FileResult.Content + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		Assert(Header->Compression == 3);
		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
#if 1
		bit_scan_result RedShift = FindLSBSetBit(RedMask);
		bit_scan_result GreenShift = FindLSBSetBit(GreenMask);
		bit_scan_result BlueShift = FindLSBSetBit(BlueMask);
		bit_scan_result AlphaShift = FindLSBSetBit(AlphaMask);

		Assert(RedShift.Found);
		Assert(GreenShift.Found);
		Assert(BlueShift.Found);
		Assert(AlphaShift.Found);
		
		uint32 *SrcDst = Pixels;
		for (int32 Y = 0; Y < Header->Height; ++Y)
		{
			for (int32 X = 0; X < Header->Width; ++X)
			{
				uint32 C = *SrcDst;
				*SrcDst++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
							 (((C >> RedShift.Index) & 0xFF) << 16) |
							 (((C >> GreenShift.Index) & 0xFF) << 8) |
							 (((C >> BlueShift.Index) & 0xFF) << 0));
				// uint8 C0 = SrcDst[0];	// Alpha
				// uint8 C1 = SrcDst[1];	// Blue	
				// uint8 C2 = SrcDst[2];	// Green	
				// uint8 C3 = SrcDst[3];	// Red	
				// *(uint32 *)SrcDst = (C0 << 24) | (C3 << 16) | (C2 << 8) | (C1 << 0);
				// SrcDst +=
			}
		}
#else
		bit_scan_result RedScan = FindLSBSetBit(RedMask);
		bit_scan_result GreenScan = FindLSBSetBit(GreenMask);
		bit_scan_result BlueScan = FindLSBSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLSBSetBit(AlphaMask);

		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);
		
		int32 RedShift = 16 - (int32)RedScan.Index;
		int32 GreenShift = 8 - (int32)GreenScan.Index;
		int32 BlueShift = 0 - (int32)BlueScan.Index;
		int32 AlphaShift = 16 - (int32)AlphaScan.Index;
		
		uint32 *SrcDst = Pixels;
		for (int32 Y = 0; Y < Header->Height; ++Y)
		{
			for (int32 X = 0; X < Header->Width; ++X)
			{
				uint32 C = *SrcDst;
				*SrcDst++ = (RotateLeft(C & RedMask, RedShift) |
							RotateLeft(C & GreenMask, GreenShift) |
							RotateLeft(C & BlueMask, BlueShift) |
							RotateLeft(C & AlphaMask, AlphaShift));
				// uint8 C0 = SrcDst[0];	// Alpha
				// uint8 C1 = SrcDst[1];	// Blue	
				// uint8 C2 = SrcDst[2];	// Green	
				// uint8 C3 = SrcDst[3];	// Red	
				// *(uint32 *)SrcDst = (C0 << 24) | (C3 << 16) | (C2 << 8) | (C1 << 0);
				// SrcDst +=
			}
		}
#endif
	}
	return Result;
}
//#endif
static void InitializeArena (memory_arena *MemoryArena, size_t Size, uint8 *BasePtr)
{
	MemoryArena->Size = Size;
	MemoryArena->Base = BasePtr;
	MemoryArena->UsedSpace = 0;
}

static high_entity* MakeEntityHighFrequency (game_state* GameState, uint32 LowIndex)
{
	high_entity* EntityHigh = 0;
	low_entity* EntityLow = &GameState->LowEntities[LowIndex];
	if (!EntityLow->HighEntityIndex)
	{
		if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities))
		{
			uint32 HighIndex = GameState->HighEntityCount++;
			EntityHigh = GameState->HighEntities + HighIndex;
			tile_map_difference Diff = Subtract(GameState->World->TileMap, &EntityLow->Pos, &GameState->CameraP);
			EntityHigh->Pos = Diff.dXY;
			EntityHigh->dPlayerP = v2 {0,0};
			EntityHigh->AbsTileZ = EntityLow->Pos.AbsTileZ;
			EntityHigh->FacingDirection = 0;
			EntityHigh->LowEntityIndex = LowIndex;

			EntityLow->HighEntityIndex = HighIndex;
		}
		else
		{
			InvalidCodePath;
		}
	}
	else
	{
		EntityHigh = GameState->HighEntities + EntityLow->HighEntityIndex;
	}
	return EntityHigh;
}

inline entity GetHighEntity(game_state* GameState, uint32 Index)
{
	entity Result = {};
	if ((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result.LowIndex = Index;
		Result.Low = GameState->LowEntities + Index;
		Result.High = MakeEntityHighFrequency (GameState, Index);
	}
	return Result;
}

static void MakeEntityLowFrequency (game_state* GameState, uint32 LowIndex)
{
	low_entity* EntityLow = &GameState->LowEntities[LowIndex];
	uint32 HighIndex = EntityLow->HighEntityIndex;
	if (HighIndex)
	{
		uint32 LastHighIndex = GameState->HighEntityCount - 1;
		if (HighIndex != LastHighIndex)
		{
			high_entity* LastEntity = GameState->HighEntities + LastHighIndex;
			high_entity* DelEntity = GameState->HighEntities + HighIndex;
			*DelEntity = *LastEntity;
			GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
		}
		--GameState->HighEntityCount;
		EntityLow->HighEntityIndex = 0;
	}
}

inline low_entity* GetLowEntity(game_state* GameState, uint32 Index)
{
	low_entity* Result = 0;
	if ((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result = GameState->LowEntities + Index;
	}
	return Result;
}

static void OffsetAndCheckFrequencyByArea(game_state* GameState, v2 Offset, rectangle2 CameraBounds)
{
	for (uint32 EntityIndex = 1; EntityIndex < GameState->HighEntityCount; )
	{
		high_entity* High = GameState->HighEntities + EntityIndex;
		High->Pos += Offset;
		if(IsInRectangle(CameraBounds, High->Pos))
		{
			EntityIndex++;
		}
		else
		{
			MakeEntityLowFrequency(GameState, EntityIndex);
		}
	}
}

static uint32 AddLowEntity(game_state* GameState, entity_type Type)
{
	Assert (GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32 EntityIndex = GameState->LowEntityCount++;
	
	GameState->LowEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex].Type = Type;
	return EntityIndex;
}

static uint32 AddPlayer (game_state* GameState)
{
	uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero);
	low_entity* EntityLow = GetLowEntity(GameState, EntityIndex);
	EntityLow->Pos = GameState->CameraP;	
	EntityLow->Pos.Offset.X = 0.0f;
	EntityLow->Pos.Offset.Y = 0.0f;
	EntityLow->Height = 0.5f;
	EntityLow->Width = 1.0f;
	EntityLow->Collides = true;

	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex = EntityIndex;
	}
	return EntityIndex;
}

static uint32 AddWall (game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	uint32 EntityIndex = AddLowEntity (GameState, EntityType_Wall);
	low_entity* EntityLow = GetLowEntity(GameState, EntityIndex);
	EntityLow->Pos.AbsTileX = AbsTileX;
	EntityLow->Pos.AbsTileY = AbsTileY;
	EntityLow->Pos.AbsTileZ = AbsTileZ;
	EntityLow->Height = GameState->World->TileMap->TileSideInMeters;
	EntityLow->Width = EntityLow->Height;
	EntityLow->Collides = true;
	
	return EntityIndex;
}

static bool TestWall (float WallX, float RelX, float RelY, float PlayerDeltaX, float PlayerDeltaY,
						float *tMin, float MinY, float MaxY)
{
	bool Hit = false;
	float tEpsilon = 0.0001f;
	if(PlayerDeltaX != 0.0f)
	{
		float tResult = (WallX - RelX) / PlayerDeltaX;
		float Y = RelY + tResult * PlayerDeltaY;
		if((tResult >= 0.0f) && (*tMin > tResult))
		{	
			if((Y >= MinY) && (Y <= MaxY))
			{
				*tMin = Maximum(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}
	return Hit;
}
static void MovePlayer (game_state* GameState, entity Entity, float deltaTime, v2 ddPlayer)
{
	tile_map* TileMap = GameState->World->TileMap;

	float ddPLength = LengthSq(ddPlayer);
	if(ddPLength > 1.0f)
	{
		ddPlayer *= (1.0f / SquareRoot(ddPLength));
	}

	float PlayerSpeed = 50.0f;
	ddPlayer *= PlayerSpeed;

	ddPlayer += -8.0f * Entity.High->dPlayerP;

	v2 OldPlayerP = Entity.High->Pos;
	v2 PlayerDelta = 0.5f * ddPlayer * Square(deltaTime) + Entity.High->dPlayerP * deltaTime;
	Entity.High->dPlayerP = ddPlayer * deltaTime + Entity.High->dPlayerP;
	v2 NewPlayerP = OldPlayerP + PlayerDelta;
	
	// uint32 MintileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	// uint32 MintileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
	// uint32 MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	// uint32 MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

	// int32 EntityTileWidth = CeilFloatToInt32(Entity->Width / TileMap->TileSideInMeters);
	// int32 EntityTileHeight = CeilFloatToInt32(Entity->Height / TileMap->TileSideInMeters);

	// MintileX -= EntityTileWidth;
	// MintileY -= EntityTileHeight;
	// MaxTileX += EntityTileWidth;
	// MaxTileY += EntityTileHeight;

	// uint32 AbsTileZ = Entity.High->Pos.AbsTileZ;	

	for (uint32 Iteration = 0; (Iteration < 4); ++Iteration)
	{
		float tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitHighEntityIndex = 0;
		v2 DesiredPos = Entity.High->Pos + PlayerDelta;

		for (uint32 TestHighEntityIndex = 1; TestHighEntityIndex < GameState->HighEntityCount; ++TestHighEntityIndex)
		{
			if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
			{
				entity TestEntity;
				TestEntity.High = GameState->HighEntities + TestHighEntityIndex;
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;

				if(TestEntity.Low->Collides)
				{
					float DiameterW = TestEntity.Low->Width + Entity.Low->Width;
					float DiameterH = TestEntity.Low->Height + Entity.Low->Height;
					v2 MinCorner = -0.5f * v2 {DiameterW, DiameterH};
					v2 MaxCorner = 0.5f * v2 {DiameterW, DiameterH};

					v2 Rel = Entity.High->Pos - TestEntity.High->Pos;
					
					if(TestWall (MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2 {-1,0};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2 {1,0};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2 {0,-1};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2 {0,1};
						HitHighEntityIndex = TestHighEntityIndex;
					}
				}
			}			
		}
		Entity.High->Pos += tMin * PlayerDelta;
		if(HitHighEntityIndex)
		{
			Entity.High->dPlayerP = Entity.High->dPlayerP - 1 * Dot(Entity.High->dPlayerP, WallNormal) * WallNormal;
			PlayerDelta = DesiredPos - Entity.High->Pos;
			PlayerDelta = PlayerDelta - 1 * Dot(PlayerDelta, WallNormal) * WallNormal;

			high_entity* HitHigh = GameState->HighEntities + HitHighEntityIndex;
			low_entity* HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
			Entity.High->AbsTileZ += HitLow->dAbsTileZ;
		}
		else
		{
			break;
		}		
	}


	if((Entity.High->dPlayerP.X == 0.0f) && (Entity.High->dPlayerP.Y == 0.0f))
	{

	}
	else if(AbsoluteValue(Entity.High->dPlayerP.X) > AbsoluteValue(Entity.High->dPlayerP.Y))
	{
		if(Entity.High->dPlayerP.X > 0)
		{
			Entity.High->FacingDirection = 0;
		}
		else
		{
			Entity.High->FacingDirection = 2;
		}
	}
	else
	{
		if(Entity.High->dPlayerP.Y > 0)
		{
			Entity.High->FacingDirection = 1;
		}
		else
		{
			Entity.High->FacingDirection = 3;
		}
	}
	Entity.Low->Pos = MapIntoTileSpace (GameState->World->TileMap, GameState->CameraP, Entity.High->Pos);
}

static void SetCamera (game_state* GameState, tile_map_position NewCameraP)
{
	tile_map* TileMap = GameState->World->TileMap;
	tile_map_difference dCameraP = Subtract (TileMap, &NewCameraP, &GameState->CameraP);
	GameState->CameraP = NewCameraP;
	uint32 TileSpanX = 17 * 3;
	uint32 TileSpanY = 9 * 3;
	rectangle2 CameraBounds = RectCenterDim(v2{0,0}, TileMap->TileSideInMeters * v2 {(float)TileSpanX, (float)TileSpanY});
	v2 EntityOffsetForFrame = -dCameraP.dXY;
	OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

	int32 MinTileX = NewCameraP.AbsTileX - TileSpanX/2;
	int32 MaxTileX = NewCameraP.AbsTileX + TileSpanX/2;
	int32 MinTileY = NewCameraP.AbsTileY - TileSpanY/2;
	int32 MaxTileY = NewCameraP.AbsTileY + TileSpanY/2;
	for (uint32 EntityIndex = 1; EntityIndex < GameState->LowEntityCount; ++EntityIndex)
	{
		low_entity* Low = GameState->LowEntities + EntityIndex;			
		if (Low->HighEntityIndex == 0)
		{
			if((Low->Pos.AbsTileZ == NewCameraP.AbsTileZ) &&
				(Low->Pos.AbsTileX >= MinTileX) &&
				(Low->Pos.AbsTileX <= MaxTileX) &&
				(Low->Pos.AbsTileY >= MinTileY) &&
				(Low->Pos.AbsTileY <= MinTileY))
			{
				MakeEntityHighFrequency (GameState, EntityIndex);
			}
		}
	}

}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		// Null Entity,
		AddLowEntity (GameState, EntityType_None);
		GameState->HighEntityCount = 1;
		GameState->Backdrop = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_background.bmp");
		GameState->Shadow = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_hero_shadow.bmp");
		hero_bitmaps *Bitmap = GameState->HeroBitmaps;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		

		InitializeArena (&GameState->WorldArena, (size_t)Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
		tile_map *TileMap = World->TileMap;
		
		InitializeTileMap(TileMap, 1.4f);
		
		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenBaseX = 0;//(INT32_MAX/TilesPerWidth)/2;
		uint32 ScreenBaseY = 0;//(INT32_MAX/TilesPerHeight)/2;
		uint32 ScreenBaseZ = 0;//INT32_MAX/2;
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;

		bool DoorLeft = false;
		bool DoorRight = false;
		bool DoorTop = false;
		bool DoorBottom = false;
		bool DoorUp = false;
		bool DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 2; ++ScreenIndex)
		{
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice = (RandomNumberTable[RandomNumberIndex++] % 2/*((DoorUp || DoorDown) ? 2 : 3)*/);
			bool CreatedZDoor = false;
			if(RandomChoice == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == ScreenBaseZ)	
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
				
			}
			else if(RandomChoice == 1)
			{
				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}
			
			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;					
					uint32 TileValue = 1;
					if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}
					if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}
					if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileX == 10) && (TileY == 6))
					{
						if(DoorUp)
						{
							TileValue = 3;
						}
						if(DoorDown)
						{
							TileValue = 4;
						}
					}
					SetTileValue (&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
					if (TileValue == 2)
					{
						AddWall (GameState, AbsTileX, AbsTileY, AbsTileZ);
					}
				}
			}
			DoorLeft = DoorRight;
			DoorBottom = DoorTop;
			if(CreatedZDoor)
			{
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}
			
			DoorRight = false;
			DoorTop = false;
			
			if(RandomChoice == 2)
			{
				//AbsTileZ = 1 - AbsTileZ;	// AbsTileZ is 0 it goes to 1 otherwise, vice versa
				if (AbsTileZ == ScreenBaseZ)
				{
					AbsTileZ = ScreenBaseZ + 1;
				}
				else
				{
					AbsTileZ = ScreenBaseZ;
				}
			}
			else if(RandomChoice == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
			
		}
// #if RAAVANAN_INTERNAL
		char *Filename = __FILE__;
		debug_read_file_result File = Memory->DEBUGReadEntireFile(Thread, Filename);
		if(File.Content)
		{
			Memory->DEBUGWriteEntireFile(Thread, "test.out", File.ContentSize, File.Content);
			Memory->DEBUGFreeFileMemory(Thread, File.Content);
		}
#if RAAVANAN_INTERNAL
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->PlayerX = GameState->PlayerY = 100;
#endif
	
		tile_map_position NewCameraP = {};
		NewCameraP.AbsTileX = ScreenBaseX * TilesPerWidth + 17/2;
		NewCameraP.AbsTileY = ScreenBaseY * TilesPerHeight + 9/2;
		NewCameraP.AbsTileZ = ScreenBaseZ;
		SetCamera(GameState, NewCameraP);
		
		Memory->IsInitialized = true;
	}
	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;
	// Tile width and Height
	int32 TileSideInPixels = 60;
	float MeterToPixels = ((float)TileSideInPixels / (float)TileMap->TileSideInMeters);		
	
	float LowerLeftX = (float)-TileSideInPixels/2;
	float LowerLeftY = (float)Buffer->Height;

	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		uint32 LowIndex = GameState->PlayerControllerIndex[ControllerIndex];
		if (LowIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddPlayer(GameState);
				GameState->PlayerControllerIndex[ControllerIndex] = EntityIndex;
			}
		}
		else
		{
			entity ControllingEntity = GetHighEntity (GameState, LowIndex);
			v2 ddPlayerP = {};
			if(Controller->IsAnalog)
			{
#if RAAVANAN_INTERNAL
				GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
				GameState->YOffset += (int)(4.0f * (Controller->StickAverageX));
#endif
				ddPlayerP = v2 {Controller->StickAverageX, Controller->StickAverageY};
			}
			else
			{
				if(Controller->MoveUp.EndedDown)
				{
					ddPlayerP.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ddPlayerP.Y = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ddPlayerP.X = 1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ddPlayerP.X = -1.0f;
				}
			}
			
			if (Controller->ActionUp.EndedDown)
			{
				ControllingEntity.High->dZ = 3.0f;
			}
			MovePlayer(GameState, ControllingEntity, Input->deltaTime, ddPlayerP);
		}
	}

	v2 EntityOffsetForFrame = {};
	entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity.High)
	{
		tile_map_position NewCameraP = GameState->CameraP;
		NewCameraP.AbsTileZ = CameraFollowingEntity.Low->Pos.AbsTileZ;
#if 1
		if (CameraFollowingEntity.High->Pos.X > (9.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX += 17;
		}
		if (CameraFollowingEntity.High->Pos.X < -(9.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX -= 17;
		}
		if (CameraFollowingEntity.High->Pos.Y > (5.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY += 9;
		}
		if (CameraFollowingEntity.High->Pos.Y < -(5.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY -= 9;
		}
#else
		if (CameraFollowingEntity.High->Pos.X > (1.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX += 1;
		}
		if (CameraFollowingEntity.High->Pos.X < -(1.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX -= 1;
		}
		if (CameraFollowingEntity.High->Pos.Y > (1.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY += 1;
		}
		if (CameraFollowingEntity.High->Pos.Y < -(1.0f * TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY -= 1;
		}
#endif
		SetCamera(GameState, NewCameraP);
	}	

	RenderBitMap (Buffer, &GameState->Backdrop, 0, 0);

	float ScreenCenterX = 0.5f * (float)Buffer->Width;
	float ScreenCenterY = 0.5f * (float)Buffer->Height;
#if 0
	for (int32 Relrow = -10; Relrow < 10; ++Relrow)
	{
		for(int32 Relcol = - 20; Relcol < 20; ++Relcol)
		{
			uint32 Column = GameState->CameraP.AbsTileX + Relcol;
			uint32 Row = GameState->CameraP.AbsTileY + Relrow;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
			if(TileID > 1)
			{
				float ColorVal = ((TileID == 2) ? 1.0f : ((TileID > 2) ? 0.25f : 0.5f));
				ColorVal = ((Column == GameState->CameraP.AbsTileX) && (Row == GameState->CameraP.AbsTileY)) ? 0.0f : ColorVal;

				v2 TileSide = {0.5f * TileSideInPixels, 0.5f * TileSideInPixels};
				v2 Center = {ScreenCenterX - MeterToPixels * GameState->CameraP.Offset.X + ((float)Relcol) * TileSideInPixels,
								ScreenCenterY + MeterToPixels * GameState->CameraP.Offset.Y - ((float)Relrow) * TileSideInPixels};
				v2 Min = Center - TileSide;
				v2 Max = Center + TileSide;
				RenderRectangle(Buffer, Min, Max, ColorVal, ColorVal, ColorVal);
			}			
		}
	}
#endif
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	
	for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
	{
		high_entity* HighEntity = GameState->HighEntities + HighEntityIndex;
		low_entity* LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

		HighEntity->Pos += EntityOffsetForFrame;
		
		float dt = Input->deltaTime;
		float ddZ = -9.8f;
		HighEntity->Z += (0.5f * ddZ * Square(dt) + HighEntity->dZ * dt);
		HighEntity->dZ = ddZ * dt + HighEntity->dZ;
		if (HighEntity->Z < 0)
		{
			HighEntity->Z = 0;
		}
		float CAlpha = 1.0f - 0.5f * HighEntity->Z;
		CAlpha = (CAlpha < 0.0f) ? 0.0f : CAlpha;
		float PlayerR = 1.0f;
		float PlayerG = 1.0f;
		float PlayerB = 0.7f;
		float PlayerGroundPointX = ScreenCenterX + MeterToPixels * HighEntity->Pos.X;
		float PlayerGroundPointY = ScreenCenterY - MeterToPixels * HighEntity->Pos.Y;
		float Z = -MeterToPixels * HighEntity->Z;
		v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MeterToPixels * LowEntity->Width, PlayerGroundPointY - 0.5f * MeterToPixels * LowEntity->Height};
		v2 EntiryWidthHeight = {LowEntity->Width, LowEntity->Height};
		if (LowEntity->Type == EntityType_Hero)
		{
			hero_bitmaps *HeroBitsmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];
			RenderBitMap(Buffer, &GameState->Shadow, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY, CAlpha);
			RenderBitMap(Buffer, &HeroBitsmaps->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
			RenderBitMap(Buffer, &HeroBitsmaps->Cape, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
			RenderBitMap(Buffer, &HeroBitsmaps->Head, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
		}
		else
		{
			RenderRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MeterToPixels * EntiryWidthHeight, PlayerR, PlayerG, PlayerB);	
		}
	}
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
