#include "Raavanan_sim_region.h"
#include "Raavanan_world.h"

inline void UpdateFamiliar(sim_region* Region, sim_entity* Entity, float dt)
{
	sim_entity* ClosestHero = 0;
	float ClosestDstSq = Square(10.0f);
	for(uint32 TestEntityIndex = 1; TestEntityIndex < Region->EntityCount; ++TestEntityIndex)
	{
		sim_entity* TestEntity = Region->Entities;
		if(TestEntity->Type == EntityType_Hero)
		{
			float TestDstSq = LengthSq(TestEntity->Pos - Entity->Pos);
            if(TestEntity->Type == EntityType_Hero)
            {
                TestDstSq *= 0.75f;
            }
			if(ClosestDstSq > TestDstSq)
			{
				ClosestHero = TestEntity;
				ClosestDstSq = TestDstSq;
			}
		}
	}
	v2 ddp = {};
	if(ClosestHero && ClosestDstSq > Square(3.0f))
	{
		float Acceleration = 0.5f;
		float OneOverLength = Acceleration / SquareRoot(ClosestDstSq);
		ddp = OneOverLength * (ClosestHero->Pos - Entity->Pos);
	}
	move_spec MoveSpec = DefaultMoveSpec();
	MoveSpec.UnitMaxAccelVector = true;
	MoveSpec.Speed = 50.0f;
	MoveSpec.Drag = 8.0f;
	MoveEntity(Region, Entity, dt, &MoveSpec, ddp);
}

internal void UpdateMonster(sim_region* Region, sim_entity* Entity, float dt)
{

}

internal void UpdateSword(sim_region* Region, sim_entity* Entity, float dt)
{
	move_spec MoveSpec = DefaultMoveSpec();
	MoveSpec.Speed = 0.0f;
	v2 OldP = Entity->Pos;
	MoveEntity(Region, Entity, dt, &MoveSpec, V2(0,0));
	float DistanceTraveled = Length(Entity->Pos - OldP);
	Entity->DistanceRemaining -= DistanceTraveled;
	if(Entity->DistanceRemaining < 0)
	{
        Assert(!"Need to make entities be able to NOT be there");
	}
}
