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

inline tile_chunk *GetTileChunk (world *World, int32 TileChunkX, int32 TileChunkY)
{
	tile_chunk *TileChunk = 0;
	if((TileChunk >= 0) && (TileChunkX < World->TileChunkXCount) && 
		(TileChunk >= 0) && (TileChunkY < World->TileChunkYCount))
	{
		TileChunk = &World->TileChunks[TileChunkY * World->TileChunkXCount + TileChunkX];
		
	}
	return TileChunk;
}

inline uint32 GetTileIndex1D(world *World, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < World->ChunkDim);
	Assert(TileY < World->ChunkDim);
	uint32 TileChunkValue = TileChunk->Tiles[TileY * World->ChunkDim + TileX];
	return TileChunkValue;
}

static uint32 GetTileValue(world *World, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
	uint32 TileChunkValue = 0;
	if(TileChunk)
	{
		TileChunkValue = GetTileIndex1D(World, TileChunk, TestTileX, TestTileY);
	}
	return TileChunkValue;
}

inline void RecanonicalizeCoord (world *World, uint32 *Tile, float *TileRelative)
{
	int32 Offset = FloorFloatToInt32(*TileRelative / World->TileSideInMeters);
	*Tile += Offset;
	*TileRelative -= Offset * World->TileSideInMeters;
	Assert (*TileRelative >= 0);
	Assert(*TileRelative < World->TileSideInMeters);
}

inline world_position ReCanonicalizePosition (world *World, world_position Position)
{
	world_position Result = Position;
	RecanonicalizeCoord(World, &Result.AbsTileX	, &Result.RelativeX);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.RelativeY);
	return Result;
}

inline tile_chunk_position GetChunkPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> World->ChunkShift;
	Result.TileChunkY = AbsTileY >> World->ChunkShift;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;
	return Result;
}

static uint32 GetTileValue (world *World, uint32 AbsTileX, uint32 AbsTileY)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
	tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return TileChunkValue;
}

static bool IsValidWorldPoint (world *World, world_position CanonicalPos)
{
	uint32 TileChunkValue = GetTileValue (World, CanonicalPos.AbsTileX, CanonicalPos.AbsTileY);
	bool Result = (TileChunkValue == 0);
	return Result;
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

	world World;
	World.ChunkShift = 8;
	World.ChunkMask = (1 << World.ChunkShift) - 1;	// which is equal to assigning 0xFF
	World.ChunkDim = 256;

	World.TileChunkXCount = 1;
	World.TileChunkYCount = 1;

	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.MeterToPixels = ((float)World.TileSideInPixels / (float)World.TileSideInMeters);

	tile_chunk TileChunk;
	TileChunk.Tiles = (uint32 *)TempTileData;
	World.TileChunks = &TileChunk;

	float PlayerHeight = 1.4f;
	float PlayerWidth = 0.75f * PlayerHeight;
	
	float LowerLeftX = (float)-World.TileSideInPixels/2;
	float LowerLeftY = (float)Buffer->Height;

	if(!Memory->IsInitialized)
	{
		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 2;
		GameState->PlayerP.RelativeX = 5.0f;
		GameState->PlayerP.RelativeY = 5.0f;

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
			dPlayerX *= 2.0f;
			dPlayerY *= 2.0f;

			world_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.RelativeX += dPlayerX * Input->deltaTime;
			NewPlayerP.RelativeY += dPlayerY * Input->deltaTime;
			NewPlayerP = ReCanonicalizePosition(&World, NewPlayerP);

			world_position PlayerLeft = NewPlayerP;
			PlayerLeft.RelativeX -= 0.5f * PlayerWidth;
			PlayerLeft = ReCanonicalizePosition(&World, PlayerLeft);

			world_position PlayerRight = NewPlayerP;
			PlayerRight.RelativeX += 0.5f * PlayerWidth;
			PlayerRight = ReCanonicalizePosition(&World, PlayerRight);
			
			if(IsValidWorldPoint(&World, NewPlayerP) && IsValidWorldPoint(&World, PlayerLeft) && IsValidWorldPoint(&World, PlayerRight))
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
	float CenterX = 0.5f * (float)Buffer->Width;
	float CenterY = 0.5f * (float)Buffer->Height;
	for (int32 Relrow = -10; Relrow < 10; ++Relrow)
	{
		for(int32 Relcol = - 20; Relcol < 20; ++Relcol)
		{
			uint32 Column = GameState->PlayerP.AbsTileX + Relcol;
			uint32 Row = GameState->PlayerP.AbsTileY + Relrow;
			uint32 TileID = GetTileValue(&World, Column, Row);
			float ColorVal = (TileID == 1) ? 1.0f : 0.5f;
			ColorVal = ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) ? 0.0f : ColorVal;
			float MinX = CenterX + ((float)Relcol) * World.TileSideInPixels;
			float MinY = CenterY - ((float)Relrow) * World.TileSideInPixels;
			float MaxX = MinX + World.TileSideInPixels;
			float MaxY = MinY - World.TileSideInPixels;
			RenderRectangle(Buffer, MinX, MaxY, MaxX, MinY, ColorVal, ColorVal, ColorVal);
		}
	}
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	float PlayerR = 1.0f;
	float PlayerG = 0.4f;
	float PlayerB = 0.7f;	
	float PlayerLeft = CenterX + World.MeterToPixels * GameState->PlayerP.RelativeX - 0.5f * World.MeterToPixels * PlayerWidth;
	float PlayerTop = CenterY - World.MeterToPixels * GameState->PlayerP.RelativeY - World.MeterToPixels * PlayerHeight;
	RenderRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + World.MeterToPixels * PlayerWidth, PlayerTop + World.MeterToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
