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

static void RenderBitMap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, float realX, float realY, int32 AlignX = 0, int32 AlignY = 0)
{
	realX -= AlignX;
	realY -= AlignY;
	int32 MinX = RoundFloatToInt32(realX);
	int32 MinY = RoundFloatToInt32(realY);
	int32 MaxX = RoundFloatToInt32(realX + Bitmap->Width);
	int32 MaxY = RoundFloatToInt32(realY + Bitmap->Height);
	
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

inline entity* GetEntity(game_state* GameState, uint32 Index)
{
	entity* Entity = 0;
	if ((Index > 0) && (Index < ArrayCount(GameState->Entities)))
	{
		Entity = &GameState->Entities[Index];
	}
	return Entity;
}

static uint32 AddEntity(game_state* GameState)
{
	uint32 EntityIndex = GameState->EntityCount++;
	Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
	entity* Entity = &GameState->Entities[EntityIndex];
	*Entity = {};
	return EntityIndex;
}
static void InitializePlayer (game_state* GameState, uint32 EntityIndex)
{
	entity *Entity = GetEntity(GameState, EntityIndex);
	Entity->Exists = true;
	Entity->Pos.AbsTileX = 3;
	Entity->Pos.AbsTileY = 2;
	Entity->Pos.Offset.X = 5.0f;
	Entity->Pos.Offset.Y = 5.0f;
	Entity->Height = 1.4f;
	Entity->Width = 0.75f * Entity->Height;
	if(!GetEntity(GameState, GameState->CameraFollowingEntityIndex))
	{
		GameState->CameraFollowingEntityIndex = EntityIndex;
	}
}

static void MovePlayer (game_state* GameState, entity* Entity, float deltaTime, v2 ddPlayer)
{
	tile_map* TileMap = GameState->World->TileMap;

	float ddPLength = LengthSq(ddPlayer);
	if(ddPLength > 1.0f)
	{
		ddPlayer *= (1.0f / SquareRoot(ddPLength));
	}

	float PlayerSpeed = 50.0f;
	ddPlayer *= PlayerSpeed;

	ddPlayer += -8.0f * Entity->dPlayerP;

	tile_map_position OldPlayerP = Entity->Pos;
	tile_map_position NewPlayerP = OldPlayerP;

	//Position =  1/2 * acceleration * square(time) + velocity * time + position
	v2 PlayerDelta = 0.5f * ddPlayer * Square(deltaTime) + Entity->dPlayerP * deltaTime;
	NewPlayerP.Offset += PlayerDelta;
	Entity->dPlayerP = ddPlayer * deltaTime + Entity->dPlayerP;	
	NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

#if 1
	tile_map_position PlayerLeft = NewPlayerP;
	PlayerLeft.Offset.X -= 0.5f * Entity->Width;
	PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

	tile_map_position PlayerRight = NewPlayerP;
	PlayerRight.Offset.X += 0.5f * Entity->Width;
	PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);
	
	bool IsCollided = false;
	tile_map_position CollisionPt = {};
	if(!IsValidTileMapPoint(TileMap, NewPlayerP))
	{
		CollisionPt = NewPlayerP;
		IsCollided = true;
	}
	if(!IsValidTileMapPoint(TileMap, PlayerLeft))
	{
		CollisionPt = PlayerLeft;
		IsCollided = true;
	}
	if(!IsValidTileMapPoint(TileMap, PlayerRight))
	{
		CollisionPt = PlayerRight;
		IsCollided = true;
	}
	if(IsCollided)
	{
		v2 r = {0, 0};
		if(CollisionPt.AbsTileX < Entity->Pos.AbsTileX)
		{
			r = {1, 0};
		}
		if(CollisionPt.AbsTileX > Entity->Pos.AbsTileX)
		{
			r = {-1, 0};
		}
		if(CollisionPt.AbsTileY < Entity->Pos.AbsTileY)
		{
			r = {0, 1};
		}
		if(CollisionPt.AbsTileY > Entity->Pos.AbsTileY)
		{
			r = {0, -1};
		}
		Entity->dPlayerP = Entity->dPlayerP - 1 * Dot(Entity->dPlayerP, r) * r;				
	}
	else
	{
		Entity->Pos = NewPlayerP;
	}			
#else
	uint32 MintileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MintileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
	uint32 OnePastMaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX) + 1;
	uint32 OnePastMaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY) + 1;
	uint32 AbsTileZ = Entity->Pos.AbsTileZ;
	float tMin = 1.0f;
	for (uint32 AbsTileY = MintileY; AbsTileY != OnePastMaxTileY; ++AbsTileY)
	{
		for (uint32 AbsTileX = MintileX; AbsTileX != OnePastMaxTileX; ++AbsTileX)
		{
			tile_map_position TestTileP = CenteredTilePoint (AbsTileX, AbsTileY, AbsTileZ);
			uint32 TileValue = GetTileValue(TileMap, TestTileP);
			if(IsTileValueEmpty(TileValue))
			{
				v2 MinCorner = -0.5f * v2 {TileMap->TileSideInMeters, TileMap->TileSideInMeters};
				v2 MaxCorner = 0.5f * v2 {TileMap->TileSideInMeters, TileMap->TileSideInMeters};

				tile_map_difference RelNewPlayerP = Subtract(TileMap, &TestTileP, &NewPlayerP);
				
			}
		}
	}
