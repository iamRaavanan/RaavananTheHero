#include "Raavanan_sim_region.h"
#include "Raavanan.h"

internal sim_entity_hash* GetHashFromStorageIndex(sim_region* Region, uint32 StorageIndex)
{
    Assert(StorageIndex);
    sim_entity_hash* Result = 0;
    uint32 HashValue = StorageIndex;
    for(uint32 Offset = 0; Offset < ArrayCount(Region->Hash); ++Offset)
    {
		uint32 HashMask = (ArrayCount(Region->Hash) - 1);
		uint32 HashIndex = ((HashValue + Offset) & (HashMask));
        sim_entity_hash* Entry = Region->Hash + HashIndex;
        if(Entry->Index == 0 || (Entry->Index == StorageIndex))
        {
            Result = Entry;
            break;
        }
    }
    return Result;
}

internal void MapStorageIndexToEntity(sim_region* Region, uint32 StorageIndex, sim_entity* Entity)
{
    sim_entity_hash* Entry = GetHashFromStorageIndex(Region, StorageIndex);
    Assert((Entry->Index == 0) || (Entry->Index == StorageIndex));
    Entry->Index = StorageIndex;
    Entry->Ptr = Entry->Ptr;
}

internal sim_entity* GetStorageIndexToEntity(sim_region* Region, uint32 StorageIndex)
{
    sim_entity_hash* Entry = GetHashFromStorageIndex(Region, StorageIndex);
    sim_entity* Result = Entry->Ptr;
    return Result;
}

