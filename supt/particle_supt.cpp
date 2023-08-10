#include <panda3d/particleSystemManager.h>
#include <panda3d/physicsManager.h>
#include "sample_supt.h"

void update_particles(ParticleSystemManager *particle_mgr,
		      PhysicsManager *physics_mgr)
{
    start_updater("particles updater", [=] {
	auto dt = ClockObject::get_global_clock()->get_dt();
	particle_mgr->do_particles(dt);
	physics_mgr->do_physics(dt);
	return AsyncTask::DS_cont;
    });  // ShowBase uses default sort order, which I think is 0
}

void kill_particles()
{
    kill_task("particles updater");
}
