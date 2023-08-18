#ifndef RAAVANAN_H
struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_buffer 
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};
static void UpdateSound(game_sound_buffer *SoundBuffer);
static void GameUpdateAndRender(game_offscreen_buffer *Buffer, int xOffset, int yOffset, 
								game_sound_buffer *SoundBuffer);

#define RAAVANAN_H
#endif