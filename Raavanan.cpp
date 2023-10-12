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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	float PlayerHeight = 1.4f;
	float PlayerWidth = 0.75f * PlayerHeight;	

	if(!Memory->IsInitialized)
	{
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
		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 2;
		GameState->PlayerP.Offset.X = 5.0f;
		GameState->PlayerP.Offset.Y = 5.0f;
		
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

	tile_map_position OldPlayerP = GameState->PlayerP;

	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
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
				GameState->HeroFacingDirection = 1;
				ddPlayerP.Y = 1.0f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				GameState->HeroFacingDirection = 3;
				ddPlayerP.Y = -1.0f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				GameState->HeroFacingDirection = 0;
				ddPlayerP.X = 1.0f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				GameState->HeroFacingDirection = 2;
				ddPlayerP.X = -1.0f;
			}
			if((ddPlayerP.X != 0.0f) &&(ddPlayerP.Y != 0.0f))
			{
				ddPlayerP *= 0.7071067811865f;
			}

			float PlayerSpeed = 10.0f;
			if(Controller->ActionUp.EndedDown)
			{
				PlayerSpeed = 50.0f;
			}
			ddPlayerP *= PlayerSpeed;

			ddPlayerP += -1.5f * GameState->dPlayerP;

			tile_map_position NewPlayerP = GameState->PlayerP;
			//Position =  1/2 * acceleration * square(time) + velocity * time + position
			v2 PlayerDelta = 0.5f * ddPlayerP * Square(Input->deltaTime) + GameState->dPlayerP * Input->deltaTime;
			NewPlayerP.Offset += PlayerDelta;
			GameState->dPlayerP = ddPlayerP * Input->deltaTime + GameState->dPlayerP;	
			NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

#if 1
			tile_map_position PlayerLeft = NewPlayerP;
			PlayerLeft.Offset.X -= 0.5f * PlayerWidth;
			PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

			tile_map_position PlayerRight = NewPlayerP;
			PlayerRight.Offset.X += 0.5f * PlayerWidth;
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
				if(CollisionPt.AbsTileX < GameState->PlayerP.AbsTileX)
				{
					r = {1, 0};
				}
				if(CollisionPt.AbsTileX > GameState->PlayerP.AbsTileX)
				{
					r = {-1, 0};
				}
				if(CollisionPt.AbsTileY < GameState->PlayerP.AbsTileY)
				{
					r = {0, 1};
				}
				if(CollisionPt.AbsTileY > GameState->PlayerP.AbsTileY)
				{
					r = {0, -1};
				}
				GameState->dPlayerP = GameState->dPlayerP - 1 * Dot(GameState->dPlayerP, r) * r;				
			}
			else
			{
				GameState->PlayerP = NewPlayerP;
			}			
#else 
			uint32 MintileX = 0;
			uint32 MintileY = 0;
			uint32 OnePastMaxTileX = 0;
			uint32 OnePastMaxTileY = 0;
			uint32 AbsTileZ = GameState->PlayerP.AbsTileZ;
			tile_map_position BestPlayerP = GameState->PlayerP;
			float BestDistance = LengthSq(PlayerDelta);
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
						v2 TestP = ClosePointInRect (MinCorner, MaxCorner, RelNewPlayerP);
					}
				}
			}
#endif
			// char TextBuffer[256];
			// sprintf_s(TextBuffer, "T-ID:%f R: %f \n", GameState->PlayerX, GameState->PlayerY);
			// OutputDebugStringA(TextBuffer);
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

	
	if(!AreOnSameTile(&OldPlayerP, &GameState->PlayerP))
	{
		uint32 NewTileValue = GetTileValue(TileMap, GameState->PlayerP);
		if(NewTileValue == 3)
		{
			++GameState->PlayerP.AbsTileZ;
		}
		else if (NewTileValue == 4)
		{
			--GameState->PlayerP.AbsTileZ;
		}
	}
	GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;
	tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);
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
	Diff = Subtract (TileMap, &GameState->PlayerP, &GameState->CameraP);
	float PlayerR = 1.0f;
	float PlayerG = 0.4f;
	float PlayerB = 0.7f;
	float PlayerGroundPointX = ScreenCenterX + MeterToPixels * Diff.dXY.X;
	float PlayerGroundPointY = ScreenCenterY - MeterToPixels * Diff.dXY.Y;
	v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MeterToPixels * PlayerWidth, PlayerGroundPointY - MeterToPixels * PlayerHeight};
	v2 PlayerWidthHeight = {PlayerWidth, PlayerHeight};
	RenderRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MeterToPixels * PlayerWidthHeight, PlayerR, PlayerG, PlayerB);	
	hero_bitmaps *HeroBitsmaps = &GameState->HeroBitmaps[GameState->HeroFacingDirection];
	RenderBitMap(Buffer, &HeroBitsmaps->Torso, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
	RenderBitMap(Buffer, &HeroBitsmaps->Cape, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
	RenderBitMap(Buffer, &HeroBitsmaps->Head, PlayerGroundPointX, PlayerGroundPointY, HeroBitsmaps->AlignX, HeroBitsmaps->AlignY);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
