#include "Raavanan.h"
#include "Raavanan_render_group.h"
#include "Raavanan_render_group.cpp"
#include "Raavanan_random.h"
#include "Raavanan_world.cpp"
#include "Raavanan_sim_region.cpp"
#include "Raavanan_entity.cpp"
#include <stdio.h>
#include <windows.h>
internal void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer, int ToneHz)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	int16 *SampleOut = SoundBuffer->Samples;
	
	for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
#if 0
		float SineVal = sinf(GameState->tSine);
		int16 Samplevalue = (int16)(SineVal * ToneVolume);
#else
		int16 Samplevalue = 0;
#endif
		*SampleOut++ = Samplevalue;
		*SampleOut++ = Samplevalue;
#if 0
		GameState->tSine += (float)(2.0f *  PI * (float)1.0f/(float)WavePeriod);
		if(GameState->tSine > (2.0f *  PI))
		{
			GameState->tSine -= (float)(2.0f *  PI);
		}
#endif
	}
}

internal void RenderGradiant(game_offscreen_buffer *Buffer, int xOffset, int yOffset)
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
			*pixel++ = ((Green << 16) | (Red));
		}
		Row += Buffer->Pitch;
	}
}

internal void RenderRectangle(loaded_bitmap* Buffer, v2 vMin, v2 vMax, float R, float G, float B)
{
	int32 MinX = RoundFloatToInt32(vMin.X);
	int32 MinY = RoundFloatToInt32(vMin.Y);
	int32 MaxX = RoundFloatToInt32(vMax.X);
	int32 MaxY = RoundFloatToInt32(vMax.Y);
	
	MinX = (MinX < 0) ? 0 : MinX;
	MinY = (MinY < 0) ? 0 : MinY;
	MaxX = (MaxX > Buffer->Width) ? Buffer->Width : MaxX;
	MaxY = (MaxY > Buffer->Height) ? Buffer->Height : MaxY;

	uint32 Color = ((RoundFloatToUInt32(R * 255.0f) << 16 | RoundFloatToUInt32(G * 255.0f) << 8 | RoundFloatToUInt32(B * 255.0f)));
	uint8 *Row = ((uint8 *)Buffer->Memory + MinX * BITMAP_BYTES_PER_PIXEL + MinY * Buffer->Pitch);
	
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int x = MinX; x < MaxX; ++x)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void RenderRectOutline(loaded_bitmap* Buffer, v2 Min, v2 Max, v3 Color, float T = 2.0f)
{
	float Thickness = 0.1f;
	RenderRectangle(Buffer, V2(Min.X - T, Min.Y - T), V2(Max.X + T, Min.Y + T), Color.R, Color.G, Color.B);
	RenderRectangle(Buffer, V2(Min.X - T, Max.Y - T), V2(Max.X + T, Max.Y + T), Color.R, Color.G, Color.B);

	RenderRectangle(Buffer, V2(Min.X - T, Min.Y - T), V2(Min.X + T, Max.Y + T), Color.R, Color.G, Color.B);
	RenderRectangle(Buffer, V2(Max.X - T, Min.Y - T), V2(Max.X + T, Max.Y + T), Color.R, Color.G, Color.B);
}

