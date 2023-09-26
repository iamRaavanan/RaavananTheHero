#ifndef RAAVANAN_TILE_H

struct tile_map_position
{
	// Note: Fixexd tile position.  High bits for Tile chunk index, Low bits for Tile index in the chunk
	uint32 AbsTileX;
	uint32 AbsTileY;
	// Tile relative X & Y
	float RelativeX;
	float RelativeY;
};

struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;

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
	int32 TileSideInPixels; // Tile width and Height
	float MeterToPixels;
	
	uint32 TileChunkXCount;
	uint32 TileChunkYCount;
	tile_chunk *TileChunks;
};
#define RAAVANAN_TILE_H
#endif