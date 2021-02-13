#include "pandaFramework.h"
#include "pandaSystem.h"
#include "collisionNode.h"
#include "collisionRay.h"
#include "collisionTraverser.h"
#include "collisionHandlerQueue.h"
#include "mouseWatcher.h"
#include "ambientLight.h"
#include "directionalLight.h"

#include "header/functions.h"

std::string samplePath = "./";
std::vector<NodePath> lose_triggers;

int main(int argc, char* argv[]) {
	using namespace Globals;

	//Check if any arguments was given at startup
	if (argc > 1) {
		if (argv[1] == "-vs") {
			samplePath = "../../../";
		}
	}

	//Open framework
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

	//Material for the ball
	PT(Material) material = new Material();
	material->set_specular(LColor(1, 1, 1, 1));
	material->set_shininess(96);
	ball_root.set_material(material, 1);


	//More initialization
	ball_root.set_pos(maze.find("**/start").get_pos());
	PT(GenericAsyncTask) roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
	AsyncTaskManager::get_global_ptr()->add(roll_task);
	
	window->get_render().ls();

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}