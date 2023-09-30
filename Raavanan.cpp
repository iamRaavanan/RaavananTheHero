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

static void RenderRectangle(game_offscreen_buffer *Buffer, float realMinX, float realMinY, float realMaxX, float realMaxY, float R, float G, float B)
{
	int32 MinX = RoundFloatToInt32(realMinX);
	int32 MinY = RoundFloatToInt32(realMinY);
	int32 MaxX = RoundFloatToInt32(realMaxX);
	int32 MaxY = RoundFloatToInt32(realMaxY);
	
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

static void RenderBitMap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, float realX, float realY)
{
	int32 MinX = RoundFloatToInt32(realX);
	int32 MinY = RoundFloatToInt32(realY);
	int32 MaxX = RoundFloatToInt32(realX + Bitmap->Width);
	int32 MaxY = RoundFloatToInt32(realY + Bitmap->Height);
	
	MinX = (MinX < 0) ? 0 : MinX;
	MinY = (MinY < 0) ? 0 : MinY;
	MaxX = (MaxX > Buffer->Width) ? Buffer->Width : MaxX;
	MaxY = (MaxY > Buffer->Height) ? Buffer->Height : MaxY;

	uint32 *SrcRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	uint8 *DstRow = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Dst = (uint32 *)DstRow;
		uint32 *Src = SrcRow;
		for (int x = MinX; x < MaxX; ++x)
		{
			*Dst++ = *Src++;
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

struct bit_scan_result
{
	bool Found;
	uint32 Index;
};
static bit_scan_result FindLSBSetBit (uint32 Value)
{
	bit_scan_result Result = {};
	for (uint32 Test = 0; Test < 32; ++Test)
	{
		if (Value & (1 << Test))
		{
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}
	return Result;
}

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
		GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/structured_art.bmp");
		GameState->Backdrop = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_background.bmp");

		GameState->PlayerHead = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_head.bmp");
		GameState->PlayerCape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_cape.bmp");
		GameState->PlayerTorso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_torso.bmp");

		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 2;
		GameState->PlayerP.OffsetX = 5.0f;
		GameState->PlayerP.OffsetY = 5.0f;
		
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
			float dPlayerX = 0.0f;
			float dPlayerY = 0.0f;
			if(Controller->MoveUp.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				dPlayerY = -1.0f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			float PlayerSpeed = 2.0f;
			if(Controller->ActionUp.EndedDown)
			{
				PlayerSpeed = 10.0f;
			}
			dPlayerX *= PlayerSpeed;
			dPlayerY *= PlayerSpeed;

			tile_map_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.OffsetX += dPlayerX * Input->deltaTime;
			NewPlayerP.OffsetY += dPlayerY * Input->deltaTime;
			NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

			tile_map_position PlayerLeft = NewPlayerP;
			PlayerLeft.OffsetX -= 0.5f * PlayerWidth;
			PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

			tile_map_position PlayerRight = NewPlayerP;
			PlayerRight.OffsetX += 0.5f * PlayerWidth;
			PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);
			
			if(IsValidTileMapPoint(TileMap, NewPlayerP) && IsValidTileMapPoint(TileMap, PlayerLeft) && IsValidTileMapPoint(TileMap, PlayerRight))
			{
				if(!AreOnSameTile(&GameState->PlayerP, &NewPlayerP))
				{
					uint32 NewTileValue = GetTileValue(TileMap, NewPlayerP);
					if(NewTileValue == 3)
					{
						++NewPlayerP.AbsTileZ;
					}
					else if (NewTileValue == 4)
					{
						--NewPlayerP.AbsTileZ;
					}
				}
				GameState->PlayerP = NewPlayerP;
			}
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

	RenderBitMap (Buffer, &GameState->Backdrop, 0, 0);

	float ScreenCenterX = 0.5f * (float)Buffer->Width;
	float ScreenCenterY = 0.5f * (float)Buffer->Height;
	for (int32 Relrow = -10; Relrow < 10; ++Relrow)
	{
		for(int32 Relcol = - 20; Relcol < 20; ++Relcol)
		{
			uint32 Column = GameState->PlayerP.AbsTileX + Relcol;
			uint32 Row = GameState->PlayerP.AbsTileY + Relrow;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
			if(TileID > 1)
			{
				float ColorVal = ((TileID == 2) ? 1.0f : ((TileID > 2) ? 0.25f : 0.5f));
				ColorVal = ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) ? 0.0f : ColorVal;
				float CenterX = ScreenCenterX - MeterToPixels * GameState->PlayerP.OffsetX + ((float)Relcol) * TileSideInPixels;
				float CenterY = ScreenCenterY + MeterToPixels * GameState->PlayerP.OffsetY - ((float)Relrow) * TileSideInPixels;
				float MinX = CenterX - 0.5f * TileSideInPixels;
				float MinY = CenterY - 0.5f * TileSideInPixels;
				float MaxX = CenterX + 0.5f * TileSideInPixels;
				float MaxY = CenterY + 0.5f * TileSideInPixels;
				RenderRectangle(Buffer, MinX, MinY, MaxX, MaxY, ColorVal, ColorVal, ColorVal);
			}			
		}
	}
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	float PlayerR = 1.0f;
	float PlayerG = 0.4f;
	float PlayerB = 0.7f;	
	float PlayerLeft = ScreenCenterX - 0.5f * MeterToPixels * PlayerWidth;
	float PlayerTop = ScreenCenterY - MeterToPixels * PlayerHeight;
	RenderRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + MeterToPixels * PlayerWidth, PlayerTop + MeterToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);	
	RenderBitMap(Buffer, &GameState->PlayerHead, PlayerLeft, PlayerTop);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