internal void RenderBitMap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap, float realX, float realY, float CAlpha = 1.0f)
{
	int32 MinX = RoundFloatToInt32(realX);
	int32 MinY = RoundFloatToInt32(realY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;
	
	int32 SrcOffsetX = 0;
	if (MinX < 0)
	{
		SrcOffsetX = -MinX;
		MinX = 0;
	}
	int32 SrcOffsetY = 0;
	if (MinY < 0)
	{
		SrcOffsetY = -MinY;
		MinY = 0;
	}

	MaxX = (MaxX > Buffer->Width) ? Buffer->Width : MaxX;
	MaxY = (MaxY > Buffer->Height) ? Buffer->Height : MaxY;

	uint8 *SrcRow = (uint8 *)Bitmap->Memory + SrcOffsetY * Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL * SrcOffsetX; 
	uint8 *DstRow = ((uint8 *)Buffer->Memory + MinX * BITMAP_BYTES_PER_PIXEL + MinY * Buffer->Pitch);
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Dst = (uint32 *)DstRow;
		uint32 *Src = (uint32 *)SrcRow;
		for (int x = MinX; x < MaxX; ++x)
		{
			float SA = (float)((*Src >> 24) & 0xFF);
			float RSA = (SA / 255.0f) * CAlpha;
			float SR = CAlpha * (float)((*Src >> 16) & 0xFF);
			float SG = CAlpha * (float)((*Src >> 8) & 0xFF);
			float SB = CAlpha * (float)((*Src >> 0) & 0xFF);
			
			float DA = (float)((*Dst >> 24) & 0xFF);
			float DR = (float)((*Dst >> 16) & 0xFF);
			float DG = (float)((*Dst >> 8) & 0xFF);
			float DB = (float)((*Dst >> 0) & 0xFF);
			float RDA = (DA / 255.0f);

			float OneMinusRSA = (1.0f - RSA);
			float A = 255.0f * (RSA + RDA - RSA* RDA);
			float R = OneMinusRSA * DR + SR;
			float G = OneMinusRSA * DG + SG;
			float B = OneMinusRSA * DB + SB;

			*Dst = (((uint32)(A + 0.5f) << 24) |
					((uint32)(R + 0.5f) << 16) |
					((uint32)(G + 0.5f) << 8) |
					((uint32)(B + 0.5f) << 0));
			++Dst;
			++Src;
		}
		DstRow += Buffer->Pitch;
		SrcRow += Bitmap->Pitch;
	}
}
//#if RAAVANAN_INTERNAL
#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP (thread_context *Thread, debug_read_entire_file *ReadEntireFile, char *FileName)
{
	loaded_bitmap Result = {};
	debug_read_file_result FileResult = ReadEntireFile (Thread, FileName);
	if(FileResult.ContentSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)FileResult.Content;
		uint32 *Pixels = (uint32 *)((uint8 *)FileResult.Content + Header->BitmapOffset);
		Result.Memory = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		Assert(Header->Compression == 3);
uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);        
        
        bit_scan_result RedScan = FindLSBSetBit(RedMask);
        bit_scan_result GreenScan = FindLSBSetBit(GreenMask);
        bit_scan_result BlueScan = FindLSBSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLSBSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32 RedShiftDown = (int32)RedScan.Index;
        int32 GreenShiftDown = (int32)GreenScan.Index;
        int32 BlueShiftDown = (int32)BlueScan.Index;
        int32 AlphaShiftDown = (int32)AlphaScan.Index;
        
        uint32 *SourceDest = Pixels;
        for(int32 Y = 0; Y < Header->Height; ++Y)
        {
            for(int32 X = 0; X < Header->Width; ++X)
            {
                uint32 C = *SourceDest;
				float R = (float)((C & RedMask) >> RedShiftDown);
				float G = (float)((C & GreenMask) >> GreenShiftDown);
				float B = (float)((C & BlueMask) >> BlueShiftDown);
				float A = (float)((C & AlphaMask) >> AlphaShiftDown);
				float AN = (A / 255.0f);
				R = R * AN;
				G = G * AN;
				B = B * AN;
				*SourceDest++ = (((uint32)(A + 0.5f) << 24) |
								((uint32)(R + 0.5f) << 16) |
								((uint32)(G + 0.5f) << 8) |
								((uint32)(B + 0.5f) << 0));
            }
        }
	}
	Result.Pitch = -Result.Width * BITMAP_BYTES_PER_PIXEL;
	Result.Memory = (uint8 *)Result.Memory - Result.Pitch * (Result.Height - 1);
	return Result;
}
//#endif

internal add_low_entity_result AddLowEntity(game_state* GameState, entity_type Type, world_position WorldPos)
{
	Assert (GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32 EntityIndex = GameState->LowEntityCount++;
	low_entity* LowEntity = GameState->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Sim.Type = Type;
	LowEntity->Sim.Collision = GameState->NullVC;
	LowEntity->Pos = NullPosition();

	ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, LowEntity, WorldPos);

	add_low_entity_result Result = {};
	Result.Low = LowEntity;
	Result.LowIndex = EntityIndex;
	return Result;
}

internal add_low_entity_result AddGroundedEntity(game_state* GameState, entity_type Type, world_position Pos, sim_entity_collision_volume_group* Collision)
{
	add_low_entity_result Entity = AddLowEntity(GameState, Type, Pos);
	Entity.Low->Sim.Collision = Collision;
	return Entity;
}

inline world_position ChunkPositionFromTilePosition(world* World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ, v3 AdditionalOffset = V3(0,0,0))
{
	world_position BasePos = {};
	float TileSideInMeter = 1.4f;
	float TileDepthInMeter = 3.0f;
	v3 TileDim = V3(TileSideInMeter, TileSideInMeter, TileDepthInMeter);
	v3 Offset = Hadamard(TileDim, V3((float)AbsTileX, (float)AbsTileY, (float)AbsTileZ));
	world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);
	Assert(IsCanonical(World, Result.Offset));
	return Result;
}

internal add_low_entity_result AddWall (game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity (GameState, EntityType_Wall, WorldPos, GameState->WallVC);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
	
	return Entity;
}

internal add_low_entity_result AddStandardRoom(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity (GameState, EntityType_Space, WorldPos, GameState->StandardRoomVC);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);
	
	return Entity;
}

internal void InitHitPoints(low_entity* LowEntity, uint32 Count)
{
	Assert(Count <= ArrayCount(LowEntity->Sim.HitPoint));
	LowEntity->Sim.HitPointMax = Count;
	for(uint32 HitPointIndex = 0; HitPointIndex < LowEntity->Sim.HitPointMax; ++HitPointIndex)
	{
		hit_point* HitPoint = LowEntity->Sim.HitPoint + HitPointIndex;
		HitPoint->Flags = 0;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
	}
}

internal add_low_entity_result AddSword(game_state* GameState)
{
	add_low_entity_result Entity = AddLowEntity (GameState, EntityType_Sword, NullPosition());
	Entity.Low->Sim.Collision = GameState->SwordVC;
	AddFlags (&Entity.Low->Sim, EntityFlag_Moveable);
	
	return Entity;
}

