#include "Raavanan_intrinsics.h"
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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
	
	uint32 TempTileData[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	float PlayerHeight = 1.4f;
	float PlayerWidth = 0.75f * PlayerHeight;
	if(!Memory->IsInitialized)
	{

		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 2;
		GameState->PlayerP.RelativeX = 5.0f;
		GameState->PlayerP.RelativeY = 5.0f;
		
		GameState->World = ;
		world *World = GameState->World;
		tile_map *TileMap = World->TileMap;
		
		TileMap->ChunkShift = 8;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;	// which is equal to assigning 0xFF
		TileMap->ChunkDim = 256;

		TileMap->TileChunkXCount = 1;
		TileMap->TileChunkYCount = 1;

		TileMap->TileSideInMeters = 1.4f;
		TileMap->TileSideInPixels = 60;
		TileMap->MeterToPixels = ((float)TileMap->TileSideInPixels / (float)TileMap->TileSideInMeters);

		tile_chunk TileChunk;
		TileChunk.Tiles = (uint32 *)TempTileData;
		TileMap->TileChunks = &TileChunk;		
		
		float LowerLeftX = (float)-TileMap->TileSideInPixels/2;
		float LowerLeftY = (float)Buffer->Height;
		
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		for (uint32 ScreenY = 0; ScreenY < 32; ++ScreenY)
		{
			for (uint32 ScreenX = 0; ScreenX < 32; ++ScreenX)
			{
				for (uint32 TileY = 0; TileY < TilesPerWidth; ++TileY)
				{
					for (uint32 TileX = 0; TileX < TilesPerHeight; ++TileX)
					{
						uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
						uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;
						SetTileValue (TileMap, AbsTileX, AbsTileY);
					}
				}
			}
		}
#if RAAVANAN_INTERNAL
		char *Filename = __FILE__;
		debug_read_file_result File = Memory->DEBUGReadEntireFile(Thread, Filename);
		if(File.Content)
		{
			Memory->DEBUGWriteEntireFile(Thread, "test.out", File.ContentSize, File.Content);
			Memory->DEBUGFreeFileMemory(Thread, File.Content);
		}
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->PlayerX = GameState->PlayerY = 100;
#endif

		Memory->IsInitialized = true;
	}
	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;
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
			NewPlayerP.RelativeX += dPlayerX * Input->deltaTime;
			NewPlayerP.RelativeY += dPlayerY * Input->deltaTime;
			NewPlayerP = ReCanonicalizePosition(TileMap, NewPlayerP);

			tile_map_position PlayerLeft = NewPlayerP;
			PlayerLeft.RelativeX -= 0.5f * PlayerWidth;
			PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

			tile_map_position PlayerRight = NewPlayerP;
			PlayerRight.RelativeX += 0.5f * PlayerWidth;
			PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);
			
			if(IsValidTileMapPoint(TileMap, NewPlayerP) && IsValidTileMapPoint(TileMap, PlayerLeft) && IsValidTileMapPoint(TileMap, PlayerRight))
			{
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
	
	RenderRectangle (Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 0.0f, 0.0f, 0.0f);
	float ScreenCenterX = 0.5f * (float)Buffer->Width;
	float ScreenCenterY = 0.5f * (float)Buffer->Height;
	for (int32 Relrow = -10; Relrow < 10; ++Relrow)
	{
		for(int32 Relcol = - 20; Relcol < 20; ++Relcol)
		{
			uint32 Column = GameState->PlayerP.AbsTileX + Relcol;
			uint32 Row = GameState->PlayerP.AbsTileY + Relrow;
			uint32 TileID = GetTileValue(&TileMap, Column, Row);
			float ColorVal = (TileID == 1) ? 1.0f : 0.5f;
			ColorVal = ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) ? 0.0f : ColorVal;
			float CenterX = ScreenCenterX - TileMap->MeterToPixels * GameState->PlayerP.RelativeX + ((float)Relcol) * TileMap->TileSideInPixels;
			float CenterY = ScreenCenterY + TileMap->MeterToPixels * GameState->PlayerP.RelativeY - ((float)Relrow) * TileMap->TileSideInPixels;
			float MinX = CenterX - 0.5f * TileMap->TileSideInPixels;
			float MinY = CenterY - 0.5f * TileMap->TileSideInPixels;
			float MaxX = CenterX + 0.5f * TileMap->TileSideInPixels;
			float MaxY = CenterY + 0.5f * TileMap->TileSideInPixels;
			RenderRectangle(Buffer, MinX, MinY, MaxX, MaxY, ColorVal, ColorVal, ColorVal);
		}
	}
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	float PlayerR = 1.0f;
	float PlayerG = 0.4f;
	float PlayerB = 0.7f;	
	float PlayerLeft = ScreenCenterX - 0.5f * TileMap->MeterToPixels * PlayerWidth;
	float PlayerTop = ScreenCenterY - TileMap->MeterToPixels * PlayerHeight;
	RenderRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + TileMap->MeterToPixels * PlayerWidth, PlayerTop + TileMap->MeterToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
