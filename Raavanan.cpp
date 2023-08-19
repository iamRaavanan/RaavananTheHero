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

static void GameUpdateAndRender (game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer)
{
	static int xOffset = 0;
	static int yOffset = 0;
	static int ToneHz = 256;

	game_controller_input *Input0 = &Input->Controllers[0];
	if(Input0->IsAnalog)
	{
		ToneHz = 256 + (int)(128.0f * (Input0->EndY));
		yOffset += (int)4.0f * (Input0->EndX);
	}
	else
	{

	}
	
	if(Input0->Down.EndedDown)
	{
		xOffset += 1;
	}

	UpdateSound(SoundBuffer, ToneHz);
    RenderGradiant(Buffer, xOffset, yOffset);
}