internal add_low_entity_result AddPlayer (game_state* GameState)
{
	world_position WorldPos = GameState->CameraP;
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Hero, WorldPos, GameState->PlayerVC);
	AddFlags (&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	InitHitPoints(Entity.Low, 3);

	add_low_entity_result Sword = AddSword(GameState);
	Entity.Low->Sim.Sword.Index = Sword.LowIndex;
	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex = Entity.LowIndex;
	}
	return Entity;
}

internal add_low_entity_result AddMonster(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Monster, WorldPos, GameState->MonsterVC);
	AddFlags (&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	InitHitPoints(Entity.Low, 3);
	return Entity;
}

internal add_low_entity_result AddStairWell(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Stairwell, WorldPos, GameState->StairWellVC);
	AddFlags (&Entity.Low->Sim, EntityFlag_Collides);
	Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.XY;
	Entity.Low->Sim.WalkableHeight = GameState->TypicalFloorHeight;
	return Entity;
}

internal add_low_entity_result AddFamiliar(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Familiar, WorldPos, GameState->FamiliarVC);
	AddFlags (&Entity.Low->Sim, EntityFlag_Collides);
	return Entity;
}

internal void DrawHitPoints(sim_entity* Entity, render_group* Group)
{
	if(Entity->HitPointMax >= 1)
	{
		v2 HealthDim = {0.2f, 0.2f};
		float SpacingX = 1.5f * HealthDim.X;
		v2 HitP = {-0.5f * (Entity->HitPointMax - 1) * SpacingX, -0.25f};
		v2 dHitP = {SpacingX, 0.0f};
		for(uint32 HealthIndex = 0; HealthIndex < Entity->HitPointMax; ++HealthIndex)
		{
			hit_point* HitPoint = Entity->HitPoint + HealthIndex;
			v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
			if(HitPoint->FilledAmount == 0)
			{
				Color = {0.2f, 0.2f, 0.2f, 1.0f};
			}
			PushRect(Group, HitP, 0, HealthDim, Color, 0.0f);
			HitP += dHitP;
		}
	}
}

internal void ClearCollisionRule(game_state* GameState, uint32 StorageIndex)
{
	for(uint32 HashBucket = 0; HashBucket < ArrayCount(GameState->CollisionRuleHash); ++HashBucket)
	{
		for(pairwise_collision_rule** Rule = &GameState->CollisionRuleHash[HashBucket];
		*Rule;)
		{
			if(((*Rule)->StorageIndexA == StorageIndex) || ((*Rule)->StorageIndexB == StorageIndex))
			{
				pairwise_collision_rule* RemovedRule = *Rule;
				*Rule = (*Rule)->NextInHash;
				RemovedRule->NextInHash = GameState->FirstFreeCollisionRule;
				GameState->FirstFreeCollisionRule = RemovedRule;
			}
			else
			{
				Rule = &(*Rule)->NextInHash;
			}
		}
	}
}

internal void AddCollisionRule (game_state* GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool CanCollide)
{
	if(StorageIndexA > StorageIndexB)
	{
		uint32 Temp = StorageIndexA;
		StorageIndexA = StorageIndexB;
		StorageIndexB = Temp;
	}
	pairwise_collision_rule* Found = 0;
	uint32 HashBucket = StorageIndexA & (ArrayCount(GameState->CollisionRuleHash) - 1);
	for(pairwise_collision_rule* Rule = GameState->CollisionRuleHash[HashBucket];
		Rule; Rule = Rule->NextInHash)
	{
		if((Rule->StorageIndexA == StorageIndexA) &&
			(Rule->StorageIndexB == StorageIndexB))
		{
			Found = Rule;
			break;
		}
	}
	if(!Found)
	{
		Found = GameState->FirstFreeCollisionRule;
		if(Found)
		{
			GameState->FirstFreeCollisionRule = Found->NextInHash;
		}
		else
		{
			Found = PushStruct(&GameState->WorldArena, pairwise_collision_rule);
		}
		Found->NextInHash = GameState->CollisionRuleHash[HashBucket];
		GameState->CollisionRuleHash[HashBucket] = Found;
	}
	if(Found)
	{
		Found->StorageIndexA = StorageIndexA;
		Found->StorageIndexB = StorageIndexB;
		Found->CanCollide = CanCollide;
	}
}

sim_entity_collision_volume_group* MakeSimpleGroundedCollision(game_state* GameState, float DimX, float DimY, float DimZ)
{
	sim_entity_collision_volume_group* Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
	Group->VolumeCount = 1;
	Group->Volumes = PushArray(&GameState->WorldArena, Group->VolumeCount, sim_entity_collision_volume);
	Group->TotalVolume.OffsetPos = V3(0, 0, 0.5f * DimZ);
	Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
	Group->Volumes[0] = Group->TotalVolume;
	return Group;
}

sim_entity_collision_volume_group* MakeNullCollision(game_state* GameState)
{
	sim_entity_collision_volume_group* Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
	Group->VolumeCount = 0;
	Group->Volumes = 0;
	Group->TotalVolume.OffsetPos = V3(0, 0, 0);
	Group->TotalVolume.Dim = V3(0, 0, 0);
	return Group;
}

