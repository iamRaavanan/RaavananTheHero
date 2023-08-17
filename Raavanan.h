#ifndef RAAVANAN_H
struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

static void GameUpdateAndRender(game_offscreen_buffer *Buffer, int xOffset, int yOffset);

#define RAAVANAN_H
#endif