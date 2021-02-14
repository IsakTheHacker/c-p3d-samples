#include "header/functions.h"

void panda_exit(const Event* theEvent, void* data) {
	exit(0);
}

void new_game() {
	using namespace Globals;

	ball_root.set_pos(maze.find("**/start").get_pos());
	ballV.set(0, 0, 0);
	accelV.set(0, 0, 0);

	roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
	AsyncTaskManager::get_global_ptr()->add(roll_task);
}

void wall_collide_handler(PT(CollisionEntry) entry) {
	using namespace Globals;

	LVector3 norm = entry->get_surface_normal(window->get_render()) * -1;
	double current_speed = ballV.length();
	LVector3 inVec = ballV / current_speed;
	double vel_angle = norm.dot(inVec);
	LVector3 hit_dir = entry->get_surface_point(window->get_render()) - ball_root.get_pos();
	hit_dir.normalize();
	double hit_angle = norm.dot(hit_dir);

	if (vel_angle > 0 && hit_angle > 0.995) {
		//Standard reflection equation
		LPoint3 reflectVec = (norm * norm.dot(inVec * -1) * 2) + inVec;

		ballV = reflectVec * (current_speed * (((1 - vel_angle) * 0.5) + 0.5));

		LPoint3 disp = entry->get_surface_point(window->get_render()) - entry->get_interior_point(window->get_render());
		LVector3 newPos = ball_root.get_pos() + disp;
		ball_root.set_pos(newPos);
	}
	printf("Wall collide handler called!\n");
}

void ground_collide_handler(PT(CollisionEntry) entry) {
	using namespace Globals;

	double newZ = entry->get_surface_point(window->get_render()).get_z();
	ball_root.set_z(newZ + 0.4);

	LVector3 norm = entry->get_surface_normal(window->get_render());
	LVector3 accelSide = norm.cross(LVector3::up());

	accelV = norm.cross(accelSide);
	printf("Ground collide handler called!\n");
}

void lose_game(PT(CollisionEntry) entry) {
	using namespace Globals;

	LPoint3 to_pos = entry->get_interior_point(window->get_render());

	AsyncTaskManager::get_global_ptr()->remove(roll_task);
	new_game();

	printf("Lose game!\n");
}

AsyncTask::DoneStatus roll_func(GenericAsyncTask* task, void* mouseWatcherNode) {
	using namespace Globals;

	double dt = ClockObject::get_global_clock()->get_dt();
	PT(MouseWatcher) mouseWatcher = DCAST(MouseWatcher, (PandaNode*)mouseWatcherNode);
	if (dt > 0.2) {
		return AsyncTask::DS_cont;
	}

	collision_traverser.traverse(window->get_render());
	if (collision_handler->get_num_entries() > 0) {
		collision_handler->sort_entries();
		for (size_t i = 0; i < collision_handler->get_num_entries(); i++) {
			PT(CollisionEntry) entry = collision_handler->get_entry(i);
			std::string name = entry->get_into_node()->get_name();
			if (name == "wall_collide") {
				wall_collide_handler(entry);
			} else if (name == "ground_collide") {
				ground_collide_handler(entry);
			} else if (name == "loseTrigger") {
				lose_game(entry);
			}
		}
	}

	if (mouseWatcher->has_mouse()) {
		LPoint2 mpos = mouseWatcher->get_mouse(); //Get the mouse position
		maze.set_p(mpos.get_y() * -10);
		maze.set_r(mpos.get_x() * 10);
	}

	ballV += accelV * dt * ACCELERATION;
	if (ballV.length_squared() > MAX_SPEED_SQ) {
		ballV.normalize();
		ballV *= MAX_SPEED;
	}
	ball_root.set_pos(ball_root.get_pos() + (ballV * dt));		//Update the position based on the velocity
	LRotation prevRot(ball.get_quat());
	LVector3 axis = LVector3::up().cross(ballV);
	LRotation newRot(axis, 45.5 * dt * ballV.length());
	ball.set_quat(prevRot * newRot);

	return AsyncTask::DS_cont;
}