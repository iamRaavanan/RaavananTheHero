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

enum entity_type
{
	EntityType_None,
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Stairwell,
	EntityType_Sword,
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
	EntityFlag_Collides = (1 << 0),
	EntityFlag_NonSpatial = (1 << 1),
	EntityFlag_Movable = (1 << 2),
	EntityFlag_ZSupported = (1 << 4),

	EntityFlag_Simming = (1 << 30),	
};

struct sim_entity
{
    uint32 StorageIndex;
	bool Updatable;

    entity_type Type;
	uint32 Flags;

    v3 Pos;
	v3 dPlayerP;

    uint32 ChunkZ;
	
	v3 Dim;
	
    uint32 FacingDirection;
    float tBob;

	bool Collides;
	int32 dAbsTileZ;

	uint32 HighEntityIndex;

	uint32 HitPointMax;
	hit_point HitPoint[16];

	entity_reference Sword;
	float DistanceLimit;
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
	float MaxEntityRadius;
	float MaxEntityVelocity;

    world_position Origin;
    rectangle3 Bounds;
	rectangle3 UpdatableBounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity* Entities;

	float DefaultGroundLevel;
    sim_entity_hash Hash[4096];
};

#define RAAVANAN_SIM_REGION_H
#endif