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
		tSine += (float)(2.0f *  PI * (float)1.0f/(float)WavePeriod);
	}
}

static void RenderGradiant(game_offscreen_buffer *Buffer, int xOffset, int yOffset)
{	
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
			uint8 Red = (uint8)(x + xOffset);
			uint8 Green = (uint8)(y+ yOffset);
			*pixel++ = ((Green << 8) | (Red << 16));
		}
		Row += Buffer->Pitch;
	}
}

static void GameUpdateAndRender (game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
#if RAAVANAN_INTERNAL
		char *Filename = __FILE__;
		debug_read_file_result File = DEBUGReadEntireFile(Filename);
		if(File.Content)
		{
			DEBUGWriteEntireFile("test.out", File.ContentSize, File.Content);
			DEBUGFreeFileMemory(File.Content);
		}
#endif
		GameState->ToneHz = 256;
		Memory->IsInitialized = true;
	}
	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog)
		{
			GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
			GameState->YOffset += (int)(4.0f * (Controller->StickAverageX));
		}
		else
		{
			if(Controller->MoveDown.EndedDown)
			{
				GameState->YOffset -= 1;
			}
			if(Controller->MoveUp.EndedDown)
			{
				GameState->YOffset += 1;
			}
		}
		if(Controller->ActionDown.EndedDown)
		{
			GameState->XOffset += 1;
		}
	}	
	UpdateSound(SoundBuffer, GameState->ToneHz);
    RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
}