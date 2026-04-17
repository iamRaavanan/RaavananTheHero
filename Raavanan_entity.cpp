#include "Raavanan_sim_region.h"
#include "Raavanan_world.h"

inline move_spec DefaultMoveSpec()
{
	move_spec Result = {};
	Result.UnitMaxAccelVector = false;
	Result.Speed = 1.0f;
	Result.Drag = 0.0f;
	return Result;
}