internal void FillGroundChunk(transient_state* TransientState, game_state* GameState, ground_buffer* GroundBuffer, world_position* ChunkP)
{
	loaded_bitmap* Bitmap = &GroundBuffer->Bitmap;
	GroundBuffer->Pos = *ChunkP;
	float Width = (float)Bitmap->Width;
	float Height = (float)Bitmap->Height;
	for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
	{
		for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
		{
			int32 ChunkX = ChunkP->ChunkX + ChunkOffsetX;
			int32 ChunkY = ChunkP->ChunkY + ChunkOffsetY;
			random_series Series = RandomSeed(183 * ChunkX + 843 * ChunkY + 453 * ChunkP->ChunkZ);
			v2 Center = 0.5f*V2(Width, Height) + V2(ChunkOffsetX * Width, -ChunkOffsetY * Height);
			for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
			{
				loaded_bitmap* Stamp;
				if(RandomChoice(&Series, 2))
				{
					Stamp = GameState->Grass + RandomChoice(&Series, ArrayCount(GameState->Grass));
				}
				else
				{
					Stamp = GameState->Stone + RandomChoice(&Series, ArrayCount(GameState->Stone));
				}
				float Radius = 5.0f;
				v2 BitmapCenter = 0.5f*V2i(Stamp->Width, Stamp->Height);
				v2 Offset = {Width * RandomUnilateral(&Series), Height * RandomUnilateral(&Series)};
				v2 Pos = Center + Offset - BitmapCenter;
				RenderBitMap(Bitmap, Stamp, Pos.X, Pos.Y);
			}
		}
	}

	for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
	{
		for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
		{
			int32 ChunkX = ChunkP->ChunkX + ChunkOffsetX;
			int32 ChunkY = ChunkP->ChunkY + ChunkOffsetY;
			random_series Series = RandomSeed(183 * ChunkX + 843 * ChunkY + 453 * ChunkP->ChunkZ);
			v2 Center = 0.5f*V2(Width, Height) + V2(ChunkOffsetX * Width, -ChunkOffsetY * Height);
			for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
			{
				loaded_bitmap* Stamp = GameState->Tuft + RandomChoice(&Series, ArrayCount(GameState->Tuft));
				float Radius = 5.0f;
				v2 BitmapCenter = 0.5f*V2i(Stamp->Width, Stamp->Height);
				v2 Offset = {Width * RandomUnilateral(&Series), Height * RandomUnilateral(&Series)};
				v2 Pos = Center + Offset - BitmapCenter;
				RenderBitMap(Bitmap, Stamp, Pos.X, Pos.Y);
			}
		}
	}
}

internal void ClearBitmap(loaded_bitmap* Bitmap)
{
	if(Bitmap->Memory)
	{
		int32 TotalBitmapSize = Bitmap->Width * Bitmap->Height * BITMAP_BYTES_PER_PIXEL;	
		ZeroSize(TotalBitmapSize, Bitmap->Memory);
	}
}
internal loaded_bitmap MakeEmptyBitmap(memory_arena* Arena, int32 Width, int32 Height, bool ClearToZero = true)
{
	loaded_bitmap Result = {};
	Result.Width = Width;
	Result.Height = Height;
	Result.Pitch = Result.Width * BITMAP_BYTES_PER_PIXEL;
	int32 TotalBitmapSize = Width * Height * BITMAP_BYTES_PER_PIXEL;
	Result.Memory = PushSize_(Arena, TotalBitmapSize);
	
	if(ClearToZero) {
		ClearBitmap(&Result);
	}

	return Result;
}

