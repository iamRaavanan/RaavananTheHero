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

#define RAAVANAN_ENTITY_H
#endif