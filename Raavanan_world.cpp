#include "Raavanan.h"
#include "Raavanan_intrinsics.h"
#include "Raavanan_world.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_chunk *GetWorldChunk (world *world, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
								memory_arena *Arena = 0)
{
	Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN || (ChunkX < TILE_CHUNK_SAFE_MARGIN));
	Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN || (ChunkY < TILE_CHUNK_SAFE_MARGIN));
	Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN || (ChunkZ < TILE_CHUNK_SAFE_MARGIN));
	
	uint32 HashValue = 19 * ChunkX + 7 * ChunkY + 3 * ChunkZ;
	uint32 HashSlot = HashValue & (ArrayCount(world->ChunkHash) - 1);
	
	Assert(HashSlot < ArrayCount(world->ChunkHash));

	world_chunk *Chunk = world->ChunkHash + HashSlot;
	do
	{
		if ((ChunkX == Chunk->ChunkX) &&
			(ChunkY == Chunk->ChunkY) &&
			(ChunkZ == Chunk->ChunkZ))
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
			uint32 TileCount = world->ChunkDim * world->ChunkDim;
			Chunk->ChunkX = ChunkX;
			Chunk->ChunkY = ChunkY;
			Chunk->ChunkZ = ChunkZ;
			
			Chunk->NextInHash = 0;
			break;
		}
		Chunk = Chunk->NextInHash;
	} while (Chunk);
	return Chunk;
}

#if 0
inline world_chunk_position GetChunkPositionFor(world *world, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_chunk_position Result;
	Result.WorldChunkX = AbsTileX >> world->ChunkShift;
	Result.WorldChunkY = AbsTileY >> world->ChunkShift;
	Result.WorldChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & world->ChunkMask;
	Result.RelTileY = AbsTileY & world->ChunkMask;
	return Result;
}
#endif

inline void RecanonicalizeCoord (world *world, int32 *Tile, float *TileRelative)
{
	int32 Offset = RoundFloatToInt32(*TileRelative / world->TileSideInMeters);
	*Tile += Offset;
	*TileRelative -= Offset * world->TileSideInMeters;
	Assert (*TileRelative > -0.5f * world->TileSideInMeters);
	Assert(*TileRelative < 0.5f * world->TileSideInMeters);
}

inline world_position MapIntoTileSpace (world *world, world_position BasePosition, v2 Offset)
{
	world_position Result = BasePosition;
	Result.Offset += Offset;
	RecanonicalizeCoord(world, &Result.AbsTileX	, &Result.Offset.X);
	RecanonicalizeCoord(world, &Result.AbsTileY, &Result.Offset.Y);
	return Result;
}

inline bool AreOnSameTile(world_position *A, world_position *B)
{
	bool Result = ((A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ));
	return Result;
}

inline world_difference Subtract (world *world, world_position *A, world_position *B)
{
	world_difference Result = {};
	v2 dTile = {(float)A->AbsTileX - (float)B->AbsTileX, (float)A->AbsTileY - (float)B->AbsTileY};
	float dTileZ = (float)A->AbsTileZ - (float)B->AbsTileZ;

	Result.dXY = world->TileSideInMeters * dTile + A->Offset - B->Offset;
	Result.dZ = world->TileSideInMeters * dTileZ +  0.0f;
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

static void Initializeworld (world *world, float TileSideInMeteres)
{
	world->ChunkShift = 4;
	world->ChunkMask = (1 << world->ChunkShift) - 1;	// which is equal to assigning 0xFF
	world->ChunkDim = (1 << world->ChunkShift);	// Which is equal to 256
	world->TileSideInMeters = TileSideInMeteres;
	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(world->ChunkHash); ++ChunkIndex)
	{
		world->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
	}
}