internal sim_entity* AddEntity(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source);
inline void LoadEntityReference(game_state* GameState, sim_region* Region, entity_reference* Ref)
{
    if(Ref->Index)
    {
        sim_entity_hash* Entry = GetHashFromStorageIndex(Region, Ref->Index);
        if(Entry->Ptr == 0)
        {
			Entry->Index = Ref->Index;
            AddEntity(GameState, Region, Ref->Index, GetLowEntity(GameState, Ref->Index));
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

internal sim_entity* AddEntity(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source)
{
    Assert(StorageIndex);
    sim_entity* Entity = 0;	
    if(Region->EntityCount < Region->MaxEntityCount)
    {
        Entity = Region->Entities + Region->EntityCount++;
        MapStorageIndexToEntity(Region, StorageIndex, Entity);
        if(Source)
        {
            *Entity = Source->Sim;
            LoadEntityReference(GameState, Region, &Entity->Sword);
        }
        Entity->StorageIndex = StorageIndex;
    }
    else
    {
        InvalidCodePath;
    }
    return Entity;
}

inline v2 GetSimSpacePos (sim_region* Region, low_entity* StoredEntity)
{
    world_difference Diff = Subtract(Region->World, &StoredEntity->Pos, &Region->Origin);
    v2 Result = Diff.dXY;
    return Result;
}

internal sim_entity* AddEntity(game_state* GameState, sim_region* Region, uint32 StorageIndex, low_entity* Source, v2* SimPos)
{
    sim_entity* Dest = AddEntity(GameState, Region, StorageIndex, Source);
    if(Dest)
    {
        if(SimPos)
        {
            Dest->Pos = *SimPos;
        }
        else
        {
            Dest->Pos = GetSimSpacePos(Region, Source);
        }
    }
    return Dest;
}

internal sim_region* BeginSim(memory_arena* SimArena, game_state* GameState, world* World, world_position Origin, rectangle2 Bounds)
{
    sim_region* SimRegion = PushStruct(SimArena, sim_region);
	ZeroStruct(SimRegion->Hash);

    SimRegion->World = World;
    SimRegion->Origin = Origin;
    SimRegion->Bounds = Bounds;
    SimRegion->MaxEntityCount = 4096 ;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

    world_position MinChunkP = MapIntoChunkSpace(GameState->World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
	world_position MaxChunkP = MapIntoChunkSpace(GameState->World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
	
	for(int32 ChunkY = MinChunkP.ChunkY; ChunkY < MaxChunkP.ChunkY; ++ChunkY)
	{
		for(int32 ChunkX = MinChunkP.ChunkX; ChunkX < MaxChunkP.ChunkX; ++ChunkX)
		{
			world_chunk* Chunk = GetWorldChunk(GameState->World, ChunkX, ChunkY, SimRegion->Origin.ChunkZ);
			if(Chunk)
			{
				for(world_entity_block* Block = &Chunk->FirstBlock; Block; Block = Block->Next)
				{
					for (uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
					{
						uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
						low_entity* Low = GameState->LowEntities + LowEntityIndex;
						v2 SimSpaceP = GetSimSpacePos(SimRegion, Low);
                        if(IsInRectangle(SimRegion->Bounds  , SimSpaceP))
                        {
                            AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
                        }
					}
				}
			}
		}
	}
    return SimRegion;
}

internal void EndSim(sim_region* Region, game_state* GameState)
{
    sim_entity* Entity = Region->Entities;
    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex)
    {
        low_entity* Stored = GameState->LowEntities + Entity->StorageIndex;
        Stored->Sim = *Entity;
        StoreEntityReference(&Stored->Sim.Sword);


        world_position NewPos = MapIntoChunkSpace(GameState->World, Region->Origin, Entity->Pos);
        ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity->StorageIndex, Stored, &Stored->Pos, &NewPos);

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
        NewCameraP = Stored->Pos;
    #endif
        }
    }
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

internal void MoveEntity (sim_region* Region, sim_entity* Entity, float deltaTime, move_spec* MoveSpec, v2 ddPlayer)
{
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

	ddPlayer += -MoveSpec->Drag * Entity->dPlayerP;

	v2 OldPlayerP = Entity->Pos;
	v2 PlayerDelta = 0.5f * ddPlayer * Square(deltaTime) + Entity->dPlayerP * deltaTime;
	Entity->dPlayerP = ddPlayer * deltaTime + Entity->dPlayerP;
	v2 NewPlayerP = OldPlayerP + PlayerDelta;

	for (uint32 Iteration = 0; (Iteration < 4); ++Iteration)
	{
		float tMin = 1.0f;
		v2 WallNormal = {};
		sim_entity* HitEntity = 0;
		v2 DesiredPos = Entity->Pos + PlayerDelta;
		if(Entity->Collides)
		{
			for (uint32 TestHighEntityIndex = 1; TestHighEntityIndex < Region->EntityCount; ++TestHighEntityIndex)
			{
                sim_entity* TestEntity = Region->Entities + TestHighEntityIndex;
				if(Entity != TestEntity)
				{
					if(TestEntity->Collides)
					{
						float DiameterW = TestEntity->Width + Entity->Width;
						float DiameterH = TestEntity->Height + Entity->Height;
						v2 MinCorner = -0.5f * v2 {DiameterW, DiameterH};
						v2 MaxCorner = 0.5f * v2 {DiameterW, DiameterH};

						v2 Rel = Entity->Pos - TestEntity->Pos;
						
						if(TestWall (MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
						{
							WallNormal = V2(-1,0);
							HitEntity = TestEntity;
						}
						if(TestWall (MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y))
						{
							WallNormal = V2(1,0);
							HitEntity = TestEntity;
						}
						if(TestWall (MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
						{
							WallNormal = V2(0,-1);
							HitEntity = TestEntity;
						}
						if(TestWall (MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X))
						{
							WallNormal = V2(0,1);
							HitEntity = TestEntity;
						}
					}
				}			
			}		
		}
		Entity->Pos += tMin * PlayerDelta;
		if(HitEntity)
		{
			Entity->dPlayerP = Entity->dPlayerP - 1 * Dot(Entity->dPlayerP, WallNormal) * WallNormal;
			PlayerDelta = DesiredPos - Entity->Pos;
			PlayerDelta = PlayerDelta - 1 * Dot(PlayerDelta, WallNormal) * WallNormal;
        }
		else
		{
			break;
		}		
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
