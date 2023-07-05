/*
 * Translation of Python ball-in-maze sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/ball-in-maze
 *
 * Original C++ conversion by IsakTheHacker March 2021.
 * Additional code changes by Thomas J. Moore June 2023:
 *  - On-screen text, drain animation, code comments added from Python version
 *  - Reorganized into single C++ file to match Python version
 *  - Restyled to match my taste a bit more (in particular, less space)
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang, Phil Saltzman
 * Last Updated: 2015-03-13
 *
 * This tutorial shows how to detect and respond to collisions. It uses solids
 * create in code and the egg files, how to set up collision masks, a traverser,
 * and a handler, how to detect collisions, and how to dispatch function based
 * on the collisions. All of this is put together to simulate a labyrinth-style
 * game
 */

// Panda3D includes
#include <panda3d/pandaFramework.h>
#include <panda3d/collisionHandlerQueue.h>
#include <panda3d/collisionRay.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/mouseWatcher.h>

#include "anim_supt.h"

// Some constants for the program
namespace { // don't export/pollute the global namespace
const int ACCEL = 70;	//Acceleration in ft/sec/sec
const int MAX_SPEED = 5;	//Max speed in ft/sec
const int MAX_SPEED_SQ = 25;	//Squared to make it easier to use length_squared
				// Instead of length

// Global variables.  The Python sample stored these in the class; I am not
// using a class here.  Since it's not a class, C++ doesn't do forward
// references, so they are all declared up here.
PandaFramework framework;
WindowFramework* window;
LVector3 ball_v(0, 0, 0);
LVector3 accel_v(0, 0, 0);
PT(CollisionHandlerQueue) collision_handler;
CollisionTraverser collision_traverser;
PT(GenericAsyncTask) roll_task;
NodePath maze, ball, ball_root;
std::vector<NodePath> lose_triggers;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;

// Forward decl
void start();
void wall_collide_handler(PT(CollisionEntry) entry);
void ground_collide_handler(PT(CollisionEntry) entry);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Ball in maze - C++ Panda3D Samples");
    window = framework.open_window();

    // This code puts the standard title and instruction text on screen
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    TextNode *text_node = new TextNode("title");
    auto text = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Collision Detection");
    text.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(1.0-0.1, 0, -1+0.1); // BottomRight == (1,0,-1)
    text.set_scale(0.08);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText

    text_node = new TextNode("instructions");
    text = NodePath(text_node);
    text_node->set_text("Mouse pointer tilts the board");
    text.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_left);
    text.set_pos(-1+0.05, 0, 1-0.08); // TopLeft == (-1,0,1)
    text_node->set_text_color(1, 1, 1, 1);
    text.set_scale(0.06);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText

    // Keyboard definitions
    window->enable_keyboard();
#if 0
    // Escape quits
    framework.define_key("escape", "Quit", framework.event_esc, &framework);
#else
    // Or, default key bindings.  In particular, ? displays bindings; ESC quits
    // This adds debugging keys.
    framework.enable_default_keys();
