#include "header/functions.h"
#include <asyncTaskSequence.h>

void panda_exit(const Event* theEvent, void* data) {
	exit(0);
}

void new_game() {
	using namespace Globals;

        // The maze model also has a locator in it for where to start the ball
        // To access it we use the find command
        // Set the ball in the starting position
	ball_root.set_pos(maze.find("**/start").get_pos());
	ballV.set(0, 0, 0);         // Initial velocity is 0
	accelV.set(0, 0, 0);        // Initial acceleration is 0

        // Create the movement task
	// It is stopped before calling new_game() again.
	roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
	AsyncTaskManager::get_global_ptr()->add(roll_task);
}

// This function handles the collision between the ball and a wall
void wall_collide_handler(PT(CollisionEntry) entry) {
	using namespace Globals;

        // First we calculate some numbers we need to do a reflection
	// The normal of the wall
	auto norm = entry->get_surface_normal(window->get_render()) * -1;
	double current_speed = ballV.length();	// The current speed
	auto inVec = ballV / current_speed;	// The direction of travel
	double vel_angle = norm.dot(inVec);	// Angle of incidence
	auto hit_dir = entry->get_surface_point(window->get_render()) - ball_root.get_pos();
	hit_dir.normalize();
	// The angle between the ball and the normal
	double hit_angle = norm.dot(hit_dir);

        // Ignore the collision if the ball is either moving away from the wall
        // already (so that we don't accidentally send it back into the wall)
        // and ignore it if the collision isn't dead-on (to avoid getting caught on
        // corners)
	if (vel_angle > 0 && hit_angle > 0.995) {
		//Standard reflection equation
		auto reflectVec = (norm * norm.dot(inVec * -1) * 2) + inVec;

		// This makes the velocity half of what it was if the hit was dead-on
		// and nearly exactly what it was if this is a glancing blow
		ballV = reflectVec * (current_speed * (((1 - vel_angle) * 0.5) + 0.5));

		// Since we have a collision, the ball is already a little bit buried in
		// the wall. This calculates a vector needed to move it so that it is
		// exactly touching the wall
		auto disp = entry->get_surface_point(window->get_render()) -
			    entry->get_interior_point(window->get_render());
		auto newPos = ball_root.get_pos() + disp;
		ball_root.set_pos(newPos);
	}
}

// This function handles the collision between the ray and the ground
// Information about the interaction is passed in entry
void ground_collide_handler(PT(CollisionEntry) entry) {
	using namespace Globals;

        // Set the ball to the appropriate Z value for it to be exactly on the
        // ground
	double newZ = entry->get_surface_point(window->get_render()).get_z();
	ball_root.set_z(newZ + 0.4);

        // Find the acceleration direction. First the surface normal is crossed with
        // the up vector to get a vector perpendicular to the slope
	auto norm = entry->get_surface_normal(window->get_render());
	auto accelSide = norm.cross(LVector3::up());

        // Then that vector is crossed with the surface normal to get a vector that
        // points down the slope. By getting the acceleration in 3D like this rather
        // than in 2D, we reduce the amount of error per-frame, reducing jitter
	accelV = norm.cross(accelSide);
}

