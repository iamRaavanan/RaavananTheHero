#include "Raavanan_intrinsics.h"
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

	uint32 Color = ((RoundFloatToInt32(R * 255.0f) << 16 | RoundFloatToInt32(G * 255.0f) << 8 | RoundFloatToInt32(B * 255.0f)));
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

inline tile_map *GetTileMap (world *World, int32 TileMapX, int32 TileMapY)
{
	tile_map *TileMap = 0;
	if((TileMapX >= 0) && (TileMapX < World->TileMapXCount) && 
		(TileMapY >= 0) && (TileMapY < World->TileMapXCount))
	{
		TileMap = &World->TileMaps[TileMapY * World->TileMapXCount + TileMapX];
		
	}
	return TileMap;
}

inline uint32 GetTileIndex1D(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	Assert(TileMap);
	uint32 TileMapValue = TileMap->Tiles[TileY * World->XCount + TileX];
	return TileMapValue;
}

static bool IsValidTile(world *World, tile_map *Tile, int32 TestTileX, int32 TestTileY)
{
	bool Result = false;
	if(Tile)
	{
		if((TestTileX >= 0) &&(TestTileX < World->XCount) &&
			(TestTileY >= 0) &&(TestTileY < World->YCount))
		{
			uint32 TileValue = GetTileIndex1D(World, Tile, TestTileX, TestTileY);
			Result = (TileValue == 0);
		}
	}
	return Result;
}

inline canonical_position GetCanonicalPosition (world *World, raw_position Position)
{
	canonical_position Result;
	Result.TileMapX = Position.TileMapX;
	Result.TileMapY = Position.TileMapY;
	float X = Position.X - World->UpperLeftX;
	float Y = Position.Y - World->UpperLeftY;
	Result.TileX = TruncateFloatToInt32(X / World->TileSideInPixels);
	Result.TileY = TruncateFloatToInt32(Y / World->TileSideInPixels);
	Result.RelativeX = X - Result.TileX * World->TileSideInPixels;
	Result.RelativeY = Y - Result.TileY * World->TileSideInPixels;
	if(Result.TileX < 0)
	{
		Result.TileX = World->XCount + Result.TileX;
		--Result.TileMapX;
	}
	if(Result.TileY < 0)
	{
		Result.TileY = World->YCount + Result.TileY;
		--Result.TileMapY;
	}
	if(Result.TileX >= World->XCount)
	{
		Result.TileX = Result.TileX - World->XCount;
		++Result.TileMapX;
	}
	if(Result.TileY >= World->YCount)
	{
		Result.TileY = Result.TileY - World->YCount;
		++Result.TileMapY;
	}
	return Result;
}

static bool IsValidWorldPoint (world *World, raw_position TestPos)
{
	bool Result = false;
	canonical_position CanonicalPos = GetCanonicalPosition(World, TestPos);
	tile_map *TileMap = GetTileMap(World, CanonicalPos.TileMapX, CanonicalPos.TileMapY);
	Result = IsValidTile(World, TileMap, CanonicalPos.TileX, CanonicalPos.TileY);
	return Result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
	
	uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};
	tile_map TileMaps[2][2];
	TileMaps[0][0].Tiles = (uint32 *)Tiles00;
	TileMaps[0][1].Tiles = (uint32 *)Tiles10;
	TileMaps[1][0].Tiles = (uint32 *)Tiles01;
	TileMaps[1][1].Tiles = (uint32 *)Tiles11;

	world World;
	World.TileMapXCount = 2;
	World.TileMapYCount = 2;	
	World.XCount = TILE_MAP_COUNT_X;
	World.YCount = TILE_MAP_COUNT_Y;
	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.TileMaps = (tile_map *)TileMaps;	
	
	World.UpperLeftY = 0;
	World.UpperLeftX = (float)-World.TileSideInPixels/2;

	float PlayerWidth = 0.75f * World.TileSideInPixels;
	float PlayerHeight = World.TileSideInPixels;

	tile_map *CurrentTileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
	Assert(CurrentTileMap);
	if(!Memory->IsInitialized)
	{
		GameState->PlayerX = 150;
		GameState->PlayerY = 150;

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
				dPlayerY = -1.0f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			dPlayerX *= 128.0f;
			dPlayerY *= 128.0f;
			float NewPlayerX = GameState->PlayerX + dPlayerX * Input->deltaTime;
			float NewPlayerY = GameState->PlayerY + dPlayerY * Input->deltaTime;

			raw_position PlayerPos = {GameState->PlayerTileMapX, GameState->PlayerTileMapY, NewPlayerX, NewPlayerY};
			raw_position PlayerLeft = PlayerPos;
			PlayerLeft.X -= 0.5f * PlayerWidth;
			raw_position PlayerRight = PlayerPos;
			PlayerRight.X += 0.5f * PlayerWidth;
			if(IsValidWorldPoint(&World, PlayerPos) && IsValidWorldPoint(&World, PlayerLeft) && IsValidWorldPoint(&World, PlayerRight))
			{
				canonical_position CanonicalPos = GetCanonicalPosition(&World, PlayerPos);
				GameState->PlayerTileMapX = CanonicalPos.TileMapX;
				GameState->PlayerTileMapY = CanonicalPos.TileMapY;
				GameState->PlayerX = World.UpperLeftX + World.TileSideInPixels * CanonicalPos.TileX + CanonicalPos.RelativeX;
				GameState->PlayerY = World.UpperLeftY + World.TileSideInPixels * CanonicalPos.TileY + CanonicalPos.RelativeY;
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
	for (int row = 0; row < 9; ++row)
	{
		for(int col = 0; col < 17; ++col)
		{
			uint32 TileID = GetTileIndex1D(&World, CurrentTileMap, col, row);
			float ColorVal = (TileID == 1) ? 1.0f : 0.5f;
			float MinX = World.UpperLeftX + ((float)col) * World.TileSideInPixels;
			float MinY = World.UpperLeftY + ((float)row) * World.TileSideInPixels;
			float MaxX = MinX + World.TileSideInPixels;
			float MaxY = MinY + World.TileSideInPixels;
			RenderRectangle(Buffer, MinX, MinY, MaxX, MaxY, ColorVal, ColorVal, ColorVal);
		}
	}
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	float PlayerR = 1.0f;
	float PlayerG = 0.4f;
	float PlayerB = 0.7f;	
	float PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
	float PlayerTop = GameState->PlayerY - PlayerHeight;
	RenderRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