#if 0
internal void RequestGroundBuffers(world_position CenterPos, rectangle3 Bounds)
{
	Bounds = Offset(Bounds, CenterPos.Offset);
	CenterPos.Offset = V3(0,0,0);
	
	// FillGroundChunk(TransientState, GameState, TransientState->GroundBuffers, &GameState->CameraP);
}
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
	
	uint32 GroundBufferWidth = 256;
	uint32 GroundBufferHeight = 256;
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		GameState->MetersToPixels = 48.0f;
		GameState->PixelToMeters = 1.0f / GameState->MetersToPixels;
		GameState->TypicalFloorHeight = 3.0f;

		v3 WorldChunkInMeters = V3(GameState->PixelToMeters * (float)GroundBufferWidth, GameState->PixelToMeters * (float)GroundBufferWidth, GameState->TypicalFloorHeight);
		InitializeArena (&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));
		// Null Entity,
		
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		
		Initializeworld(World, WorldChunkInMeters);
		
		float TileSideInMeters = 1.4f;
		float TileDepthInMeters = GameState->TypicalFloorHeight;
		GameState->NullVC = MakeNullCollision(GameState);
		GameState->SwordVC = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
		GameState->StairWellVC = MakeSimpleGroundedCollision(GameState, TileSideInMeters, 
			2.0f * TileSideInMeters, 
			1.1f * TileDepthInMeters);
			GameState->PlayerVC = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
			GameState->MonsterVC = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
			GameState->FamiliarVC = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
			GameState->WallVC = MakeSimpleGroundedCollision(GameState, TileSideInMeters, TileSideInMeters, TileDepthInMeters);
			GameState->StandardRoomVC = MakeSimpleGroundedCollision(GameState, TilesPerWidth * TileSideInMeters, TilesPerHeight * TileSideInMeters, 0.9f * TileDepthInMeters);
				
		AddLowEntity (GameState, EntityType_None, NullPosition());

		GameState->Grass[0] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/grass00.bmp");
		GameState->Grass[1] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/grass01.bmp");
		GameState->Tuft[0] = DEBUGLoadBMP (Thread, Memory->	DEBUGReadEntireFile, "test2/tuft00.bmp");
		GameState->Tuft[1] = DEBUGLoadBMP (Thread, Memory->	DEBUGReadEntireFile, "test2/tuft01.bmp");
		GameState->Tuft[2] = DEBUGLoadBMP (Thread, Memory->	DEBUGReadEntireFile, "test2/tuft02.bmp");
		GameState->Stone[0] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/ground00.bmp");
		GameState->Stone[1] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/ground01.bmp");
		GameState->Stone[2] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/ground02.bmp");
		GameState->Stone[3] = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/ground03.bmp");

		GameState->Backdrop = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_background.bmp");
		GameState->Shadow = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_hero_shadow.bmp");
		GameState->Tree = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/tree00.bmp");
		GameState->Sword = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/rock03.bmp");
		GameState->Stairwell = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test2/rock02.bmp");
		hero_bitmaps *Bitmap = GameState->HeroBitmaps;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->Align = V2(72, 182);
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->Align = V2(72, 182);
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->Align = V2(72, 182);
		++Bitmap;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->Align = V2(72, 182);		
		
		random_series Series = RandomSeed(1234);
		uint32 ScreenBaseX = 0;
		uint32 ScreenBaseY = 0;
		uint32 ScreenBaseZ = 0;
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;

		bool DoorLeft = false;
		bool DoorRight = false;
		bool DoorTop = false;
		bool DoorBottom = false;
		bool DoorUp = false;
		bool DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 2000; ++ScreenIndex)
		{
			// uint32 DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown) ? 2 : 3);
			uint32 DoorDirection = RandomChoice(&Series, 2);
			bool CreatedZDoor = false;
			if(DoorDirection == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == ScreenBaseZ)	
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
				
			}
			else if(DoorDirection == 1)
			{
				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}
			
			AddStandardRoom (GameState, ScreenX*TilesPerWidth + TilesPerWidth/2, ScreenY*TilesPerHeight + TilesPerHeight/2, AbsTileZ);

			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;					
					bool ShouldBeDoor = false;
					if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
					{
						ShouldBeDoor = true;
					}
					if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
					{
						ShouldBeDoor = true;
					}
					if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
					{
						ShouldBeDoor = true;
					}
					if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
					{
						ShouldBeDoor = true;
					}
					if(ShouldBeDoor)
					{
						AddWall (GameState, AbsTileX, AbsTileY, AbsTileZ);
					}
					else if (CreatedZDoor)
					{
						if((TileX == 10) && (TileY == 5))
						{
							AddStairWell (GameState, AbsTileX, AbsTileY, DoorDown ? AbsTileZ -1 : AbsTileZ);
						}
					}
				}
			}
			DoorLeft = DoorRight;
			DoorBottom = DoorTop;
			if(CreatedZDoor)
			{
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}
			
			DoorRight = false;
			DoorTop = false;
			
			if(DoorDirection == 2)
			{
				//AbsTileZ = 1 - AbsTileZ;	// AbsTileZ is 0 it goes to 1 otherwise, vice versa
				if (AbsTileZ == ScreenBaseZ)
				{
					AbsTileZ = ScreenBaseZ + 1;
				}
				else
				{
					AbsTileZ = ScreenBaseZ;
				}
			}
			else if(DoorDirection == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
			
		}
// #if RAAVANAN_INTERNAL
#if RAAVANAN_INTERNAL	
		char *Filename = __FILE__;
		debug_read_file_result File = Memory->DEBUGReadEntireFile(Thread, Filename);
		if(File.Content)
		{
			Memory->DEBUGWriteEntireFile(Thread, "test.out", File.ContentSize, File.Content);
			Memory->DEBUGFreeFileMemory(Thread, File.Content);
		}
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;
		GameState->PlayerX = GameState->PlayerY = 100;
#endif
#if 0
		while(GameState->LowEntityCount < (ArrayCount(GameState->LowEntities) - 16))
		{
			uint32 Coordinate = 1024 + GameState->LowEntityCount;
			AddWall(GameState, Coordinate, Coordinate, Coordinate);
		}
