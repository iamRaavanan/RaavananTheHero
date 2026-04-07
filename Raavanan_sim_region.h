#ifndef RAAVANAN_SIM_REGION_H
#include "Raavanan_Platform.h"
#include "Raavanan_math.h"
#include "Raavanan_world.h"

struct move_spec
{
	bool UnitMaxAccelVector;
	float Speed;
	float Drag;
};

inline move_spec DefaultMoveSpec()
{
	move_spec Result = {};
	Result.UnitMaxAccelVector = false;
	Result.Speed = 1.0f;
	Result.Drag = 0.0f;
	return Result;
}

enum entity_type
{
	EntityType_None,
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Sword
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
	uint8 Flags;
	uint8 FilledAmount;
};

struct sim_entity;
union entity_reference
{
    sim_entity* Ptr;
    uint32 Index;
};

enum sim_entity_flags
{
	EntityFlag_Collides = (1 << 1),
	EntityFlag_NonSpatial = (1 << 2),

	EntityFlag_Simming = (1 << 30),	
};

struct sim_entity
{
    uint32 StorageIndex;

    entity_type Type;
	uint32 Flags;

    v2 Pos;
	v2 dPlayerP;
	
    float Z;
    float dZ;
	
    uint32 ChunkZ;
	
	float Height, Width;
	
    uint32 FacingDirection;
    float tBob;

	bool Collides;
	int32 dAbsTileZ;

	uint32 HighEntityIndex;

	uint32 HitPointMax;
	hit_point HitPoint[16];

	entity_reference Sword;
	float DistanceRemaining;
};

struct stored_entity
{
    world_position *Pos;
};

struct sim_entity_hash
{
    sim_entity* Ptr;
    uint32 Index;
};

struct sim_region
{
    world* World;
    world_position Origin;
    rectangle2  Bounds;
    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity* Entities;

    sim_entity_hash Hash[4096];
};

#define RAAVANAN_SIM_REGION_H
#endif