#endif

    // Camera
    // Mouse was never enabled, so no need to disable.  Just set the camera.
    // framework.disable_mouse();
    NodePath camera = window->get_camera_group();
    camera.set_pos_hpr(0, 0, 25, 0, -90, 0);  // Place the camera

    // Load the maze and place it in the scene
    maze = def_load_model("models/maze");
    maze.reparent_to(window->get_render());

    // Most times, you want collisions to be tested against invisible geometry
    // rather than every polygon. This is because testing against every polygon
    // in the scene is usually too slow. You can have simplified or approximate
    // geometry for the solids and still get good results.
    //
    // Sometimes you'll want to create and position your own collision solids in
    // code, but it's often easier to have them built automatically. This can be
    // done by adding special tags into an egg file. Check maze.egg and ball.egg
    // and look for lines starting with <Collide>. The part in brackets tells
    // Panda exactly what to do. Polyset means to use the polygons in that group
    // as solids, while Sphere tells panda to make a collision sphere around them
    // Keep means to keep the polygons in the group as visable geometry (good
    // for the ball, not for the triggers), and descend means to make sure that
    // the settings are applied to any subgroups.
    //
    // Once we have the collision tags in the models, we can get to them using
    // NodePath's find command

    // Find the collision node named wall_colllide
    NodePath walls = maze.find("**/wall_collide");

    // Collision objects are sorted using BitMasks. BitMasks are ordinary numbers
    // with extra methods for working with them as binary bits. Every collision
    // solid has both a from mask and an into mask. Before Panda tests two
    // objects, it checks to make sure that the from and into collision masks
    // have at least one bit in common. That way things that shouldn't interact
    // won't. Normal model nodes have collision masks as well. By default they
    // are set to bit 20. If you want to collide against actual visable polygons,
    // set a from collide mask to include bit 20
    //
    // For this example, we will make everything we want the ball to collide with
    // include bit 0
    DCAST(CollisionNode, walls.node())->set_into_collide_mask(1<<0);
    // CollisionNodes are usually invisible but can be shown. Uncomment the next
    // line to see the collision walls
    //walls.show();

    // We will now find the triggers for the holes and set their masks to 0 as
    // well. We also set their names to make them easier to identify during
    // collisions
    for (size_t i = 0; i < 6; i++) {
	NodePath trigger = maze.find("**/hole_collide" + std::to_string(i));
	trigger.node()->set_into_collide_mask(1<<0);
	trigger.node()->set_name("lose_trigger");
	lose_triggers.push_back(trigger);
	// Uncomment this line to see the triggers
	//trigger.show();
    }

    // Ground_collide is a single polygon on the same plane as the ground in the
    // maze. We will use a ray to collide with it so that we will know exactly
    // what height to put the ball at every frame. Since this is not something
    // that we want the ball itself to collide with, it has a different
    // bitmask.
    NodePath maze_ground = maze.find("**/ground_collide");
    DCAST(CollisionNode, maze_ground.node())->set_into_collide_mask(1<<1);

    // Load the ball and attach it to the scene
    // It is on a root dummy node so that we can rotate the ball itself without
    // rotating the ray that will be attached to it
    ball_root = window->get_render().attach_new_node("ballRoot");
    ball = def_load_model("models/ball");
    ball.reparent_to(ball_root);

    // Find the collison sphere for the ball which was created in the egg file
    // Notice that it has a from collision mask of bit 0, and an into collison
    // mask of no bits. This means that the ball can only cause collisions, not
    // be collided into
    NodePath ball_sphere = ball.find("**/ball");
    DCAST(CollisionNode, ball_sphere.node())->set_from_collide_mask(1<<0);
    DCAST(CollisionNode, ball_sphere.node())->set_into_collide_mask(0);

    // Now we create a ray to start above the ball and cast down. This is to
    // Determine the height the ball should be at and the angle the floor is
    // tilting. We could have used the sphere around the ball itself, but it
    // would not be as reliable
    PT(CollisionRay) ball_ground_ray = new CollisionRay();	//Create the ray
    ball_ground_ray->set_origin(0, 0, 10);			//Set its origin
    ball_ground_ray->set_direction(0, 0, -1);		//And its direction
    // Collision solids go in CollisionNode
    // Create and name the node
    PT(CollisionNode) ball_ground_col = new CollisionNode("groundRay");
    ball_ground_col->add_solid(ball_ground_ray);	//Add the ray
    ball_ground_col->set_from_collide_mask(1<<1);	//Set its bitmasks
    ball_ground_col->set_into_collide_mask(0);
    // Attach the node to the ballRoot so that the ray is relative to the ball
    // (it will always be 10 feet over the ball and point down)
    NodePath ball_ground_col_NP = ball_root.attach_new_node(ball_ground_col);
    // Uncomment this line to see the ray
    //ball_ground_col_NP.show();

    // Finally, we create a CollisionTraverser. CollisionTraversers are what
    // do the job of walking the scene graph and calculating collisions.
    // For a traverser to actually do collisions, you need to call
    // traverser.traverse() on a part of the scene.  For this example, this
    // is done by a task called roll_task, which does this for the entire
    // scene once a frame.  This automatically/only uses collision_traverser,
    // defined above in globals
    //CollisionTraverser collistion_traverser;

    // Collision traversers tell collision handlers about collisions, and then
    // the handler decides what to do with the information. We are using a
    // CollisionHandlerQueue, which simply creates a list of all of the
    // collisions in a given pass. There are more sophisticated handlers like
    // one that sends events and another that tries to keep collided objects
    // apart, but the results are often better with a simple queue
    collision_handler = new CollisionHandlerQueue();

    // Now we add the collision nodes that can create a collision to the
    // traverser. The traverser will compare these to all others nodes in the
    // scene. There is a limit of 32 CollisionNodes per traverser
    // We add the collider, and the handler to use as a pair
    collision_traverser.add_collider(ball_sphere, collision_handler);
    collision_traverser.add_collider(ball_ground_col_NP, collision_handler);

    // Collision traversers have a built in tool to help visualize collisions.
    // Uncomment the next line to see it.
    //collision_traverser.show_collisions(window->get_render());

    // This section deals with lighting for the ball. Only the ball was lit
    // because the maze has static lighting pregenerated by the modeler
    PT(AmbientLight) ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(0.55, 0.55, 0.55, 1));

    PT(DirectionalLight) directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(0, 0, -1));
    directional_light->set_color(LColor(0.375, 0.375, 0.375, 1));
    directional_light->set_specular_color(LColor(1, 1, 1, 1));
    ball_root.set_light(window->get_render().attach_new_node(ambient_light));
    ball_root.set_light(window->get_render().attach_new_node(directional_light));

    // This section deals with adding a specular highlight to the ball to make
    // it look shiny.  Normally, this is specified in the .egg file.
    PT(Material) material = new Material();
    material->set_specular(LColor(1, 1, 1, 1));
    material->set_shininess(96);
    ball_root.set_material(material, 1);

    // Finally, we call start for more initialization
    start();
}