// If the ball hits a hole trigger, then it should fall in the hole.
// This is faked rather than dealing with the actual physics of it.
void lose_game(PT(CollisionEntry) entry) {
	using namespace Globals;

        // The triggers are set up so that the center of the ball should move to the
        // collision point to be in the hole
	auto to_pos = entry->get_interior_point(window->get_render());

	AsyncTaskManager::get_global_ptr()->remove(roll_task);  // Stop the maze task

        // Move the ball into the hole over a short sequence of time. Then wait a
        // second and call new_game to reset the game
	auto drain_task = new AsyncTaskSequence("drain_task");
	// The Python code uses LerpFunc, but there is no equivalent in C++.
	// That would require deriving a new LerpInterpolator.  In general,
	// using an interpolator is too complex for this simple demo.  Instead,
	// linear interpolation is done manually.
	struct udata {
		LPoint3 to_pos;
		LVector3 pos_diff;
		double start;
	} *drain_data = new udata;
	to_pos[2] = ball_root.get_z() - 0.9;
	drain_data->to_pos = to_pos;
	drain_data->pos_diff = to_pos - ball_root.get_pos();
	drain_data->start = ClockObject::get_global_clock()->get_real_time();
	drain_task->add_task(new GenericAsyncTask("drain_anim", [](GenericAsyncTask*, void*data) {
		udata *drain_data = reinterpret_cast<udata *>(data);
		auto cur = ClockObject::get_global_clock()->get_real_time() - drain_data->start;
		if(cur > .2) {
			delete drain_data;
			return AsyncTask::DS_done;
		}
		auto new_pos = drain_data->to_pos;
		auto &pos_diff = drain_data->pos_diff;
		new_pos[2] -= pos_diff[2] - pos_diff[2] * cur / .2;
		if(cur < .1) {
			new_pos[0] -= pos_diff[0] - pos_diff[0] * cur / .1;
			new_pos[1] -= pos_diff[1] - pos_diff[1] * cur / .1;
		}
		ball_root.set_pos(new_pos);
		return AsyncTask::DS_cont;
	}, drain_data));
	auto finish_task = new GenericAsyncTask("drain_finish", [](GenericAsyncTask*, void*){
		new_game();
		return AsyncTask::DS_done;
	}, 0);
	finish_task->set_delay(1.0);
	drain_task->add_task(finish_task);
	AsyncTaskManager::get_global_ptr()->add(drain_task);
}

AsyncTask::DoneStatus roll_func(GenericAsyncTask* task, void* mouseWatcherNode) {
	using namespace Globals;

        // Python's ShowBase has a task that traverses the entire scene once
	// a frame.  There is no equivalent in framework, so use this task,
	// which also gets called once a frame.
	collision_traverser.traverse(window->get_render());

        // Standard technique for finding the amount of time since the last
        // frame
	double dt = ClockObject::get_global_clock()->get_dt();

        // If dt is large, then there has been a # hiccup that could cause the ball
        // to leave the field if this functions runs, so ignore the frame
	if (dt > 0.2) {
		return AsyncTask::DS_cont;
	}

        // The collision handler collects the collisions. We dispatch which function
        // to handle the collision based on the name of what was collided into
	if (collision_handler->get_num_entries() > 0) {
		collision_handler->sort_entries(); // tjm -- why?  python demo doesn't sort
		for (size_t i = 0; i < collision_handler->get_num_entries(); i++) {
			PT(CollisionEntry) entry = collision_handler->get_entry(i);
			const std::string &name = entry->get_into_node()->get_name();
			if (name == "wall_collide") {
				wall_collide_handler(entry);
			} else if (name == "ground_collide") {
				ground_collide_handler(entry);
			} else if (name == "loseTrigger") {
				lose_game(entry);
			}
		}
	}

	// Read the mouse position and tilt the maze accordingly
	PT(MouseWatcher) mouseWatcher = DCAST(MouseWatcher, (PandaNode*)mouseWatcherNode);
	if (mouseWatcher->has_mouse()) {
		auto mpos = mouseWatcher->get_mouse(); //Get the mouse position
		maze.set_p(mpos.get_y() * -10);
		maze.set_r(mpos.get_x() * 10);
	}

	// Finally, we move the ball
        // Update the velocity based on acceleration
	ballV += accelV * dt * ACCELERATION;
        // Clamp the velocity to the maximum speed
	if (ballV.length_squared() > MAX_SPEED_SQ) {
		ballV.normalize();
		ballV *= MAX_SPEED;
	}
        // Update the position based on the velocity
	ball_root.set_pos(ball_root.get_pos() + (ballV * dt));

        // This block of code rotates the ball. It uses something called a quaternion
        // to rotate the ball around an arbitrary axis. That axis perpendicular to
        // the balls rotation, and the amount has to do with the size of the ball
        // This is multiplied on the previous rotation to incrimentally turn it.
	auto prevRot(ball.get_quat());
	auto axis = LVector3::up().cross(ballV);
	LRotation newRot(axis, 45.5 * dt * ballV.length());
	ball.set_quat(prevRot * newRot);

	return AsyncTask::DS_cont;       // Continue the task indefinitely
}
