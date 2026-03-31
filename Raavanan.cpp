
#include "Raavanan_random.h"
#include "Raavanan_world.cpp"
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

internal void RenderRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, float R, float G, float B)
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
	uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	
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

internal void RenderBitMap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, float realX, float realY, float CAlpha = 1.0f)
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

	uint32 *SrcRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	SrcRow += -SrcOffsetY * Bitmap->Width + SrcOffsetX; 
	uint8 *DstRow = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
	for (int y = MinY; y < MaxY; ++y)
	{
		uint32 *Dst = (uint32 *)DstRow;
		uint32 *Src = SrcRow;
		for (int x = MinX; x < MaxX; ++x)
		{
			float A = (float)((*Src >> 24) & 0xFF) / 255.0f;
			A *= CAlpha;
			float SR = (float)((*Src >> 16) & 0xFF);
			float SG = (float)((*Src >> 8) & 0xFF);
			float SB = (float)((*Src >> 0) & 0xFF);

			float DR = (float)((*Dst >> 16) & 0xFF);
			float DG = (float)((*Dst >> 8) & 0xFF);
			float DB = (float)((*Dst >> 0) & 0xFF);

			float R = (1 - A) * DR + A * SR;
			float G = (1 - A) * DG + A * SG;
			float B = (1 - A) * DB + A * SB;

			*Dst = (((uint32)(R + 0.5f) << 16) |
					((uint32)(G + 0.5f) << 8) |
					((uint32)(B + 0.5f) << 0));
			++Dst;
			++Src;
		}
		DstRow += Buffer->Pitch;
		SrcRow -= Bitmap->Width;
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
		Result.Pixels = Pixels;
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

        int32 RedShift = 16 - (int32)RedScan.Index;
        int32 GreenShift = 8 - (int32)GreenScan.Index;
        int32 BlueShift = 0 - (int32)BlueScan.Index;
        int32 AlphaShift = 24 - (int32)AlphaScan.Index;
        
        uint32 *SourceDest = Pixels;
        for(int32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(int32 X = 0;
                X < Header->Width;
                ++X)
            {
                uint32 C = *SourceDest;

                *SourceDest++ = (RotateLeft(C & RedMask, RedShift) |
                                 RotateLeft(C & GreenMask, GreenShift) |
                                 RotateLeft(C & BlueMask, BlueShift) |
                                 RotateLeft(C & AlphaMask, AlphaShift));
            }
        }
	}
	return Result;
}
//#endif

inline v2 GetCameraSpaceP(game_state* GameState, low_entity* EntityLow)
{
	world_difference Diff = Subtract(GameState->World, &EntityLow->Pos, &GameState->CameraP);
	v2 Result = Diff.dXY;
	return Result;
}

inline high_entity* MakeEntityHighFrequency (game_state* GameState, low_entity* EntityLow, uint32 LowIndex, v2 CameraSpaceP)
{	
	high_entity* EntityHigh = 0;
	Assert(EntityLow->HighEntityIndex == 0);
	if(EntityLow->HighEntityIndex == 0)
	{
		if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities))
		{
			uint32 HighIndex = GameState->HighEntityCount++;
			EntityHigh = GameState->HighEntities + HighIndex;
			EntityHigh->Pos = CameraSpaceP;
			EntityHigh->dPlayerP = V2(0,0);
			EntityHigh->ChunkZ = EntityLow->Pos.ChunkZ;
			EntityHigh->FacingDirection = 0;
			EntityHigh->LowEntityIndex = LowIndex;

			EntityLow->HighEntityIndex = HighIndex;
		}
		else
		{
			InvalidCodePath;
		}
	}
	return EntityHigh;
}

inline high_entity* MakeEntityHighFrequency (game_state* GameState, uint32 LowIndex)
{
	high_entity* EntityHigh = 0;
	low_entity* EntityLow = &GameState->LowEntities[LowIndex];
	if (EntityLow->HighEntityIndex)
	{
		EntityHigh = GameState->HighEntities + EntityLow->HighEntityIndex;
	}
	else
	{
		v2 CameraSpaceP = GetCameraSpaceP(GameState, EntityLow);
		EntityHigh = MakeEntityHighFrequency(GameState, EntityLow, LowIndex, CameraSpaceP);
	}
	
	return EntityHigh;
}

