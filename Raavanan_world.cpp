#include "Raavanan.h"
#include "Raavanan_intrinsics.h"
#include "Raavanan_world.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

inline world_position NullPosition(void)
{
	world_position Result = {};
	Result.ChunkX = TILE_CHUNK_UNINITIALIZED;
	return Result;
}

inline bool IsWorldPosValid (world_position P)
{
	bool Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
	return Result;
}

inline bool IsCanonical(float ChunkDim, float TileRelative)
{
	float Epsilon = 0.01f;
	bool Result = ((TileRelative >= -(0.5f * ChunkDim + Epsilon)) &&
					(TileRelative <= (0.5f * ChunkDim + Epsilon)));
	return Result;
}

inline bool IsCanonical(world* World, v3 Offset)
{
	bool Result = (IsCanonical(World->ChunkDimInMeters.X, Offset.X) && 
					IsCanonical(World->ChunkDimInMeters.Y, Offset.Y) &&
					IsCanonical(World->ChunkDimInMeters.Z, Offset.Z));
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

inline void RecanonicalizeCoord (float ChunkDim, int32 *Tile, float *TileRelative)
{
	int32 Offset = RoundFloatToInt32(*TileRelative / ChunkDim);
	*Tile += Offset;
	*TileRelative -= Offset * ChunkDim;	
	Assert(IsCanonical(ChunkDim, *TileRelative));
}

inline world_position MapIntoChunkSpace (world *world, world_position BasePosition, v3 Offset)
{
	world_position Result = BasePosition;
	Result.Offset += Offset;
	RecanonicalizeCoord(world->ChunkDimInMeters.X, &Result.ChunkX, &Result.Offset.X);
	RecanonicalizeCoord(world->ChunkDimInMeters.Y, &Result.ChunkY, &Result.Offset.Y);
	RecanonicalizeCoord(world->ChunkDimInMeters.Z, &Result.ChunkZ, &Result.Offset.Z);
	return Result;
}

inline world_position ChunkPositionFromTilePosition(world* World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ, v3 AdditionalOffset = V3(0,0,0))
{
	world_position BasePos = {};
	v3 TileDim = V3(World->TileSideInMeters, World->TileSideInMeters, World->TileDepthInMeters);
	v3 Offset = Hadamard(TileDim, V3((float)AbsTileX, (float)AbsTileY, (float)AbsTileZ));
	world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);
	Assert(IsCanonical(World, Result.Offset));
	return Result;
}

inline v3 Subtract (world *world, world_position *A, world_position *B)
{
	v3 dTile = {(float)A->ChunkX - (float)B->ChunkX, (float)A->ChunkY - (float)B->ChunkY, (float)A->ChunkZ - (float)B->ChunkZ};

	v3 Result = Hadamard(world->ChunkDimInMeters, dTile) + A->Offset - B->Offset;
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

static void Initializeworld (world *World, float TileSideInMeteres, float TileDepthInMeters)
{
	World->TileSideInMeters = TileSideInMeteres;
	World->ChunkDimInMeters = {(float)TILES_PER_CHUNK * TileSideInMeteres,
								(float)TILES_PER_CHUNK * TileSideInMeteres,
								(float)TileDepthInMeters};
	World->TileDepthInMeters = (float)TileDepthInMeters;
	World->FirstFree = 0;
	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex)
	{
		World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
		World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
	}
}

inline void ChangeEntityLocationRaw (memory_arena *Arena, world *World, uint32 LowEntityIndex, world_position *OldP, world_position *NewP)
{
	Assert(!OldP || IsWorldPosValid(*OldP));
	Assert(!NewP || IsWorldPosValid(*NewP));

	if(OldP && NewP && AreInSameChunk(World, OldP, NewP))
	{

	}
	else
	{
		if(OldP)
		{
			world_chunk* Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, Arena);
			Assert(Chunk);
			if(Chunk)
			{
				bool NotFound = true;
				world_entity_block *FirstBlock = &Chunk->FirstBlock;
				for(world_entity_block *Block = FirstBlock; Block && NotFound; Block=Block->Next)
				{
					for (uint32 index = 0; ((index < Block->EntityCount) && NotFound); ++index)
					{
						if(Block->LowEntityIndex[index] == LowEntityIndex)
						{
							Assert(FirstBlock->EntityCount > 0);
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
							NotFound = false;
						}
					}
				}
			}
		}
		if(NewP)
		{
			world_chunk* Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
			Assert(Chunk);
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
}

inline void ChangeEntityLocation (memory_arena *Arena, world *World, uint32 LowEntityIndex, low_entity* LowEntity, world_position NewP)
{
	world_position* OldPos = 0;
	world_position* NewPos = 0;
	if(!IsSet(&LowEntity->Sim, EntityFlag_NonSpatial) && IsWorldPosValid(LowEntity->Pos))
	{
		OldPos = &LowEntity->Pos;
	} 
	if(IsWorldPosValid(NewP))
	{
		NewPos = &NewP;
	}
	ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldPos, NewPos);
	if(NewPos)
	{
		LowEntity->Pos = *NewPos;
		ClearFlags(&LowEntity->Sim, EntityFlag_NonSpatial);
	}
	else
	{
		LowEntity->Pos = NullPosition();
		AddFlags(&LowEntity->Sim, EntityFlag_NonSpatial);
	}
}