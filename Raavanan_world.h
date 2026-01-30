#ifndef RAAVANAN_WORLD_H

struct world_difference
{
	v2 dXY;
	float dZ;
};

struct world_position
{
	// Puzzler! How can we get rid of AbsTile* here, and still allow references to entities to be able to figure
	// out where they are( or rather, which worldchunk they are in ?)
	// Note: Fixexd tile position.  High bits for Tile chunk index, Low bits for Tile index in the chunk
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;
	// Tile relative X & Y
	v2 Offset;
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
	int32 ChunkShift;
	int32 ChunkMask;
	int32 ChunkDim;
	
	world_chunk ChunkHash[4096];
};
#define RAAVANAN_WORLD_H
#endif