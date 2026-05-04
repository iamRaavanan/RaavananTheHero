#include "Raavanan_sim_region.h"
#include "Raavanan.h"
#include <stdio.h>
#include <windows.h>

internal sim_entity_hash* GetHashFromStorageIndex(sim_region* Region, uint32 StorageIndex)
{
    Assert(StorageIndex);
    sim_entity_hash* Result = 0;
    uint32 HashValue = StorageIndex;
    for(uint32 Offset = 0; Offset < ArrayCount(Region->Hash); ++Offset)
    {
		uint32 HashMask = (ArrayCount(Region->Hash) - 1);
		uint32 HashIndex = ((HashValue + Offset) & HashMask);
        sim_entity_hash* Entry = Region->Hash + HashIndex;
        if(Entry->Index == 0 || (Entry->Index == StorageIndex))
        {
            Result = Entry;
            break;
        }
    }
    return Result;
}

internal sim_entity* GetStorageIndexToEntity(sim_region* Region, uint32 StorageIndex)
{
    sim_entity_hash* Entry = GetHashFromStorageIndex(Region, StorageIndex);
    sim_entity* Result = Entry->Ptr;
    return Result;
}

inline v3 GetSimSpacePos (sim_region* Region, low_entity* StoredEntity)
{
	v3 Result = InvalidP;
	if(!IsSet(&StoredEntity->Sim, EntityFlag_NonSpatial))
	{
		Result = Subtract(Region->World, &StoredEntity->Pos, &Region->Origin);
	}
    return Result;
}

internal sim_entity* AddEntity(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source, v3* SimPos);
inline void LoadEntityReference(game_state* GameState, sim_region* Region, entity_reference* Ref)
{
    if(Ref->Index)
    {
        sim_entity_hash* Entry = GetHashFromStorageIndex(Region, Ref->Index);
        if(Entry->Ptr == 0)
        {
			Entry->Index = Ref->Index;
			low_entity* LowEntity = GetLowEntity(GameState, Ref->Index);
			v3 SimP = GetSimSpacePos(Region, LowEntity);
            Entry->Ptr = AddEntity(GameState, Region, Ref->Index, LowEntity, &SimP);
        }
        Ref->Ptr = Entry->Ptr;
    }
}

inline void StoreEntityReference(entity_reference* Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->StorageIndex;
    }
}

internal sim_entity* AddEntityRaw(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source)
{
    Assert(StorageIndex);
    sim_entity* Entity = 0;	
	sim_entity_hash* Entry = GetHashFromStorageIndex(Region, StorageIndex);
	if(Entry->Ptr ==0)
	{
		if(Region->EntityCount < Region->MaxEntityCount)
		{
			Entity = Region->Entities + Region->EntityCount++;
			Assert((Entry->Index == 0) ||(Entry->Index == StorageIndex));
			Entry->Index = StorageIndex;
			Entry->Ptr = Entity;
			
			if(Source)
			{
				*Entity = Source->Sim;
				LoadEntityReference(GameState, Region, &Entity->Sword);
				Assert(!IsSet(&Source->Sim, EntityFlag_Simming));
				AddFlags(&Source->Sim, EntityFlag_Simming);
			}
			Entity->StorageIndex = StorageIndex;
			Entity->Updatable = false;
		}
		else
		{
			InvalidCodePath;
		}
	}
    return Entity;
}

internal bool EntityOverlapsRectanlge(v3 Pos, sim_entity_collision_volume Volume, rectangle3 Bounds)
{
	rectangle3 UpdatedRect = AddRadiusTo(Bounds, 0.5f * Volume.Dim);
	bool Result = IsInRectangle(UpdatedRect, Pos + Volume.OffsetPos);
	return Result;
}

internal sim_entity* AddEntity(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source, v3* SimPos)
{
    sim_entity* Dest = AddEntityRaw(GameState, Region, StorageIndex, Source);
    if(Dest)
    {
        if(SimPos)
        {
            Dest->Pos = *SimPos;
			Dest->Updatable = EntityOverlapsRectanlge(Dest->Pos, Dest->Collision->TotalVolume, Region->UpdatableBounds);
        }
        else
        {
            Dest->Pos = GetSimSpacePos(Region, Source);
        }
    }
    return Dest;
}

