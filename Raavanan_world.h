#ifndef RAAVANAN_WORLD_H

struct world_difference
{
	v2 dXY;
	float dZ;
};

struct world_position
{
	// Note: Fixexd tile position.  High bits for Tile chunk index, Low bits for Tile index in the chunk
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;
	// Tile relative X & Y
	v2 Offset;
};

struct world_chunk_position
{
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	int32 RelTileX;
	int32 RelTileY;
};

struct world_entity_block
{
	uint32 EntityCount;
	uint32 LowEntityIndex[16];
	world_entity_block *Next;
};

struct world_chunk
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;
	
	world_entity_block FirstBlock;

	world_chunk *NextInHash;
};

struct world
{
	float TileSideInMeters;
	int32 WorldChunkShift;
	int32 WorldChunkMask;
	int32 WorldChunkDim;
	
	world_chunk TileChunkHash[4096];
};
#define RAAVANAN_WORLD_H
#endif