// Forward decl
AsyncTask::DoneStatus roll_func(GenericAsyncTask *, void *);

void start()
{
    // The maze model also has a locator in it for where to start the ball
    // To access it we use the find command
    auto start_pos = maze.find("**/start").get_pos();
    // Set the ball in the starting position
    ball_root.set_pos(start_pos);
    ball_v.set(0, 0, 0);         // Initial velocity is 0
    accel_v.set(0, 0, 0);        // Initial acceleration is 0

    // Create the movement task
    // It is stopped before calling start() again, so no need to make sure it's
    // not running
    //framework.get_task_mgr().remove(find_task("roll_task"));
    // while window is a global, passing the mouse node down avoids this
    // function call every time the task is executed.
    roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
    framework.get_task_mgr().add(roll_task);
}

// This function handles the collision between the ray and the ground
// Information about the interaction is passed in entry
void ground_collide_handler(PT(CollisionEntry) entry)
{
    // Set the ball to the appropriate Z value for it to be exactly on the
    // ground
    double newZ = entry->get_surface_point(window->get_render()).get_z();
    ball_root.set_z(newZ + 0.4);

    // Find the acceleration direction. First the surface normal is crossed with
    // the up vector to get a vector perpendicular to the slope
    auto norm = entry->get_surface_normal(window->get_render());
    auto accel_side = norm.cross(LVector3::up());
    // Then that vector is crossed with the surface normal to get a vector that
    // points down the slope. By getting the acceleration in 3D like this rather
    // than in 2D, we reduce the amount of error per-frame, reducing jitter
    accel_v = norm.cross(accel_side);
}

// This function handles the collision between the ball and a wall
void wall_collide_handler(PT(CollisionEntry) entry)
{
    // First we calculate some numbers we need to do a reflection
    // The normal of the wall
    auto norm = entry->get_surface_normal(window->get_render()) * -1;
    double cur_speed = ball_v.length();	// The current speed
    auto in_vec = ball_v / cur_speed;	// The direction of travel
    double vel_angle = norm.dot(in_vec);	// Angle of incidence
    auto hit_dir = entry->get_surface_point(window->get_render()) - ball_root.get_pos();
    hit_dir.normalize();
    // The angle between the ball and the normal
    double hit_angle = norm.dot(hit_dir);

    // Ignore the collision if the ball is either moving away from the wall
    // already (so that we don't accidentally send it back into the wall)
    // and ignore it if the collision isn't dead-on (to avoid getting caught on
    // corners)
    if (vel_angle > 0 && hit_angle > 0.995) {
	// Standard reflection equation
	auto relfect_vec = (norm * norm.dot(in_vec * -1) * 2) + in_vec;

	// This makes the velocity half of what it was if the hit was dead-on
	// and nearly exactly what it was if this is a glancing blow
	ball_v = relfect_vec * (cur_speed * (((1 - vel_angle) * 0.5) + 0.5));
	// Since we have a collision, the ball is already a little bit buried in
	// the wall. This calculates a vector needed to move it so that it is
	// exactly touching the wall
	auto disp = entry->get_surface_point(window->get_render()) -
	    entry->get_interior_point(window->get_render());
	auto newPos = ball_root.get_pos() + disp;
	ball_root.set_pos(newPos);
    }
}