internal sim_region* BeginSim(memory_arena* SimArena, game_state* GameState, world* World, world_position Origin, rectangle3 Bounds, float deltaTime)
{
    sim_region* SimRegion = PushStruct(SimArena, sim_region);
	ZeroStruct(SimRegion->Hash);

	SimRegion->MaxEntityRadius = 5.0f;
	SimRegion->MaxEntityVelocity = 30.0f;
	float UpdateSafetyMargin = SimRegion->MaxEntityRadius + SimRegion->MaxEntityVelocity * deltaTime;
	float UpdateSafetyMarginZ = 1.0f;
    SimRegion->World = World;
    SimRegion->Origin = Origin;
	SimRegion->UpdatableBounds = AddRadiusTo(Bounds, V3(SimRegion->MaxEntityRadius));
    SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, V3(UpdateSafetyMargin, UpdateSafetyMargin, UpdateSafetyMarginZ));
    SimRegion->MaxEntityCount = 4096 ;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
	
	for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
	{
		for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
		{
			for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
			{
				world_chunk* Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
				if(Chunk)
				{
					for(world_entity_block* Block = &Chunk->FirstBlock; Block; Block = Block->Next)
					{
						for (uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
						{
							uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
							low_entity* Low = GameState->LowEntities + LowEntityIndex;
							if(!IsSet(&Low->Sim, EntityFlag_NonSpatial))
							{
								v3 SimSpaceP = GetSimSpacePos(SimRegion, Low);
								if(EntityOverlapsRectanlge(SimSpaceP, Low->Sim.Collision->TotalVolume, SimRegion->Bounds))
								{
									AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
								}
							}
						}
					}
				}
			}
		}
	}
	uint32 SimregionCount = SimRegion->EntityCount;
    return SimRegion;
}

internal void EndSim(sim_region* Region, game_state* GameState)
{
    sim_entity* Entity = Region->Entities;
    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
    {
        low_entity* Stored = GameState->LowEntities + Entity->StorageIndex;
		Assert(IsSet(&Stored->Sim, EntityFlag_Simming));
        Stored->Sim = *Entity;
		Assert(!IsSet(&Stored->Sim, EntityFlag_Simming));
        StoreEntityReference(&Stored->Sim.Sword);


        world_position NewPos = IsSet(Entity, EntityFlag_NonSpatial) ? NullPosition() : MapIntoChunkSpace(GameState->World, Region->Origin, Entity->Pos);
        ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity->StorageIndex, Stored, NewPos);

        if(Entity->StorageIndex == GameState->CameraFollowingEntityIndex)
        {
            world_position NewCameraP = GameState->CameraP;
            NewCameraP.ChunkZ = Stored->Pos.ChunkZ;
    #if 0
            if (CameraFollowingEntity->Pos.X > (9.0f * World->TileSideInMeters))
            {
                NewCameraP.ChunkX += 17;
            }
            if (CameraFollowingEntity->Pos.X < -(9.0f * World->TileSideInMeters))
            {
                NewCameraP.ChunkX -= 17;
            }
            if (CameraFollowingEntity->Pos.Y > (5.0f * World->TileSideInMeters))
            {
                NewCameraP.ChunkY += 9;
            }
            if (CameraFollowingEntity->Pos.Y < -(5.0f * World->TileSideInMeters))
            {
                NewCameraP.ChunkY -= 9;
            }
    #else
		float CamZOffset = NewCameraP.Offset.Z;
        NewCameraP = Stored->Pos;
		NewCameraP.Offset.Z = CamZOffset;
    #endif
			GameState->CameraP = NewCameraP;
        }
    }
}

internal bool TestWall (float WallX, float RelX, float RelY, float PlayerDeltaX, float PlayerDeltaY,
						float *tMin, float MinY, float MaxY)
{
	bool Hit = false;
	float tEpsilon = 0.001f;
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

internal bool CanCollide (game_state* GameState, sim_entity* A, sim_entity* B)
{
	bool Result = false;
	if(A != B)
	{
		if(A->StorageIndex > B->StorageIndex)
		{
			sim_entity* Temp = A;
			A = B;
			B = Temp;
		}
		if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides))
		{
			if(!IsSet(A, EntityFlag_NonSpatial) && !IsSet(B, EntityFlag_NonSpatial))
			{
				Result = true;
			}
			
			uint32 HashBucket = A->StorageIndex & (ArrayCount(GameState->CollisionRuleHash) - 1);
			for(pairwise_collision_rule* Rule = GameState->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextInHash)
			{
				if((Rule->StorageIndexA == A->StorageIndex) &&
					(Rule->StorageIndexB == B->StorageIndex))
				{
					Result = Rule->CanCollide;
					break;
				}
			}	
		}		
	}
	return Result;
}