#endif
		world_position NewCameraP = {};
		uint32 CameraTileX = ScreenBaseX * TilesPerWidth + 17/2;
		uint32 CameraTileY = ScreenBaseY * TilesPerHeight + 9/2;
		uint32 CameraTileZ = ScreenBaseZ;
		NewCameraP = ChunkPositionFromTilePosition(GameState->World, CameraTileX , CameraTileY, CameraTileZ);
		GameState->CameraP = NewCameraP;
		AddMonster(GameState, CameraTileX - 2, CameraTileY + 2, CameraTileZ);
		for(int FamiliarIndex = 0; FamiliarIndex < 1; ++FamiliarIndex)
		{
			int32 FamiliarOffsetX = RandomBetween(&Series, -7, 7);
			int32 FamiliarOffsetY = RandomBetween(&Series, -3, -1);
			if((FamiliarOffsetX != 0) || (FamiliarOffsetY != 0))
			{
				AddFamiliar(GameState, CameraTileX + FamiliarOffsetX, CameraTileY + FamiliarOffsetY, CameraTileZ);
			}
		}		
		
		Memory->IsInitialized = true;
	}
	// Transient Initialization
	Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
	transient_state *TransientState = (transient_state *)Memory->TransientStorage;
	if(!TransientState->IsInitialized)
	{
		InitializeArena (&TransientState->TransientArena, Memory->TransientStorageSize - sizeof(transient_state), (uint8 *)Memory->TransientStorage + sizeof(transient_state));
		
		TransientState->GroundBufferCount = 64;
		TransientState->GroundBuffers = PushArray(&TransientState->TransientArena, TransientState->GroundBufferCount, ground_buffer);
		for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TransientState->GroundBufferCount; ++GroundBufferIndex)
		{
			ground_buffer* GroundBuffer = TransientState->GroundBuffers + GroundBufferIndex;
			GroundBuffer->Bitmap =  MakeEmptyBitmap(&TransientState->TransientArena, GroundBufferWidth, GroundBufferHeight, false);
			GroundBuffer->Pos = NullPosition();	
		}
		
		TransientState->IsInitialized = true;
	}

	if(Input->ExecutableReloaded)
	{
		for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TransientState->GroundBufferCount; ++GroundBufferIndex)
		{
			ground_buffer* GroundBuffer = TransientState->GroundBuffers + GroundBufferIndex;
			GroundBuffer->Pos = NullPosition();
		}
	}
	
	world *World = GameState->World;
	// Tile width and Height
	float MeterToPixels = GameState->MetersToPixels;
	float PixelToMeters = 1.0f / MeterToPixels;
	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		controlled_hero* ControlledHero = GameState->ControlledHeros + ControllerIndex;
		if (ControlledHero->EntityIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
				*ControlledHero = {};
				ControlledHero->EntityIndex = AddPlayer(GameState).LowIndex;
			}
		}
		else
		{
			ControlledHero->dZ = 0.0f;
			ControlledHero->ddPlayer = {};
			ControlledHero->dSword = {};
			if(Controller->IsAnalog)
			{
#if RAAVANAN_INTERNAL
				GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
				GameState->YOffset += (int)(4.0f * (Controller->StickAverageX));
#endif
				ControlledHero->ddPlayer = V2(Controller->StickAverageX, Controller->StickAverageY);
			}
			else
			{
				if(Controller->MoveUp.EndedDown)
				{
					ControlledHero->ddPlayer.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ControlledHero->ddPlayer.Y = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ControlledHero->ddPlayer.X = 1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ControlledHero->ddPlayer.X = -1.0f;
				}
			}
			
			if (Controller->Start.EndedDown)
			{
				ControlledHero->dZ = 3.0f;
			}
			ControlledHero->dSword = {};
			if(Controller->ActionUp.EndedDown)
			{
				ControlledHero->dSword = V2(0.0f, 1.0f);
			}
			if(Controller->ActionDown.EndedDown)
			{
				ControlledHero->dSword = V2(0.0f, -1.0f);
			}
			if(Controller->ActionLeft.EndedDown)
			{
				ControlledHero->dSword = V2(-1.0f, 0.0f);
			}
			if(Controller->ActionRight.EndedDown)
			{
				ControlledHero->dSword = V2(1.0f, 0.0f);
			}
		}
	}
	
	temporary_memory RenderMemory = BeginTemporaryMemory(&TransientState->TransientArena);
	render_group* RenderGroup = AllocateRenderGroup(&TransientState->TransientArena, Megabytes(4), GameState->MetersToPixels);
	loaded_bitmap RenderBuffer_ = {};
	loaded_bitmap* RenderBuffer = &RenderBuffer_;
	RenderBuffer->Width = Buffer->Width;
	RenderBuffer->Height = Buffer->Height;
	RenderBuffer->Pitch = Buffer->Pitch;
	RenderBuffer->Memory = Buffer->Memory;

	RenderRectangle(RenderBuffer, V2(0.0f, 0.0f), V2((float)RenderBuffer->Width, (float)RenderBuffer->Height), 0.5f, 0.5f, 0.5f);

	float ScreenWidthInMeters = RenderBuffer->Width * PixelToMeters;
	float ScreenHeightInMeters = RenderBuffer->Height * PixelToMeters;

	rectangle3 CameraBoundsInMeters = RectCenterDim(V3(0,0,0), V3(ScreenWidthInMeters, ScreenHeightInMeters, 0));
	
	v2 ScreenCenter = V2(0.5f * (float)RenderBuffer->Width, 0.5f * (float)RenderBuffer->Height);
	
	for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TransientState->GroundBufferCount; ++GroundBufferIndex)
	{
		ground_buffer* GroundBuffer = TransientState->GroundBuffers + GroundBufferIndex;
		if(IsWorldPosValid(GroundBuffer->Pos))
		{
			loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
			v3 Delta =  Subtract(GameState->World, &GroundBuffer->Pos, &GameState->CameraP);
			PushBitmap (RenderGroup, Bitmap, Delta.XY, Delta.Z, 0.5f * V2i(Bitmap->Width, Bitmap->Height));
		}
	}
	
	{
		world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
		world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));
	
		for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
		{
			for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
			{
				for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
				{
					// world_chunk* Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
					// if(Chunk)
					{
						world_position ChunkCenterPos = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
						v3 RelativePos = Subtract(GameState->World, &ChunkCenterPos, &GameState->CameraP);
						v2 ScreenPos = V2(ScreenCenter.X + MeterToPixels * RelativePos.X, ScreenCenter.Y - MeterToPixels * RelativePos.Y);
						v2 ScreenDim = GameState->MetersToPixels * World->ChunkDimInMeters.XY;

						float FurthesetBufferLengthSq = 0;
						ground_buffer* FurthesetBuffer = 0;
						for (uint32 GroundBufferIndex = 0; GroundBufferIndex < TransientState->GroundBufferCount; ++GroundBufferIndex)
						{
							ground_buffer* GroundBuffer = TransientState->GroundBuffers + GroundBufferIndex;
							if(AreInSameChunk(GameState->World, &GroundBuffer->Pos, &ChunkCenterPos))
							{
								FurthesetBuffer = 0;
								break;
							}
							else if(IsWorldPosValid(GroundBuffer->Pos))
							{
								RelativePos = Subtract(GameState->World, &GroundBuffer->Pos, &GameState->CameraP);
								float BufferLengthSq = LengthSq(RelativePos.XY);
								if(FurthesetBufferLengthSq < BufferLengthSq)
								{
									FurthesetBufferLengthSq = BufferLengthSq;
									FurthesetBuffer = GroundBuffer;
								}
							}
							else
							{
								FurthesetBufferLengthSq = FLOATMAX;
								FurthesetBuffer = GroundBuffer;
							}
						}
						if(FurthesetBuffer)
						{
							FillGroundChunk(TransientState, GameState, FurthesetBuffer, &ChunkCenterPos);
						}
						RenderRectOutline(RenderBuffer, ScreenPos - 0.5f * ScreenDim, ScreenPos + 0.5f * ScreenDim, V3(1.0f, 1.0f, 0.0f));
					}
				}
			}
		}
	}
	
	v3 SimBoundsExpansion = V3(15.0f, 15.0f, 15.0f);
	rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
	
	temporary_memory SimMemory = BeginTemporaryMemory(&TransientState->TransientArena);
	sim_region* SimRegion = BeginSim(&TransientState->TransientArena, GameState, GameState->World, GameState->CameraP, SimBounds, Input->deltaTime);
	
	// char TextBuffer[256];
	// sprintf_s(TextBuffer, "SimRegion->EntityCount:%d\n", SimRegion->EntityCount);
	// OutputDebugStringA(TextBuffer);
	for (uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
	{
		sim_entity* Entity = SimRegion->Entities + EntityIndex;
		if(Entity->Updatable)
		{
			float dt = Input->deltaTime;
			
			float ShadowAlpha = 1.0f - 0.5f * Entity->Pos.Z;
			ShadowAlpha = (ShadowAlpha < 0.0f) ? 0.0f : ShadowAlpha;
			
			move_spec MoveSpec = DefaultMoveSpec(); 
			v3 ddPlayer = {};

			render_basis* Basis = PushStruct(&TransientState->TransientArena, render_basis);
			RenderGroup->DefaultBasis = Basis;

			hero_bitmaps *HeroBitsmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
			switch (Entity->Type)
			{
				case EntityType_Hero:
				{
					for(uint32 ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeros); ++ControlIndex)
					{
						controlled_hero* ConHero = GameState->ControlledHeros + ControlIndex;
						if(Entity->StorageIndex == ConHero->EntityIndex)
						{
							if(ConHero->dZ != 0.0f)
							{
								Entity->dPlayerP.Z = ConHero->dZ;
							}

							MoveSpec.UnitMaxAccelVector = true;
							MoveSpec.Speed = 50.0f;
							MoveSpec.Drag = 8.0f;
							ddPlayer = V3(ConHero->ddPlayer, 0.0f);
							
							if((ConHero->dSword.X != 0) || (ConHero->dSword.Y != 0))
							{
								sim_entity* Sword = Entity->Sword.Ptr;
								if(Sword && IsSet(Sword, EntityFlag_NonSpatial))
								{
									Sword->DistanceLimit = 5.f;
									MakeEntitySpatial(Sword, Entity->Pos, 5.0f * V3(ConHero->dSword, 0.0f));
									AddCollisionRule(GameState, Sword->StorageIndex, Entity->StorageIndex, false);
								}
							}
						}
					}
					
					PushBitmap(RenderGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(RenderGroup, &HeroBitsmaps->Torso, V2(0,0), 0, HeroBitsmaps->Align);
					PushBitmap(RenderGroup, &HeroBitsmaps->Cape, V2(0,0), 0, HeroBitsmaps->Align);
					PushBitmap(RenderGroup, &HeroBitsmaps->Head, V2(0,0), 0, HeroBitsmaps->Align);
					DrawHitPoints (Entity, RenderGroup);
				}
				break;
				case EntityType_Wall:
					{
					#if 0
					RenderRectangle(RenderBuffer, PlayerLeftTop, PlayerLeftTop + MeterToPixels * 0.9f * EntiryWidthHeight, PlayerR, PlayerG, PlayerB);	
					#else
					PushBitmap(RenderGroup, &GameState->Tree, V2(0,0), 0, V2(40,80));
					#endif
					}
				break;
				case EntityType_Sword:
					{
						MoveSpec.UnitMaxAccelVector = false;
						MoveSpec.Speed = 0.0f;
						MoveSpec.Drag = 0.0f;
						if(Entity->DistanceLimit == 0.0f)
						{
							ClearCollisionRule(GameState, Entity->StorageIndex);
							MakeEntityNonSpatial(Entity);
						}
						PushBitmap(RenderGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
						PushBitmap(RenderGroup, &GameState->Sword, V2(0,0), 0, V2(30,10));
					}
				break;
				case EntityType_Stairwell:
				{
					// PushBitmap(RenderGroup, &GameState->Stairwell, V2(0,0), 0, V2(37,37));
					PushRect(RenderGroup, V2(0,0), 0, Entity->WalkableDim, V4(1, 0.5f, 0, 1), 0.0f);
					PushRect(RenderGroup, V2(0,0), Entity->WalkableHeight, Entity->WalkableDim, V4(1, 1, 0, 1), 0.0f);
				}
				break;
				case EntityType_Familiar:
				{
					sim_entity* ClosestHero = 0;
					float ClosestDstSq = Square(10.0f);
					sim_entity* TestEntity = SimRegion->Entities;
					for(uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity)
					{
						if(TestEntity->Type == EntityType_Hero)
						{
							float TestDstSq = LengthSq(TestEntity->Pos - Entity->Pos);
							if(ClosestDstSq > TestDstSq)
							{
								ClosestHero = TestEntity;
								ClosestDstSq = TestDstSq;
							}
						}
					}
					if(ClosestHero && ClosestDstSq > Square(3.0f))
					{
						float Acceleration = 0.5f;
						float OneOverLength = Acceleration / SquareRoot(ClosestDstSq);
						ddPlayer = OneOverLength * (ClosestHero->Pos - Entity->Pos);
					}
					MoveSpec.UnitMaxAccelVector = true;
					MoveSpec.Speed = 50.0f;
					MoveSpec.Drag = 8.0f;
					
					Entity->tBob += dt;
					if(Entity->tBob > (2.0f * PI))
					{
						Entity->tBob -= (2.0f * PI);
					}
					float SinValue = Sin(2.0f * Entity->tBob);
					PushBitmap(RenderGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, SinValue * (0.5f * ShadowAlpha) - (0.2f * SinValue), 0.0f);
					PushBitmap(RenderGroup, &HeroBitsmaps->Head, V2(0,0), 0.25f * SinValue, HeroBitsmaps->Align);
				}
				break;
				case EntityType_Monster:
				{
					PushBitmap(RenderGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(RenderGroup, &HeroBitsmaps->Torso, V2(0,0), 0, HeroBitsmaps->Align);
					DrawHitPoints (Entity, RenderGroup);
				}
				break;
				case EntityType_Space:
				{
					#if 0
					for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
					{
						sim_entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
						PushRectOutline(RenderGroup, Volume->OffsetPos.XY, 0, Volume->Dim.XY, V4(1, 0.5f, 0, 1), 0.0f);
					}
					#endif
				}
				break;
				default:
				{
					InvalidCodePath;
				}
				break;
			}
			if(!IsSet(Entity, EntityFlag_NonSpatial) && IsSet(Entity, EntityFlag_Moveable))
			{
				MoveEntity(GameState, SimRegion, Entity, Input->deltaTime, &MoveSpec, ddPlayer);
			}
			Basis->Pos = GetEntityGroundPoint(Entity);
		}
	}
	
#if 0
	v2 PlayerLeftTop = {EntityGroundPointX - 0.5f * MeterToPixels * LowEntity->Sim.Width, EntityGroundPointY - 0.5f * MeterToPixels * LowEntity->Sim.Height};
	v2 EntiryWidthHeight = {LowEntity->Sim.Width, LowEntity->Sim.Height};
#endif
	for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
	{
		entity_visible_piece* Piece = (entity_visible_piece *)(RenderGroup->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(entity_visible_piece);
		v3 EntityBaseP = Piece->Basis->Pos;
		float ZFudge = (1.0f + 0.1f * (EntityBaseP.Z + Piece->OffsetZ));
		float EntityGroundPointX = ScreenCenter.X + MeterToPixels * ZFudge * EntityBaseP.X;
		float EntityGroundPointY = ScreenCenter.Y - MeterToPixels * ZFudge * EntityBaseP.Y;
		float EntityZ = -MeterToPixels * EntityBaseP.Z;
		
		v2 Center = v2{EntityGroundPointX + Piece->Offset.X,
						EntityGroundPointY + Piece->Offset.Y + Piece->EntityZCofficient * EntityZ};
		if(Piece->Bitmap)
		{
			RenderBitMap(RenderBuffer, Piece->Bitmap, Center.X, Center.Y, Piece->A);
		}
		else
		{
			v2 HalfDim = MeterToPixels*0.5f * Piece->Dim;
			RenderRectangle(RenderBuffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
		}
	}
	EndSim(SimRegion, GameState);
	EndTemporaryMemory(SimMemory);
	EndTemporaryMemory(RenderMemory);

	CheckArena(&GameState->WorldArena);
	CheckArena(&TransientState->TransientArena);
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