void lose_game(PT(CollisionEntry) entry);

// This is the task that deals with making everything interactive
AsyncTask::DoneStatus roll_func(GenericAsyncTask* task, void* mouse_watcher_node)
{
    // Python's ShowBase has a task that traverses the entire scene once
    // a frame.  There is no equivalent in framework, so use this task,
    // which also gets called once a frame.
    collision_traverser.traverse(window->get_render());

    // Standard technique for finding the amount of time since the last
    // frame
    double dt = ClockObject::get_global_clock()->get_dt();

    // If dt is large, then there has been a # hiccup that could cause the ball
    // to leave the field if this functions runs, so ignore the frame
    if (dt > 0.2)
	return AsyncTask::DS_cont;

    // The collision handler collects the collisions. We dispatch which function
    // to handle the collision based on the name of what was collided into
    if (collision_handler->get_num_entries() > 0) {
	collision_handler->sort_entries(); // tjm -- why?  python demo doesn't sort
	for (size_t i = 0; i < collision_handler->get_num_entries(); i++) {
	    PT(CollisionEntry) entry = collision_handler->get_entry(i);
	    const std::string &name = entry->get_into_node()->get_name();
	    if (name == "wall_collide")
		wall_collide_handler(entry);
	    else if (name == "ground_collide")
		ground_collide_handler(entry);
	    else if (name == "lose_trigger")
		lose_game(entry);
	}
    }

    // Read the mouse position and tilt the maze accordingly
    PT(MouseWatcher) mouse_watcher = DCAST(MouseWatcher, (PandaNode*)mouse_watcher_node);
    if (mouse_watcher->has_mouse()) {
	auto mpos = mouse_watcher->get_mouse(); //Get the mouse position
	maze.set_p(mpos.get_y() * -10);
	maze.set_r(mpos.get_x() * 10);
    }

    // Finally, we move the ball
    // Update the velocity based on acceleration
    ball_v += accel_v * dt * ACCEL;
    // Clamp the velocity to the maximum speed
    if (ball_v.length_squared() > MAX_SPEED_SQ) {
	ball_v.normalize();
	ball_v *= MAX_SPEED;
    }
    // Update the position based on the velocity
    ball_root.set_pos(ball_root.get_pos() + (ball_v * dt));

    // This block of code rotates the ball. It uses something called a quaternion
    // to rotate the ball around an arbitrary axis. That axis perpendicular to
    // the balls rotation, and the amount has to do with the size of the ball
    // This is multiplied on the previous rotation to incrimentally turn it.
    auto prevRot(ball.get_quat());
    auto axis = LVector3::up().cross(ball_v);
    LRotation newRot(axis, 45.5 * dt * ball_v.length());
    ball.set_quat(prevRot * newRot);

    return AsyncTask::DS_cont;       // Continue the task indefinitely
}

// If the ball hits a hole trigger, then it should fall in the hole.
// This is faked rather than dealing with the actual physics of it.
void lose_game(PT(CollisionEntry) entry)
{
    // The triggers are set up so that the center of the ball should move to the
    // collision point to be in the hole
    auto to_pos = entry->get_interior_point(window->get_render());

    framework.get_task_mgr().remove(roll_task);  // Stop the maze task

    // Move the ball into the hole over a short sequence of time. Then wait a
    // second and call start to reset the game
    (new Sequence({
	new Parallel({
	    new LerpFunc(&ball_root, &NodePath::set_x, ball_root.get_x(),
			 to_pos.get_x(), .1),
	    new LerpFunc(&ball_root, &NodePath::set_y, ball_root.get_y(),
			 to_pos.get_y(), .1),
	    new LerpFunc(&ball_root, &NodePath::set_z, ball_root.get_z(),
			 (PN_stdfloat)(ball_root.get_z() - .9), .2)
	}),
	new Wait(1),
	new Func(start())
    }))->start();
}
}

int main(int argc, char* argv[]) {
    if(argc > 1)
	sample_path = argv[1]; // old C++ sample had -vs for ../../.., but I don't care

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