internal bool HandleCollision(game_state* GameState, sim_entity* A, sim_entity* B)
{
	bool StopsOnCollision = false;
	if(A->Type == EntityType_Sword)
	{
		AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
		StopsOnCollision = false;
	}
	else
	{
		StopsOnCollision = true;
	}
	if(A->Type > B->Type)
	{
		sim_entity* Temp = A;
		A = B;
		B = Temp;
	}
	if((A->Type == EntityType_Monster) && (B->Type == EntityType_Sword))
	{
		if(A->HitPointMax > 0)
			--A->HitPointMax;
	}
	return StopsOnCollision;
}

internal bool CanOverlap(game_state* GameState, sim_entity* Mover, sim_entity* Region)
{
	bool Result = false;
	if(Mover != Region)
	{
		if(Region->Type == EntityType_Stairwell)
		{
			Result = true;
		}
	}
	return Result;
}

internal void HandleOverlap(game_state* GameState, sim_entity* Mover, sim_entity* Region, float deltaTime, float* Ground)
{
	if(Region->Type == EntityType_Stairwell)
	{
		*Ground = GetStairGround(Region, GetEntityGroundPoint(Mover));
	}
}

internal bool SpeculativeCollide (sim_entity* Mover, sim_entity* Region)
{
	bool Result = true;
	if(Region->Type == EntityType_Stairwell)
	{
		float StepHeight = 0.1f;
		// Result = ((AbsoluteValue(GetEntityGroundPoint(Mover).Z - Ground) > StepHeight) || ((Bary.Y > 0.1f) && (Bary.Y < 0.9f)));
		v3 MoverGroundPoint = GetEntityGroundPoint(Mover);
		float Ground = GetStairGround(Region, MoverGroundPoint);
		Result = (AbsoluteValue(MoverGroundPoint.Z - Ground) > StepHeight);
	}
	return Result;
}

