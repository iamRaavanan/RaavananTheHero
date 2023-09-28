#include "Raavanan.h"
#include "Raavanan_tile.h"

inline void RecanonicalizeCoord (tile_map *tilemap, uint32 *Tile, float *TileRelative)
{
	int32 Offset = RoundFloatToInt32(*TileRelative / tilemap->TileSideInMeters);
	*Tile += Offset;
	*TileRelative -= Offset * tilemap->TileSideInMeters;
	//Assert (*TileRelative >= -0.5f * tilemap->TileSideInMeters);
	//Assert(*TileRelative <= 0.5f * tilemap->TileSideInMeters);
}

inline tile_map_position ReCanonicalizePosition (tile_map *tilemap, tile_map_position Position)
{
	tile_map_position Result = Position;
	RecanonicalizeCoord(tilemap, &Result.AbsTileX	, &Result.RelativeX);
	RecanonicalizeCoord(tilemap, &Result.AbsTileY, &Result.RelativeY);
	return Result;
}

inline tile_chunk *GetTileChunk (tile_map *tilemap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
	tile_chunk *TileChunk = 0;
	if((TileChunkX >= 0) && (TileChunkX < tilemap->TileChunkXCount) && 
		(TileChunkY >= 0) && (TileChunkY < tilemap->TileChunkYCount) &&
		(TileChunkZ >= 0) && (TileChunkZ < tilemap->TileChunkZCount))
	{
		TileChunk = &tilemap->TileChunks[TileChunkZ * tilemap->TileChunkYCount * tilemap->TileChunkXCount + TileChunkY * tilemap->TileChunkXCount + TileChunkX];
	}
	return TileChunk;
}

inline uint32 GetTileIndex1D(tile_map *tilemap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < tilemap->ChunkDim);
	Assert(TileY < tilemap->ChunkDim);
	uint32 TileChunkValue = TileChunk->Tiles[TileY * tilemap->ChunkDim + TileX];
	return TileChunkValue;
}

inline void SetTileIndex1D(tile_map *tilemap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue)
{
	Assert(TileChunk);
	Assert(TileX < tilemap->ChunkDim);
	Assert(TileY < tilemap->ChunkDim);
	TileChunk->Tiles[TileY * tilemap->ChunkDim + TileX] = TileValue;
}

static uint32 GetTileValue(tile_map *tilemap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
	uint32 TileChunkValue = 0;
	if(TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileIndex1D(tilemap, TileChunk, TestTileX, TestTileY);
	}
	return TileChunkValue;
}

static void SetTileValue(tile_map *tilemap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY, uint32 TileValue)
{
	if(TileChunk && TileChunk->Tiles)
	{
		SetTileIndex1D(tilemap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}

inline tile_chunk_position GetChunkPositionFor(tile_map *tilemap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> tilemap->ChunkShift;
	Result.TileChunkY = AbsTileY >> tilemap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & tilemap->ChunkMask;
	Result.RelTileY = AbsTileY & tilemap->ChunkMask;
	return Result;
}

static uint32 GetTileValue (tile_map *tilemap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(tilemap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(tilemap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	uint32 TileChunkValue = GetTileValue(tilemap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return TileChunkValue;
}

static bool IsValidTileMapPoint (tile_map *tilemap, tile_map_position CanonicalPos)
{
	uint32 TileChunkValue = GetTileValue (tilemap,  CanonicalPos.AbsTileX, CanonicalPos.AbsTileY, CanonicalPos.AbsTileZ);
	bool Result = (TileChunkValue == 1);
	return Result;
}

static void SetTileValue (memory_arena *MemoryArena, tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	Assert(TileChunk);
	if(!TileChunk->Tiles)
	{
		uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
		TileChunk->Tiles = PushArray(MemoryArena, TileCount, uint32);
		for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
		{
			TileChunk->Tiles[TileIndex] = 1;
		}
	}
	SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}
