#ifndef RAAVANAN_TILE_H

struct tile_map_difference
{
	v2 dXY;
	float dZ;
};

struct tile_map_position
{
	// Note: Fixexd tile position.  High bits for Tile chunk index, Low bits for Tile index in the chunk
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;
	// Tile relative X & Y
	v2 Offset;
};

struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct tile_map
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	float TileSideInMeters;
	
	uint32 TileChunkXCount;
	uint32 TileChunkYCount;
	uint32 TileChunkZCount;
	tile_chunk *TileChunks;
};
#define RAAVANAN_TILE_H
#endif