internal void MoveEntity (game_state* GameState, sim_region* Region, sim_entity* Entity, float deltaTime, move_spec* MoveSpec, v3 ddPlayer)
{
	Assert(!IsSet(Entity, EntityFlag_NonSpatial));
	world* World = Region->World;
	
	if(MoveSpec->UnitMaxAccelVector)
	{
		float ddPLength = LengthSq(ddPlayer);
		if(ddPLength > 1.0f)
		{
			ddPlayer *= (1.0f / SquareRoot(ddPLength));
		}
	}
	
	ddPlayer *= MoveSpec->Speed;
	v3 Drag = -MoveSpec->Drag * Entity->dPlayerP;
	Drag.Z = 0.0f;
	ddPlayer += Drag;
	
	if(!IsSet(Entity, EntityFlag_ZSupported))
	{
		ddPlayer += V3(0, 0, -9.8f);
	}

	v3 OldPlayerP = Entity->Pos;
	v3 PlayerDelta = 0.5f * ddPlayer * Square(deltaTime) + Entity->dPlayerP * deltaTime;
	Entity->dPlayerP = ddPlayer * deltaTime + Entity->dPlayerP;
	
	Assert(LengthSq(Entity->dPlayerP) <= Square(Region->MaxEntityVelocity));
	v3 NewPlayerP = OldPlayerP + PlayerDelta;

	float DistanceRemaining = Entity->DistanceLimit;
	if(DistanceRemaining == 0.0f)
	{
		DistanceRemaining = 1000.0f;
	}

	for (uint32 Iteration = 0; Iteration < 4; ++Iteration)
	{
		float tMin = 1.0f;
		float PlayerDeltaLength = Length(PlayerDelta);
		if(PlayerDeltaLength > 0.0f)
		{
			if(PlayerDeltaLength > DistanceRemaining)
			{
				tMin = (DistanceRemaining / PlayerDeltaLength);
			}
			v3 WallNormal = {};
			sim_entity* HitEntity = 0;
			v3 DesiredPos = Entity->Pos + PlayerDelta;
			
			if(!IsSet(Entity, EntityFlag_NonSpatial))
			{
				for (uint32 TestHighEntityIndex = 0; TestHighEntityIndex < Region->EntityCount; ++TestHighEntityIndex)
				{
					sim_entity* TestEntity = Region->Entities + TestHighEntityIndex;
					if(CanCollide(GameState, Entity, TestEntity))
					{
						for (uint32 EntityVolumeIndex = 0; EntityVolumeIndex < Entity->Collision->VolumeCount; ++EntityVolumeIndex)
						{
							sim_entity_collision_volume* EntityVolume = Entity->Collision->Volumes + EntityVolumeIndex;
							for (uint32 TestEntityVolumeIndex = 0; TestEntityVolumeIndex < TestEntity->Collision->VolumeCount; ++TestEntityVolumeIndex)
							{
								sim_entity_collision_volume* TestVolume = TestEntity->Collision->Volumes + TestEntityVolumeIndex;
								v3 MinkowskiDiameter = {TestVolume->Dim.X + EntityVolume->Dim.X, TestVolume->Dim.Y + EntityVolume->Dim.Y, TestVolume->Dim.Z + EntityVolume->Dim.Z};
								v3 MinCorner = -0.5f * MinkowskiDiameter;
								v3 MaxCorner = 0.5f * MinkowskiDiameter;

								v3 Rel = ((Entity->Pos + EntityVolume->OffsetPos) - (TestEntity->Pos + TestVolume->OffsetPos));
								
								if((Rel.Z >= MinCorner.Z) && (Rel.Z < MaxCorner.Z))
								{
									float tMinTest = tMin;
									v3 TestWallNormal = {};
									bool HitTest = false;
									if(TestWall (MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMinTest, MinCorner.Y, MaxCorner.Y))
									{
										TestWallNormal = V3(-1,0,0);
										HitTest = true;
									}
									if(TestWall (MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMinTest, MinCorner.Y, MaxCorner.Y))
									{
										TestWallNormal = V3(1,0,0);
										HitTest = true;
									}
									if(TestWall (MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMinTest, MinCorner.X, MaxCorner.X))
									{
										TestWallNormal = V3(0,-1,0);
										HitTest = true;
									}
									if(TestWall (MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMinTest, MinCorner.X, MaxCorner.X))
									{
										TestWallNormal = V3(0,1,0);
										HitTest = true;
									}
									if(HitTest)
									{
										if(SpeculativeCollide (Entity, TestEntity))
										{
											tMin = tMinTest;
											WallNormal = TestWallNormal;
											HitEntity = TestEntity;
										}
									}
								}
							}
						}
					}
				}		
			}
			Entity->Pos += tMin * PlayerDelta;
			DistanceRemaining -= tMin * PlayerDeltaLength;
			if(HitEntity)
			{
				PlayerDelta = DesiredPos - Entity->Pos;
				
				bool StopsOnCollision = HandleCollision(GameState, Entity, HitEntity);

				if(StopsOnCollision)
				{
					PlayerDelta = PlayerDelta - 1 * Dot(PlayerDelta, WallNormal) * WallNormal;
					Entity->dPlayerP = Entity->dPlayerP - 1 * Dot(Entity->dPlayerP, WallNormal) * WallNormal;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	float Ground = 0.0f;
	{
		rectangle3 EntityRect = RectCenterDim(Entity->Pos + Entity->Collision->TotalVolume.OffsetPos, Entity->Collision->TotalVolume.Dim);
		for (uint32 TestHighEntityIndex = 0; TestHighEntityIndex < Region->EntityCount; ++TestHighEntityIndex)
		{
			sim_entity* TestEntity = Region->Entities + TestHighEntityIndex;
			if(CanOverlap(GameState, Entity, TestEntity))
			{
				rectangle3 TestEntityRect = RectCenterDim(TestEntity->Pos + TestEntity->Collision->TotalVolume.OffsetPos, TestEntity->Collision->TotalVolume.Dim);
				if(RectangleIntersect(EntityRect, TestEntityRect))
				{
					HandleOverlap(GameState, Entity, TestEntity, deltaTime, &Ground);
				}
			}
		}
	}
	Ground += Entity->Pos.Z - GetEntityGroundPoint(Entity).Z;
	if ((Entity->Pos.Z <= Ground) || (IsSet(Entity, EntityFlag_ZSupported) && Entity->dPlayerP.Z == 0.0f))
	{
		Entity->Pos.Z = Ground;
		Entity->dPlayerP.Z = 0;
		AddFlags(Entity, EntityFlag_ZSupported);
	}
	else
	{
		ClearFlags(Entity, EntityFlag_ZSupported);
	}

	if(Entity->DistanceLimit != 0.0f)
	{
		Entity->DistanceLimit = DistanceRemaining;
	}
	if((Entity->dPlayerP.X == 0.0f) && (Entity->dPlayerP.Y == 0.0f))
	{

	}
	else if(AbsoluteValue(Entity->dPlayerP.X) > AbsoluteValue(Entity->dPlayerP.Y))
	{
		if(Entity->dPlayerP.X > 0)
		{
			Entity->FacingDirection = 0;
		}
		else
		{
			Entity->FacingDirection = 2;
		}
	}
	else
	{
		if(Entity->dPlayerP.Y > 0)
		{
			Entity->FacingDirection = 1;
		}
		else
		{
			Entity->FacingDirection = 3;
		}
	}
}
