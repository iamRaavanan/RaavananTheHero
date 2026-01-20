#include "Raavanan.h"
#include "Raavanan_world.h"

inline void RecanonicalizeCoord (world *tilemap, int32 *Tile, float *TileRelative)
{
	int32 Offset = RoundFloatToInt32(*TileRelative / tilemap->TileSideInMeters);
	*Tile += Offset;
	*TileRelative -= Offset * tilemap->TileSideInMeters;
	Assert (*TileRelative > -0.5f * tilemap->TileSideInMeters);
	Assert(*TileRelative < 0.5f * tilemap->TileSideInMeters);
}

inline world_position MapIntoTileSpace (world *tilemap, world_position BasePosition, v2 Offset)
{
	world_position Result = BasePosition;
	Result.Offset += Offset;
	RecanonicalizeCoord(tilemap, &Result.AbsTileX	, &Result.Offset.X);
	RecanonicalizeCoord(tilemap, &Result.AbsTileY, &Result.Offset.Y);
	return Result;
}

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_chunk *GetTileChunk (world *tilemap, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ,
								memory_arena *Arena = 0)
{
	Assert(TileChunkX > -TILE_CHUNK_SAFE_MARGIN || (TileChunkX < TILE_CHUNK_SAFE_MARGIN));
	Assert(TileChunkY > -TILE_CHUNK_SAFE_MARGIN || (TileChunkY < TILE_CHUNK_SAFE_MARGIN));
	Assert(TileChunkZ > -TILE_CHUNK_SAFE_MARGIN || (TileChunkZ < TILE_CHUNK_SAFE_MARGIN));
	
	uint32 HashValue = 19 * TileChunkX + 7 * TileChunkY + 3 * TileChunkZ;
	uint32 HashSlot = HashValue & (ArrayCount(tilemap->TileChunkHash) - 1);
	
	Assert(HashSlot < ArrayCount(tilemap->TileChunkHash));

	world_chunk *Chunk = tilemap->TileChunkHash + HashSlot;
	do
	{
		if ((TileChunkX == Chunk->ChunkX) &&
			(TileChunkY == Chunk->ChunkY) &&
			(TileChunkZ == Chunk->ChunkZ))
		{
			break;
		}
		if (Arena && (Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
		{
			Chunk->NextInHash = PushStruct (Arena, world_chunk);
			Chunk = Chunk->NextInHash;
			Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
		}
		if (Arena && (Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED))
		{
			uint32 TileCount = tilemap->WorldChunkDim * tilemap->WorldChunkDim;
			Chunk->ChunkX = TileChunkX;
			Chunk->ChunkY = TileChunkY;
			Chunk->ChunkZ = TileChunkZ;
			
			Chunk->NextInHash = 0;
			break;
		}
		Chunk = Chunk->NextInHash;
	} while (Chunk);
	return Chunk;
}

#if 0
inline world_chunk_position GetChunkPositionFor(world *tilemap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_chunk_position Result;
	Result.TileChunkX = AbsTileX >> tilemap->WorldChunkShift;
	Result.TileChunkY = AbsTileY >> tilemap->WorldChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & tilemap->WorldChunkMask;
	Result.RelTileY = AbsTileY & tilemap->WorldChunkMask;
	return Result;
}
#endif

inline bool AreOnSameTile(world_position *A, world_position *B)
{
	bool Result = ((A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ));
	return Result;
}

inline world_difference Subtract (world *TileMap, world_position *A, world_position *B)
{
	world_difference Result = {};
	v2 dTile = {(float)A->AbsTileX - (float)B->AbsTileX, (float)A->AbsTileY - (float)B->AbsTileY};
	float dTileZ = (float)A->AbsTileZ - (float)B->AbsTileZ;

	Result.dXY = TileMap->TileSideInMeters * dTile + A->Offset - B->Offset;
	Result.dZ = TileMap->TileSideInMeters * dTileZ +  0.0f;
	return Result;
}

inline world_position CenteredTilePoint (uint32 AbsTileX, uint32 AbsTileY,uint32 AbsTileZ)
{
	world_position Result = {};
	Result.AbsTileX = AbsTileX;
	Result.AbsTileY = AbsTileY;
	Result.AbsTileZ = AbsTileZ;
	return Result;
}

static void InitializeTileMap (world *TileMap, float TileSideInMeteres)
{
	TileMap->WorldChunkShift = 4;
	TileMap->WorldChunkMask = (1 << TileMap->WorldChunkShift) - 1;	// which is equal to assigning 0xFF
	TileMap->WorldChunkDim = (1 << TileMap->WorldChunkShift);	// Which is equal to 256
	TileMap->TileSideInMeters = TileSideInMeteres;
	for (uint32 TileChunkIndex = 0; TileChunkIndex < ArrayCount(TileMap->TileChunkHash); ++TileChunkIndex)
	{
		TileMap->TileChunkHash[TileChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
	}
}
