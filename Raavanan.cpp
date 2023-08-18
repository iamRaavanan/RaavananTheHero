#include "Raavanan.h"

static void UpdateSound(game_sound_buffer *SoundBuffer, int ToneHz)
{
	static float tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{		
		float SineVal = sinf(tSine);
		int16 Samplevalue = (int16)(SineVal * ToneVolume);
		*SampleOut++ = Samplevalue;
		*SampleOut++ = Samplevalue;
		tSine += 2.0f *  PI * (float)1.0f/(float)WavePeriod;
	}
}

static void RenderGradiant(game_offscreen_buffer *Buffer, int xOffset, int yOffset)
{
	int width = Buffer->Width;
	int height = Buffer->Height;
	
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int y = 0; y < Buffer->Height; ++y) {
		uint32 *pixel = (uint32 *)Row;
		/*
		RGBA to be added in reverse order
		As it's been following the little endian system
		In Memory 	BB GG RR xx 0x
		In Register	0x RR GG BB AA
		*/
		for(int x = 0; x < Buffer->Width; ++x) 
		{
			uint8 Red = (x + xOffset);
			uint8 Green = (y+ yOffset);
			*pixel++ = ((Green << 8) | (Red << 16));
		}
		Row += Buffer->Pitch;
	}
}

static void GameUpdateAndRender (game_offscreen_buffer *Buffer, int xOffset, int yOffset, game_sound_buffer *SoundBuffer, int ToneHz)
{
	UpdateSound(SoundBuffer, ToneHz);
    RenderGradiant(Buffer, xOffset, yOffset);
}