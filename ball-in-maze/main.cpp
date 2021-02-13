#include "pandaFramework.h"
#include "pandaSystem.h"
#include "collisionNode.h"
#include "collisionRay.h"
#include "collisionTraverser.h"
#include "collisionHandlerQueue.h"
#include "mouseWatcher.h"
#include "ambientLight.h"
#include "directionalLight.h"

std::string samplePath = "./";
WindowFramework* window;
PT(CollisionHandlerQueue) collision_handler;
NodePath maze = NodePath("maze");
NodePath ball = NodePath("ball");
NodePath ball_root = NodePath("ball_root");
LVector3 ballV(0, 0, 0);				//Initial velocity is 0
LVector3 accelV(0, 0, 0);				//Initial acceleration is 0
std::vector<NodePath> lose_triggers;

//Some constants for the program
int ACCELERATION = 70;							//Acceleration in ft/sec/sec
int MAX_SPEED = 5;								//Max speed in ft/sec
int MAX_SPEED_SQ = 25;		//MAX_SPEED squared

//Exit function
void panda_exit(const Event* theEvent, void* data) {
	exit(0);
}

void wall_collide_handler(PT(CollisionEntry) entry) {
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
	double newZ = entry->get_surface_point(window->get_render()).get_z();
	ball_root.set_z(newZ + 0.4);

	LVector3 norm = entry->get_surface_normal(window->get_render());
	LVector3 accelSide = norm.cross(LVector3::up());

	accelV = norm.cross(accelSide);
	printf("Ground collide handler called!\n");
}

//Roll task
AsyncTask::DoneStatus roll_func(GenericAsyncTask* task, void* mouseWatcherNode) {
	double dt = ClockObject::get_global_clock()->get_dt();
	PT(MouseWatcher) mouseWatcher = DCAST(MouseWatcher, (PandaNode*)mouseWatcherNode);
	if (dt > 0.2) {
		return AsyncTask::DoneStatus::DS_cont;
	}

	if (collision_handler->get_num_entries() > 0) {
		collision_handler->sort_entries();
		for (size_t i = 0; i < collision_handler->get_num_entries(); i++) {
			PT(CollisionEntry) entry = collision_handler->get_entry(i);
			std::string name = entry->get_into_node()->get_name();
			std::cout << name << std::endl;
			if (name == "wall_collide") {
				wall_collide_handler(entry);
			} else if (name == "ground_collide") {
				ground_collide_handler(entry);
			} else if (name == "loseTrigger") {
				//lose_game(entry);
			}
		}
	} else {
		printf("No entries!\n");
	}

	if (mouseWatcher->has_mouse()) {
		LPoint2 mpos = mouseWatcher->get_mouse(); //Get the mouse position
		maze.set_p(mpos.get_y() * -10);
		maze.set_r(mpos.get_x() * 10);
		ball_root.set_x(ball_root.get_x() + mpos.get_x());
		ball_root.set_y(ball_root.get_y() + mpos.get_y());
	}

	ballV += accelV * dt * ACCELERATION;
	if (ballV.length_squared() > MAX_SPEED_SQ) {
		ballV.normalize();
		ballV *= MAX_SPEED;
	}
	//ball_root.set_pos(ball_root.get_pos() + (ballV * dt));		//Update the position based on the velocity
	LRotation prevRot(ball.get_quat());
	LVector3 axis = LVector3::up().cross(ballV);
	LRotation newRot(axis, 45.5 * dt * ballV.length());
	ball.set_quat(prevRot * newRot);

	std::cout << ball_root.get_pos() << std::endl;

	return AsyncTask::DS_cont;
}

int main(int argc, char* argv[]) {

	//Check if any arguments was given at startup
	if (argc > 1) {
		if (argv[1] == "-vs") {
			samplePath = "../../../";
		}
	}

	//Open framework
	PandaFramework framework;
	framework.open_framework(argc, argv);

	//Set the window title and open new window
	framework.set_window_title("Ball in maze - C++ Panda3D Samples");
	window = framework.open_window();
	framework.show_collision_solids(window->get_render());

	//Keyboard definitions
	window->enable_keyboard();
	framework.define_key("escape", "Escape-key", panda_exit, 0);

	//Camera
	NodePath camera = window->get_camera_group();
	camera.set_pos_hpr(0, 0, 25, 0, -90, 0);		//Position the camera


	//Maze
	maze = window->load_model(framework.get_models(), samplePath + "models/maze");
	maze.reparent_to(window->get_render());

	//Walls
	NodePath walls = maze.find("**/wall_collide");										//Find the collision node named wall_collide
	DCAST(CollisionNode, walls.node())->set_into_collide_mask(BitMask32::bit(0));		//Everything the ball should collide with will include bit 0

	//Setup lose triggers
	for (size_t i = 0; i < 6; i++) {
		NodePath trigger = maze.find("**/hole_collide" + std::to_string(i));
		trigger.node()->set_into_collide_mask(BitMask32::bit(0));
		trigger.node()->set_name("loseTrigger");
		lose_triggers.push_back(trigger);
		trigger.show();
	}

	//Maze ground
	NodePath maze_ground = maze.find("**/ground_collide");
	DCAST(CollisionNode, maze_ground.node())->set_into_collide_mask(BitMask32::bit(1));

	//Ball
	ball_root = window->get_render().attach_new_node("ball_root");
	ball = window->load_model(framework.get_models(), samplePath + "models/ball");
	ball.reparent_to(ball_root);

	//Ball sphere
	NodePath ball_sphere = ball.find("**/ball");
	DCAST(CollisionNode, ball_sphere.node())->set_from_collide_mask(BitMask32::bit(0));
	DCAST(CollisionNode, ball_sphere.node())->set_into_collide_mask(BitMask32::all_off());

	//Ball ground ray
	PT(CollisionRay) ball_ground_ray = new CollisionRay();		//Create the ray
	ball_ground_ray->set_origin(0, 0, 10);						//Set its origin
	ball_ground_ray->set_direction(0, 0, -1);					//And its direction

	//Ball ground col
	PT(CollisionNode) ball_ground_col = new CollisionNode("ball_ground_col");
	ball_ground_col->add_solid(ball_ground_ray);				//Add the ray
	ball_ground_col->set_from_collide_mask(BitMask32::bit(1));	//Set its bitmasks
	ball_ground_col->set_into_collide_mask(BitMask32::all_off());
	NodePath ball_ground_col_NP = ball_root.attach_new_node(ball_ground_col);

	ball_ground_col_NP.show();

	//Collision
	CollisionTraverser collision_traverser;
	collision_handler = new CollisionHandlerQueue();
	collision_traverser.add_collider(ball_sphere, collision_handler);
	collision_traverser.add_collider(ball_ground_col_NP, collision_handler);
	collision_traverser.show_collisions(window->get_render());

	//Lighting
	PT(AmbientLight) alight = new AmbientLight("alight");
	alight->set_color(LColor(0.55, 0.55, 0.55, 1));

	PT(DirectionalLight) dlight = new DirectionalLight("dlight");
	dlight->set_direction(LVector3(0, 0, -1));
	dlight->set_color(LColor(0.375, 0.375, 0.375, 1));
	dlight->set_specular_color(LColor(1, 1, 1, 1));

	ball_root.set_light(window->get_render().attach_new_node(alight));
	ball_root.set_light(window->get_render().attach_new_node(dlight));

	ball_root.set_pos(maze.find("**/start").get_pos());

	PT(GenericAsyncTask) roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
	AsyncTaskManager::get_global_ptr()->add(roll_task);
	
	window->get_render().ls();

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}