inline entity ForceEntityIntoHigh(game_state* GameState, uint32 Index)
{
	entity Result = {};
	if ((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result.LowIndex = Index;
		Result.Low = GameState->LowEntities + Index;
		Result.High = MakeEntityHighFrequency (GameState, Index);
	}
	return Result;
}

internal void MakeEntityLowFrequency (game_state* GameState, uint32 LowIndex)
{
	low_entity* EntityLow = &GameState->LowEntities[LowIndex];
	uint32 HighIndex = EntityLow->HighEntityIndex;
	if (HighIndex)
	{
		uint32 LastHighIndex = GameState->HighEntityCount - 1;
		if (HighIndex != LastHighIndex)
		{
			high_entity* LastEntity = GameState->HighEntities + LastHighIndex;
			high_entity* DelEntity = GameState->HighEntities + HighIndex;
			*DelEntity = *LastEntity;
			GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
		}
		--GameState->HighEntityCount;
		EntityLow->HighEntityIndex = 0;
	}
}

inline low_entity* GetLowEntity(game_state* GameState, uint32 Index)
{
	low_entity* Result = 0;
	if ((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result = GameState->LowEntities + Index;
	}
	return Result;
}

inline bool ValidateEntityPairs(game_state* GameState)
{
	bool Valid = true;
	for (uint32 EntityIndex = 1; EntityIndex < GameState->HighEntityCount; ++EntityIndex)
	{
		high_entity* High = GameState->HighEntities + EntityIndex;
		Valid = Valid && (GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == EntityIndex);
	}
	return Valid;
}

internal void OffsetAndCheckFrequencyByArea(game_state* GameState, v2 Offset, rectangle2 HighFreqBounds)
{
	for (uint32 EntityIndex = 1; EntityIndex < GameState->HighEntityCount; )
	{
		high_entity* High = GameState->HighEntities + EntityIndex;
		High->Pos += Offset;
		if(IsInRectangle(HighFreqBounds, High->Pos))
		{
			EntityIndex++;
		}
		else
		{
			Assert(GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == EntityIndex);
			MakeEntityLowFrequency(GameState, High->LowEntityIndex);
		}
	}
}

internal add_low_entity_result AddLowEntity(game_state* GameState, entity_type Type, world_position* WorldPos)
{
	Assert (GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32 EntityIndex = GameState->LowEntityCount++;
	low_entity* LowEntity = GameState->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Type = Type;

	ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, LowEntity, 0, WorldPos);

	add_low_entity_result Result = {};
	Result.Low = LowEntity;
	Result.LowIndex = EntityIndex;
	return Result;
}

internal void InitHitPoints(low_entity* LowEntity, uint32 Count)
{
	Assert(Count < ArrayCount(LowEntity->HitPoint));
	LowEntity->HitPointMax = Count;
	LowEntity->HitPoint[Count-1].FilledAmount = HIT_POINT_SUB_COUNT;
	for(uint32 HitPointIndex = 0; HitPointIndex < Count-1; ++HitPointIndex)
	{
		hit_point* HitPoint = LowEntity->HitPoint + HitPointIndex;
		HitPoint->Flags = 0;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
	}
}

internal add_low_entity_result AddSword(game_state* GameState)
{
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, 0);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = false;
	return Entity;
}

internal add_low_entity_result AddPlayer (game_state* GameState)
{
	world_position WorldPos = GameState->CameraP;
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &WorldPos);
	InitHitPoints(Entity.Low, 3);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = true;

	add_low_entity_result Sword = AddSword(GameState);
	Entity.Low->SwordLowIndex = Sword.LowIndex;
	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex = Entity.LowIndex;
	}
	return Entity;
}

internal add_low_entity_result AddMonster(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Monster, &WorldPos);
	InitHitPoints(Entity.Low, 5);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = true;
	return Entity;
}

internal add_low_entity_result AddFamiliar(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &WorldPos);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = false;
	return Entity;
}

internal add_low_entity_result AddWall (game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	world_position WorldPos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity (GameState, EntityType_Wall, &WorldPos);
	Entity.Low->Pos = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	Entity.Low->Height = GameState->World->TileSideInMeters;
	Entity.Low->Width = Entity.Low->Height;
	Entity.Low->Collides = true;
	
	return Entity;
}

