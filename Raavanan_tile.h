#ifndef RAAVANAN_TILE_H

struct tile_map_difference
{
	v2 dXY;
	float dZ;
};

struct tile_map_position
{
	// Note: Fixexd tile position.  High bits for Tile chunk index, Low bits for Tile index in the chunk
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;
	// Tile relative X & Y
	v2 Offset;
};

struct tile_chunk_position
{
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	int32 RelTileX;
	int32 RelTileY;
};

struct tile_chunk
{
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;
	uint32 *Tiles;
	tile_chunk *NextInHash;
};

struct tile_map
{
	int32 ChunkShift;
	int32 ChunkMask;
	int32 ChunkDim;

	float TileSideInMeters;
	
	tile_chunk TileChunkHash[4096];
};
#define RAAVANAN_TILE_H
#endif