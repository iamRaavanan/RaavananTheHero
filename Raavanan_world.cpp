#include "Raavanan.h"
#include "Raavanan_intrinsics.h"
#include "Raavanan_world.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

inline bool IsCanonical(world* World, float TileRelative)
{
	bool Result = ((TileRelative >= -0.5f * World->ChunkSideInMeters) &&
	(TileRelative <= 0.5f * World->ChunkSideInMeters));
	return Result;
}

inline bool IsCanonical(world* World, v2 Offset)
{
	bool Result = (IsCanonical(World, Offset.X) && IsCanonical(World, Offset.Y));
	return Result;
}

inline bool AreInSameChunk(world* World, world_position *A, world_position *B)
{
	Assert(IsCanonical(World, A->Offset));
	Assert(IsCanonical(World, B->Offset));
	bool Result = ((A->ChunkX == B->ChunkX) && (A->ChunkY == B->ChunkY) && (A->ChunkZ == B->ChunkZ));
	return Result;
}

inline world_chunk *GetWorldChunk (world *world, int32 ChunkX, int32 ChunkY, int32 ChunkZ, memory_arena *Arena = 0)
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

inline void RecanonicalizeCoord (world *World, int32 *Tile, float *TileRelative)
{
	int32 Offset = RoundFloatToInt32(*TileRelative / World->ChunkSideInMeters);
	*Tile += Offset;
	*TileRelative -= Offset * World->ChunkSideInMeters;	
	Assert(IsCanonical(World, *TileRelative));
}

inline world_position MapIntoTileSpace (world *world, world_position BasePosition, v2 Offset)
{
	world_position Result = BasePosition;
	Result.Offset += Offset;
	RecanonicalizeCoord(world, &Result.ChunkX, &Result.Offset.X);
	RecanonicalizeCoord(world, &Result.ChunkY, &Result.Offset.Y);
	return Result;
}

inline world_position ChunkPositionFromTilePosition(world* World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
	world_position Result = {};
	Result.ChunkX = AbsTileX / TILES_PER_CHUNK;
	Result.ChunkY = AbsTileY / TILES_PER_CHUNK;
	Result.ChunkZ = AbsTileZ / TILES_PER_CHUNK;
	Result.Offset.X = (AbsTileX - (Result.ChunkX * TILES_PER_CHUNK)) * World->TileSideInMeters;
	Result.Offset.Y = (AbsTileY - (Result.ChunkY * TILES_PER_CHUNK)) * World->TileSideInMeters;
	return Result;
}

inline world_difference Subtract (world *world, world_position *A, world_position *B)
{
	world_difference Result = {};
	v2 dTile = {(float)A->ChunkX - (float)B->ChunkX, (float)A->ChunkY - (float)B->ChunkY};
	float dTileZ = (float)A->ChunkZ - (float)B->ChunkZ;

	Result.dXY = world->ChunkSideInMeters * dTile + A->Offset - B->Offset;
	Result.dZ = world->ChunkSideInMeters * dTileZ +  0.0f;
	return Result;
}

inline world_position CenteredChunkPoint (uint32 AbsTileX, uint32 AbsTileY,uint32 AbsTileZ)
{
	world_position Result = {};
	Result.ChunkX = AbsTileX;
	Result.ChunkY = AbsTileY;
	Result.ChunkZ = AbsTileZ;
	return Result;
}

static void Initializeworld (world *World, float TileSideInMeteres)
{
	World->TileSideInMeters = TileSideInMeteres;
	World->ChunkSideInMeters = (float)TILES_PER_CHUNK * TileSideInMeteres;
	World->FirstFree = 0;
	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex)
	{
		World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
		World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
	}
}

inline void ChangeEntityLocation (memory_arena *Arena, world *World, uint32 LowEntityIndex, world_position *OldP, world_position *NewP)
{
	if(OldP && AreInSameChunk(World, OldP, NewP))
	{

	}
	else
	{
		if(OldP)
		{
			world_chunk* Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
			Assert(Chunk);
			if(Chunk)
			{
				world_entity_block *FirstBlock = &Chunk->FirstBlock;
				for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
				{
					for (uint32 index = 0; index < Block->EntityCount; ++index)
					{
						if(Block->LowEntityIndex[index] == LowEntityIndex)
						{
							Block->LowEntityIndex[index] = FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
							if(FirstBlock->EntityCount == 0)
							{
								if(FirstBlock->Next)
								{
									world_entity_block* NextBlock = FirstBlock->Next;
									*FirstBlock = *NextBlock;

									NextBlock->Next = World->FirstFree;
									World->FirstFree = NextBlock;
								}
							}
							Block = 0;
							break;
						}
					}
				}
			}
		}
		world_chunk* Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ);
		world_entity_block* Block = &Chunk->FirstBlock;
		if(Block->EntityCount == ArrayCount(Block->LowEntityIndex))
		{
			world_entity_block* OldBlock = World->FirstFree;
			if(OldBlock)
			{
				World->FirstFree = OldBlock->Next;
			}
			else
			{
				OldBlock = PushStruct(Arena, world_entity_block);
			}
			*OldBlock = *Block;
			Block->Next = OldBlock;
			Block->EntityCount = 0;
		}
		Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
		Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
	}
}