internal bool TestWall (float WallX, float RelX, float RelY, float PlayerDeltaX, float PlayerDeltaY,
						float *tMin, float MinY, float MaxY)
{
	bool Hit = false;
	float tEpsilon = 0.0001f;
	if(PlayerDeltaX != 0.0f)
	{
		float tResult = (WallX - RelX) / PlayerDeltaX;
		float Y = RelY + tResult * PlayerDeltaY;
		if((tResult >= 0.0f) && (*tMin > tResult))
		{	
			if((Y >= MinY) && (Y <= MaxY))
			{
				*tMin = Maximum(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}
	return Hit;
}
internal void MoveEntity (game_state* GameState, entity Entity, float deltaTime, v2 ddPlayer)
{
	world* World = GameState->World;
	float ddPLength = LengthSq(ddPlayer);
	if(ddPLength > 1.0f)
	{
		ddPlayer *= (1.0f / SquareRoot(ddPLength));
	}

	float PlayerSpeed = 50.0f;
	ddPlayer *= PlayerSpeed;

	ddPlayer += -8.0f * Entity.High->dPlayerP;

	v2 OldPlayerP = Entity.High->Pos;
	v2 PlayerDelta = 0.5f * ddPlayer * Square(deltaTime) + Entity.High->dPlayerP * deltaTime;
	Entity.High->dPlayerP = ddPlayer * deltaTime + Entity.High->dPlayerP;
	v2 NewPlayerP = OldPlayerP + PlayerDelta;

	for (uint32 Iteration = 0; (Iteration < 4); ++Iteration)
	{
		float tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitHighEntityIndex = 0;
		v2 DesiredPos = Entity.High->Pos + PlayerDelta;

		for (uint32 TestHighEntityIndex = 1; TestHighEntityIndex < GameState->HighEntityCount; ++TestHighEntityIndex)
		{
			if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
			{
				entity TestEntity;
				TestEntity.High = GameState->HighEntities + TestHighEntityIndex;
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;

				if(TestEntity.Low->Collides)
				{
					float DiameterW = TestEntity.Low->Width + Entity.Low->Width;
					float DiameterH = TestEntity.Low->Height + Entity.Low->Height;
					v2 MinCorner = -0.5f * v2 {DiameterW, DiameterH};
					v2 MaxCorner = 0.5f * v2 {DiameterW, DiameterH};

					v2 Rel = Entity.High->Pos - TestEntity.High->Pos;
					
					if(TestWall (MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = V2(-1,0);
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = V2(1,0);
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = V2(0,-1);
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall (MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
					{
						WallNormal = V2(0,1);
						HitHighEntityIndex = TestHighEntityIndex;
					}
				}
			}			
		}
		Entity.High->Pos += tMin * PlayerDelta;
		if(HitHighEntityIndex)
		{
			Entity.High->dPlayerP = Entity.High->dPlayerP - 1 * Dot(Entity.High->dPlayerP, WallNormal) * WallNormal;
			PlayerDelta = DesiredPos - Entity.High->Pos;
			PlayerDelta = PlayerDelta - 1 * Dot(PlayerDelta, WallNormal) * WallNormal;

			high_entity* HitHigh = GameState->HighEntities + HitHighEntityIndex;
			low_entity* HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
			// Entity.High->ChunkZ += HitLow->dAbsTileZ;
		}
		else
		{
			break;
		}		
	}


	if((Entity.High->dPlayerP.X == 0.0f) && (Entity.High->dPlayerP.Y == 0.0f))
	{

	}
	else if(AbsoluteValue(Entity.High->dPlayerP.X) > AbsoluteValue(Entity.High->dPlayerP.Y))
	{
		if(Entity.High->dPlayerP.X > 0)
		{
			Entity.High->FacingDirection = 0;
		}
		else
		{
			Entity.High->FacingDirection = 2;
		}
	}
	else
	{
		if(Entity.High->dPlayerP.Y > 0)
		{
			Entity.High->FacingDirection = 1;
		}
		else
		{
			Entity.High->FacingDirection = 3;
		}
	}
	world_position NewPos = MapIntoChunkSpace (GameState->World, GameState->CameraP, Entity.High->Pos);
	
	ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex, Entity.Low, &Entity.Low->Pos, &NewPos);
}

internal void SetCamera (game_state* GameState, world_position NewCameraP)
{
	world* World = GameState->World;
	Assert(ValidateEntityPairs(GameState));
	world_difference dCameraP = Subtract (World, &NewCameraP, &GameState->CameraP);
	GameState->CameraP = NewCameraP;
	uint32 TileSpanX = 17 * 3;
	uint32 TileSpanY = 9 * 3;
	rectangle2 CameraBounds = RectCenterDim(v2{0,0}, World->TileSideInMeters * v2 {(float)TileSpanX, (float)TileSpanY});
	v2 EntityOffsetForFrame = -dCameraP.dXY;
	OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);
	Assert(ValidateEntityPairs(GameState));
	world_position MinChunkP = MapIntoChunkSpace(GameState->World, NewCameraP, GetMinCorner(CameraBounds));
	world_position MaxChunkP = MapIntoChunkSpace(GameState->World, NewCameraP, GetMaxCorner(CameraBounds));
	for(int32 ChunkY = MinChunkP.ChunkY; ChunkY < MaxChunkP.ChunkY; ++ChunkY)
	{
		for(int32 ChunkX = MinChunkP.ChunkX; ChunkX < MaxChunkP.ChunkX; ++ChunkX)
		{
			world_chunk* Chunk = GetWorldChunk(GameState->World, ChunkX, ChunkY, NewCameraP.ChunkZ);
			if(Chunk)
			{
				for(world_entity_block* Block = &Chunk->FirstBlock; Block; Block = Block->Next)
				{
					for (uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
					{
						uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
						low_entity* Low = GameState->LowEntities + LowEntityIndex;
						if (Low->HighEntityIndex == 0)
						{
							v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
							if(IsInRectangle(CameraBounds, CameraSpaceP))
							{
								MakeEntityHighFrequency (GameState, Low, LowEntityIndex, CameraSpaceP);
							}
						}
					}
				}
			}
		}
	}
	Assert(ValidateEntityPairs(GameState));
}

internal void PushPiece(entity_visible_piece_group* Group, loaded_bitmap* Bitmap, v2 Offset, float OffsetZ, v2 Align, 
						v2 Dim, v4 Color, float EntityZCofficient)
{
	Assert(Group->PieceCount < ArrayCount(Group->Pieces));
	entity_visible_piece* Piece = Group->Pieces + Group->PieceCount++;
	Piece->Bitmap = Bitmap;
	Piece->Offset = Group->GameState->MetersToPixel * V2(Offset.X, -Offset.Y) - Align;
	Piece->OffsetZ = Group->GameState->MetersToPixel * OffsetZ;
	Piece->EntityZCofficient = EntityZCofficient;
	Piece->R = Color.R;
	Piece->G = Color.G;
	Piece->B = Color.B;
	Piece->A = Color.A;
	Piece->Dim = Dim;
}

internal void PushBitmap(entity_visible_piece_group* Group, loaded_bitmap* Bitmap, v2 Offset, float OffsetZ, v2 Align, float Alpha = 1.0f, float EntityZCofficient = 1.0f)
{
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZCofficient);
}

internal void PushRect(entity_visible_piece_group* Group, v2 Offset, float OffsetZ, v2 Dim, v4 Color, float EntityZCofficient = 1.0f)
{
	PushPiece(Group, 0, Offset, OffsetZ, V2(0,0), Dim, Color, EntityZCofficient);
}

internal entity EntityFromHighIndex(game_state* GameState, uint32 HighEntityIndex)
{
	entity Result = {};
	if(HighEntityIndex)
	{
		Assert(HighEntityIndex < ArrayCount(GameState->HighEntities));
		Result.High = GameState->HighEntities + HighEntityIndex;
		Result.LowIndex = Result.High->LowEntityIndex;
		Result.Low = GameState->LowEntities + Result.LowIndex;
	}
	return Result;
}

internal void UpdateFamiliar(game_state* GameState, entity Entity, float dt)
{
	entity ClosestHero = {};
	float ClosestDstSq = Square(10.0f);
	for(uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
	{
		entity TestEntity = EntityFromHighIndex(GameState, HighEntityIndex);
		if(TestEntity.Low->Type == EntityType_Hero)
		{
			float TestDstSq = LengthSq(TestEntity.High->Pos - Entity.High->Pos);
			if(ClosestDstSq > TestDstSq)
			{
				ClosestHero = TestEntity;
				ClosestDstSq = TestDstSq;
			}
		}
	}
	v2 ddp = {};
	if(ClosestHero.High && ClosestDstSq > Square(3.0f))
	{
		float Acceleration = 0.5f;
		float OneOverLength = Acceleration / SquareRoot(ClosestDstSq);
		ddp = OneOverLength * (ClosestHero.High->Pos - Entity.High->Pos);
		MoveEntity(GameState, Entity, dt, ddp);
	}
}

internal void UpdateMonster(game_state* GameState, entity Entity, float dt)
{

}

internal void DrawHitPoints(low_entity* LowEntity, entity_visible_piece_group* PieceGroup)
{
	if(LowEntity->HitPointMax >= 1)
	{
		v2 HealthDim = {0.2f, 0.2f};
		float SpacingX = 1.5f * HealthDim.X;
		v2 HitP = {-0.5f * (LowEntity->HitPointMax - 1) * SpacingX, -0.25f};
		v2 dHitP = {SpacingX, 0.0f};
		for(uint32 HealthIndex = 0; HealthIndex < LowEntity->HitPointMax; ++HealthIndex)
		{
			hit_point* HitPoint = LowEntity->HitPoint + HealthIndex;
			v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
			if(HitPoint->FilledAmount == 0)
			{
				Color = {0.2f, 0.2f, 0.2f, 1.0f};
			}
			PushRect(PieceGroup, HitP, 0, HealthDim, Color, 0.0f);
			HitP += dHitP;
		}
	}
}
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		// Null Entity,
		AddLowEntity (GameState, EntityType_None, 0);
		GameState->HighEntityCount = 1;
		GameState->Backdrop = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_background.bmp");
		GameState->Shadow = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test/test_hero_shadow.bmp");
		GameState->Tree = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/tree00.bmp");
		GameState->Sword = DEBUGLoadBMP (Thread, Memory->DEBUGReadEntireFile, "test2/rock03.bmp");
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

		InitializeArena (&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		
		Initializeworld(World, 1.4f);
		
		int32 TileSideInPixels = 60;
		GameState->MetersToPixel = ((float)TileSideInPixels / (float)World->TileSideInMeters);

		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenBaseX = 0;	//(INT16_MAX/TilesPerWidth)/2;
		uint32 ScreenBaseY = 0;	//(INT16_MAX/TilesPerHeight)/2;
		uint32 ScreenBaseZ = 0;	//INT16_MAX/2;
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
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice = (RandomNumberTable[RandomNumberIndex++] % 2/*((DoorUp || DoorDown) ? 2 : 3)*/);
			bool CreatedZDoor = false;
			if(RandomChoice == 2)
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
			else if(RandomChoice == 1)
			{
				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}
			
			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;					
					uint32 TileValue = 1;
					if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}
					if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}
					if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileX == 10) && (TileY == 6))
					{
						if(DoorUp)
						{
							TileValue = 3;
						}
						if(DoorDown)
						{
							TileValue = 4;
						}
					}
					if (TileValue == 2)
					{
						AddWall (GameState, AbsTileX, AbsTileY, AbsTileZ);
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
			
			if(RandomChoice == 2)
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
			else if(RandomChoice == 1)
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

		AddMonster(GameState, CameraTileX + 6, CameraTileY + 2, CameraTileZ);
		AddFamiliar(GameState, CameraTileX - 6, CameraTileY + 2, CameraTileZ);
		SetCamera(GameState, NewCameraP);
		
		Memory->IsInitialized = true;
	}
	world *World = GameState->World;
	// Tile width and Height
	float MeterToPixels = GameState->MetersToPixel;
	
	for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		uint32 LowIndex = GameState->PlayerControllerIndex[ControllerIndex];
		if (LowIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
				uint32 EntityIndex = AddPlayer(GameState).LowIndex;
				GameState->PlayerControllerIndex[ControllerIndex] = EntityIndex;
			}
		}
		else
		{
			entity ControllingEntity = ForceEntityIntoHigh (GameState, LowIndex);
			v2 ddPlayerP = {};
			if(Controller->IsAnalog)
			{
#if RAAVANAN_INTERNAL
				GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
				GameState->YOffset += (int)(4.0f * (Controller->StickAverageX));
#endif
				ddPlayerP = v2 {Controller->StickAverageX, Controller->StickAverageY};
			}
			else
			{
				if(Controller->MoveUp.EndedDown)
				{
					ddPlayerP.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ddPlayerP.Y = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ddPlayerP.X = 1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ddPlayerP.X = -1.0f;
				}
			}
			
			if (Controller->Start.EndedDown)
			{
				ControllingEntity.High->dZ = 3.0f;
			}
			v2 dSword = {};
			if(Controller->ActionUp.EndedDown)
			{
				dSword = V2(0.0f, 1.0f);
			}
			if(Controller->ActionDown.EndedDown)
			{
				dSword = V2(0.0f, -1.0f);
			}
			if(Controller->ActionLeft.EndedDown)
			{
				dSword = V2(-1.0f, 0.0f);
			}
			if(Controller->ActionRight.EndedDown)
			{
				dSword = V2(1.0f, 0.0f);
			}
			MoveEntity(GameState, ControllingEntity, Input->deltaTime, ddPlayerP);
			if((dSword.X != 0) || (dSword.Y != 0))
			{
				low_entity* Sword = GetLowEntity(GameState, ControllingEntity.Low->SwordLowIndex);
				if(Sword && !IsWorldPosValid(Sword->Pos))
				{
					world_position SwordP = ControllingEntity.Low->Pos;
					ChangeEntityLocation(&GameState->WorldArena, GameState->World, ControllingEntity.Low->SwordLowIndex, Sword, 0, &SwordP);
				}
			}
		}
	}

	entity CameraFollowingEntity = ForceEntityIntoHigh(GameState, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity.High)
	{
		world_position NewCameraP = GameState->CameraP;
		NewCameraP.ChunkZ = CameraFollowingEntity.Low->Pos.ChunkZ;
#if 0
		if (CameraFollowingEntity.High->Pos.X > (9.0f * World->TileSideInMeters))
		{
			NewCameraP.ChunkX += 17;
		}
		if (CameraFollowingEntity.High->Pos.X < -(9.0f * World->TileSideInMeters))
		{
			NewCameraP.ChunkX -= 17;
		}
		if (CameraFollowingEntity.High->Pos.Y > (5.0f * World->TileSideInMeters))
		{
			NewCameraP.ChunkY += 9;
		}
		if (CameraFollowingEntity.High->Pos.Y < -(5.0f * World->TileSideInMeters))
		{
			NewCameraP.ChunkY -= 9;
		}
#else
	NewCameraP = CameraFollowingEntity.Low->Pos;
#endif
		SetCamera(GameState, NewCameraP);
	}	