#endif

	if(!AreOnSameTile(&OldPlayerP, &Entity->Pos))
	{
		uint32 NewTileValue = GetTileValue(TileMap, Entity->Pos);
		if(NewTileValue == 3)
		{
			++Entity->Pos.AbsTileZ;
		}
		else if (NewTileValue == 4)
		{
			--Entity->Pos.AbsTileZ;
		}
	}
	
	if((Entity->dPlayerP.X == 0.0f) && (Entity->dPlayerP.Y == 0.0f))
	{

	}
	else if(AbsoluteValue(Entity->dPlayerP.X) > AbsoluteValue(Entity->dPlayerP.Y))
	{
		if(Entity->dPlayerP.X > 0)
		{
			Entity->FacingDirection = 0;
		}
		else
		{
			Entity->FacingDirection = 2;
		}
	}
	else
	{
		if(Entity->dPlayerP.Y > 0)
		{
			Entity->FacingDirection = 1;
		}
		else
		{
			Entity->FacingDirection = 3;
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
		AddEntity (GameState);
		GameState->Backdrop = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_background.bmp");
		
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
		
		GameState->CameraP.AbsTileX = 17/2;
		GameState->CameraP.AbsTileY = 9/2;
		
		InitializeArena (&GameState->WorldArena, (size_t)Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
		tile_map *TileMap = World->TileMap;
		
		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;	// which is equal to assigning 0xFF
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);	// Which is equal to 256

		TileMap->TileChunkXCount = 128;
		TileMap->TileChunkYCount = 128;
		TileMap->TileChunkZCount = 2;
		TileMap->TileChunks = PushArray(&GameState->WorldArena, 
					TileMap->TileChunkXCount * TileMap->TileChunkYCount * TileMap->TileChunkZCount, tile_chunk);
		
		TileMap->TileSideInMeters = 1.4f;		
		
		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenY = 0;
		uint32 ScreenX = 0;

		bool DoorLeft = false;
		bool DoorRight = false;
		bool DoorTop = false;
		bool DoorBottom = false;
		bool DoorUp = false;
		bool DoorDown = false;
		uint32 AbsTileZ = 0;

		for (uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
		{
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice = (RandomNumberTable[RandomNumberIndex++] % ((DoorUp || DoorDown) ? 2 : 3));
			bool CreatedZDoor = false;
			if(RandomChoice == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == 0)	
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
				AbsTileZ = 1 - AbsTileZ;	// AbsTileZ is 0 it goes to 1 otherwise, vice versa
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
		entity* ControllingEntity = GetEntity(GameState, GameState->PlayerControllerIndex[ControllerIndex]);
		if(ControllingEntity)
		{
			if(Controller->IsAnalog)
			{
#if RAAVANAN_INTERNAL
				GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
				GameState->YOffset += (int)(4.0f * (Controller->StickAverageX));
#endif
			}
			else
			{
#if RAAVANAN_INTERNAL
				if(Controller->MoveDown.EndedDown)
				{
					GameState->YOffset -= 1;
				}
				if(Controller->MoveUp.EndedDown)
				{
					GameState->YOffset += 1;
				}
#endif
				v2 ddPlayerP = {};
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
				MovePlayer(GameState, ControllingEntity, Input->deltaTime, ddPlayerP);
				// char TextBuffer[256];
				// sprintf_s(TextBuffer, "T-ID:%f R: %f \n", GameState->PlayerX, GameState->PlayerY);
				// OutputDebugStringA(TextBuffer);
			}
		}
		else
		{
			if(Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddEntity(GameState);
				InitializePlayer(GameState, EntityIndex);
				GameState->PlayerControllerIndex[ControllerIndex] = EntityIndex;
			}
		}
		
#if RAAVANAN_INTERNAL
		GameState->PlayerX += (int)(4.0f * (Controller->StickAverageX));
		GameState->PlayerY -= (int)(4.0f * (Controller->StickAverageY));
		if(GameState->jumpTime > 0)
		{
			GameState->PlayerY += (int)(10.0f * sinf(3.14f * GameState->jumpTime));
		}
		if(Controller->ActionDown.EndedDown)
		{
			GameState->jumpTime = 2.0f;
		}
		GameState->jumpTime -= 0.033f;
#endif
	}

	entity * CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity)
	{
		GameState->CameraP.AbsTileZ = CameraFollowingEntity->Pos.AbsTileZ;
		tile_map_difference Diff = Subtract(TileMap, &CameraFollowingEntity->Pos, &GameState->CameraP);
		if (Diff.dXY.X > (9.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraP.AbsTileX += 17;
		}
		if (Diff.dXY.X < -(9.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraP.AbsTileX -= 17;
		}
		if (Diff.dXY.Y > (5.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraP.AbsTileY += 9;
		}
		if (Diff.dXY.Y < -(5.0f * TileMap->TileSideInMeters))
		{
			GameState->CameraP.AbsTileY -= 9;
		}
	}	

	RenderBitMap (Buffer, &GameState->Backdrop, 0, 0);

	float ScreenCenterX = 0.5f * (float)Buffer->Width;
	float ScreenCenterY = 0.5f * (float)Buffer->Height;
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
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	entity* Entity = GameState->Entities;
	for (uint32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex, ++Entity)
	{
		if (Entity->Exists)
		{
			tile_map_difference Diff = Subtract (TileMap, &Entity->Pos, &GameState->CameraP);
			float PlayerR = 1.0f;
			float PlayerG = 0.4f;
			float PlayerB = 0.7f;
			float PlayerGroundPointX = ScreenCenterX + MeterToPixels * Diff.dXY.X;
			float PlayerGroundPointY = ScreenCenterY - MeterToPixels * Diff.dXY.Y;
			v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MeterToPixels * Entity->Width, PlayerGroundPointY - MeterToPixels * Entity->Height};
			v2 EntiryWidthHeight = {Entity->Width, Entity->Height};
			RenderRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MeterToPixels * EntiryWidthHeight, PlayerR, PlayerG, PlayerB);	
			hero_bitmaps *HeroBitsmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
			RenderBitMap(Buffer, &HeroBitsmaps->Torso, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
			RenderBitMap(Buffer, &HeroBitsmaps->Cape, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
			RenderBitMap(Buffer, &HeroBitsmaps->Head, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
		}
	}
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
