#include "pandaFramework.h"
#include "pandaSystem.h"
#include "collisionNode.h"
#include "collisionRay.h"
#include "collisionTraverser.h"
#include "collisionHandlerQueue.h"

std::string samplePath = "./";

//Some constants for the program
int ACCELERATION = 70;							//Acceleration in ft/sec/sec
int MAX_SPEED = 5;								//Max speed in ft/sec
int MAX_SPEED_SQ = MAX_SPEED * MAX_SPEED;		//MAX_SPEED squared

//Exit function
void panda_exit(const Event* theEvent, void* data) {
	exit(0);
}

//Roll task
AsyncTask::DoneStatus roll_task(GenericAsyncTask* task, void* data) {
	double dt = ClockObject::get_global_clock()->get_dt();
	if (dt > 0.2) {
		return AsyncTask::DS_cont;
	}
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
	WindowFramework* window = framework.open_window();

	//Keyboard definitions
	window->enable_keyboard();
	framework.define_key("escape", "Escape-key", panda_exit, 0);


	//Load models
	NodePath maze = window->load_model(framework.get_models(), samplePath + "models/maze");
	maze.reparent_to(window->get_render());

	NodePath walls = maze.find("**/wall_collide");				//Find the collision node named wall_collide
	walls.node()->set_into_collide_mask(BitMask32::bit(0));		//Everything the ball should collide with will include bit 0

	NodePath maze_ground = maze.find("**/ground_collide");
	maze_ground.node()->set_into_collide_mask(BitMask32::bit(1));

	//Ball
	NodePath ball_root = window->get_render().attach_new_node("ball_root");
	NodePath ball = window->load_model(framework.get_models(), samplePath + "models/ball");
	ball.reparent_to(ball_root);

	NodePath ball_sphere = ball.find("**/ball");
	DCAST(CollisionNode, ball_sphere.node())->set_from_collide_mask(BitMask32::bit(0));
	DCAST(CollisionNode, ball_sphere.node())->set_into_collide_mask(BitMask32::all_off());

	PT(CollisionRay) ball_ground_ray = new CollisionRay();		//Create the ray
	ball_ground_ray->set_origin(0, 0, 10);						//Set its origin
	ball_ground_ray->set_direction(0, 0, -1);					//And its direction

	PT(CollisionNode) ball_ground_col = new CollisionNode("ball_ground_col");
	ball_ground_col->add_solid(ball_ground_ray);				//Add the ray
	ball_ground_col->set_from_collide_mask(BitMask32::bit(1));	//Set its bitmasks
	ball_ground_col->set_into_collide_mask(BitMask32::all_off());
	NodePath ball_ground_col_NP = ball_root.attach_new_node(ball_ground_col);

	//Collision
	CollisionTraverser collision_traverser;
	PT(CollisionHandlerQueue) collision_handler = new CollisionHandlerQueue();
	collision_traverser.add_collider(ball_sphere, collision_handler);
	collision_traverser.add_collider(ball_ground_col_NP, collision_handler);

	ball_root.set_pos(maze.find("**/start").get_pos());
	LVector3 ballV(0, 0, 0);				//Initial velocity is 0
	LVector3 accelV(0, 0, 0);				//Initial acceleration is 0

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}