#if 1
	RenderRectangle(Buffer, V2(0.0f, 0.0f), V2((float)Buffer->Width, (float)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
	RenderBitMap (Buffer, &GameState->Backdrop, 0, 0);
#endif

	float ScreenCenterX = 0.5f * (float)Buffer->Width;
	float ScreenCenterY = 0.5f * (float)Buffer->Height;
#if 0
	for (int32 Relrow = -10; Relrow < 10; ++Relrow)
	{
		for(int32 Relcol = - 20; Relcol < 20; ++Relcol)
		{
			uint32 Column = GameState->CameraP.AbsTileX + Relcol;
			uint32 Row = GameState->CameraP.AbsTileY + Relrow;
			uint32 TileID = GetTileValue(World, Column, Row, GameState->CameraP.AbsTileZ);
			if(TileID > 1)
			{
				float ColorVal = ((TileID == 2) ? 1.0f : ((TileID > 2) ? 0.25f : 0.5f));
				ColorVal = ((Column == GameState->CameraP.AbsTileX) && (Row == GameState->CameraP.AbsTileY)) ? 0.0f : ColorVal;

				v2 TileSide = {0.5f * TileSideInPixels, 0.5f * TileSideInPixels};
				v2 Center = {ScreenCenterX - MeterToPixels * GameState->CameraP.Offset.X + ((float)Relcol) * TileSideInPixels,
								ScreenCenterY + MeterToPixels * GameState->CameraP.Offset.Y - ((float)Relrow) * TileSideInPixels};
				v2 Min = Center - TileSide;
				v2 Max = Center + TileSide;
				RenderRectangle(Buffer, Min, Max, ColorVal, ColorVal, ColorVal);
			}			
		}
	}
#endif
    //RenderGradiant(Buffer, GameState->XOffset, GameState->YOffset);
	entity_visible_piece_group PieceGroup;
	PieceGroup.GameState = GameState;
	for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
	{
		high_entity* HighEntity = GameState->HighEntities + HighEntityIndex;
		low_entity* LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

		entity Entity;
		Entity.LowIndex = HighEntity->LowEntityIndex;
		Entity.Low = LowEntity;
		Entity.High = HighEntity;
		float dt = Input->deltaTime;
		
		float ShadowAlpha = 1.0f - 0.5f * HighEntity->Z;
		ShadowAlpha = (ShadowAlpha < 0.0f) ? 0.0f : ShadowAlpha;
		
		hero_bitmaps *HeroBitsmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];
		PieceGroup.PieceCount = 0;
		switch (LowEntity->Type)
		{
			case EntityType_Hero:
				{
					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(&PieceGroup, &HeroBitsmaps->Torso, V2(0,0), 0, HeroBitsmaps->Align);
					PushBitmap(&PieceGroup, &HeroBitsmaps->Cape, V2(0,0), 0, HeroBitsmaps->Align);
					PushBitmap(&PieceGroup, &HeroBitsmaps->Head, V2(0,0), 0, HeroBitsmaps->Align);
					DrawHitPoints (LowEntity, &PieceGroup);
				}
			break;
			case EntityType_Wall:
				{
				#if 0
				RenderRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MeterToPixels * 0.9f * EntiryWidthHeight, PlayerR, PlayerG, PlayerB);	
				#else
				PushBitmap(&PieceGroup, &GameState->Tree, V2(0,0), 0, V2(40,80));
				#endif
				}
			break;
			case EntityType_Sword:
				{
					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(&PieceGroup, &GameState->Sword, V2(0,0), 0, V2(30,10));
				}
			break;
			case EntityType_Familiar:
			{
				UpdateFamiliar(GameState, Entity, dt);
				Entity.High->tBob += dt;
				if(Entity.High->tBob > (2.0f * PI))
					Entity.High->tBob -= (float)(2.0f * PI);
				float SinValue = Sin(2.0f * Entity.High->tBob);
				PushBitmap(&PieceGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, SinValue * (0.5f * ShadowAlpha) - (0.2f * SinValue), 0.0f);
				PushBitmap(&PieceGroup, &HeroBitsmaps->Head, V2(0,0), 1.0f * SinValue, HeroBitsmaps->Align);
			}
			break;
			case EntityType_Monster:
			{
				UpdateMonster(GameState, Entity, dt);
				PushBitmap(&PieceGroup, &GameState->Shadow, V2(0,0), 0, HeroBitsmaps->Align, ShadowAlpha, 0.0f);
				PushBitmap(&PieceGroup, &HeroBitsmaps->Torso, V2(0,0), 0, HeroBitsmaps->Align);
				DrawHitPoints (LowEntity, &PieceGroup);
			}
		}
		
		float ddZ = -9.8f;
		HighEntity->Z += (0.5f * ddZ * Square(dt) + HighEntity->dZ * dt);
		HighEntity->dZ = ddZ * dt + HighEntity->dZ;
		if (HighEntity->Z < 0)
		{
			HighEntity->Z = 0;
		}

		float EntityGroundPointX = ScreenCenterX + MeterToPixels * HighEntity->Pos.X;
		float EntityGroundPointY = ScreenCenterY - MeterToPixels * HighEntity->Pos.Y;
		float EntityZ = -MeterToPixels * HighEntity->Z;
#if 0
		v2 PlayerLeftTop = {EntityGroundPointX - 0.5f * MeterToPixels * LowEntity->Width, EntityGroundPointY - 0.5f * MeterToPixels * LowEntity->Height};
		v2 EntiryWidthHeight = {LowEntity->Width, LowEntity->Height};
#endif
		for(uint32 PieceIndex = 0; PieceIndex < PieceGroup.PieceCount; ++PieceIndex)
		{
			entity_visible_piece* Piece = PieceGroup.Pieces + PieceIndex;
			v2 Center = v2{EntityGroundPointX + Piece->Offset.X,
							EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + Piece->EntityZCofficient * EntityZ};
			if(Piece->Bitmap)
			{
				RenderBitMap(Buffer, Piece->Bitmap, EntityGroundPointX + Piece->Offset.X, EntityGroundPointY + Piece->Offset.Y + Piece->EntityZCofficient * EntityZ + Piece->OffsetZ, Piece->A);
			}
			else
			{
				v2 HalfDim = MeterToPixels*0.5f * Piece->Dim;
				RenderRectangle(Buffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
			}
		}
	}
}

extern "C" GET_GAME_SOUND_SAMPLES(GetGameSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	UpdateSound(GameState, SoundBuffer, 400 /* GameState->ToneHz */);
}
