#ifndef RAAVANAN_ENTITY_H
#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

inline bool IsSet(sim_entity* Entity, uint32 Flag)
{
	bool Result = Entity->Flags & Flag;
	return Result;
}

inline void AddFlags(sim_entity* Entity, uint32 Flag)
{
    Entity->Flags |= Flag;
}

inline void ClearFlags(sim_entity* Entity, uint32 Flag)
{
    Entity->Flags &= ~Flag;
}

inline void MakeEntityNonSpatial(sim_entity* Entity)
{
	AddFlags(Entity, EntityFlag_NonSpatial);
	Entity->Pos = InvalidP;
}

inline void MakeEntitySpatial(sim_entity* Entity, v3 Pos, v3 dP)
{
	ClearFlags(Entity, EntityFlag_NonSpatial);
	Entity->Pos = Pos;
	Entity->dPlayerP = dP;
}

inline v3 GetEntityGroundPoint (sim_entity* Entity, v3 ForEntityP)
{
	v3 Result = ForEntityP;
	return Result;
}

inline v3 GetEntityGroundPoint (sim_entity* Entity)
{
	v3 Result = GetEntityGroundPoint(Entity, Entity->Pos);
	return Result;
}

inline float GetStairGround(sim_entity* Entity, v3 AtGroundPoint)
{
	Assert(Entity->Type == EntityType_Stairwell);
	rectangle2 RegionRect = RectCenterDim(Entity->Pos.XY, Entity->WalkableDim);
	v2 Bary = Clamp(GetBaryCentric(RegionRect, AtGroundPoint.XY));
	float Result = Entity->Pos.Z + Bary.Y * Entity->WalkableHeight;
	return Result;
}
#define RAAVANAN_ENTITY_H
#endif