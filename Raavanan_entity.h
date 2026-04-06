#ifndef RAAVANAN_ENTITY_H
#define InvalidP V2(100000.0f, 100000.0f)

inline bool IsSet(sim_entity* Entity, uint32 Flag)
{
	bool Result = Entity->Flags & Flag;
	return Result;
}

inline void AddFlag(sim_entity* Entity, uint32 Flag)
{
    Entity->Flags |= Flag;
}

inline void ClearFlag(sim_entity* Entity, uint32 Flag)
{
    Entity->Flags &= ~Flag;
}

inline void MakeEntityNonSpatial(sim_entity* Entity)
{
	AddFlag(Entity, EntityFlag_NonSpatial);
	Entity->Pos = InvalidP;
}

inline void MakeEntitySpatial(sim_entity* Entity, v2 Pos, v2 dP)
{
	ClearFlag(Entity, EntityFlag_NonSpatial);
	Entity->Pos = Pos;
	Entity->dPlayerP = dP;
}

#define RAAVANAN_ENTITY_H
#endif