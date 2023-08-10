/*
 *  Translation of Python roaming-ralph sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/roaming-ralph
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Ryan Myers
 * Models: Jeff Styers, Reagan Heller
 *
 * Last Updated: 2015-03-13
 *
 * This tutorial provides an example of creating a character
 * and having it walk around on uneven terrain, as well
 * as implementing a fully rotatable camera.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/collisionHandlerQueue.h>
#include <panda3d/collisionRay.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
NodePath ralph, floater;
PT(AnimControl) run, walk;
CollisionTraverser c_trav;
PT(CollisionHandlerQueue) ralph_ground_handler, cam_ground_handler;
// Game state variables
bool is_moving = false;
// This is used to store which keys are currently pressed.
enum k_t { k_left, k_right, k_forward, k_cam_left, k_cam_right, k_num };
bool key_map[k_num];

// Function to put instructions on the screen.
void add_instructions(PN_stdfloat pos, const std::string &msg)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("instructions");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text.set_scale(0.05);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text.set_pos(-1/a2d.get_sx() + 0.08, 0, 1 - pos - 0.04); // TopLeft = (-1, 0, 1)
    text_node->set_align(TextNode::A_left);
}

// Function to put title on the screen.
void add_title(const std::string &text)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto node = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    node.set_scale(0.07);
    text_node->set_align(TextNode::A_right);
    node.set_pos(1/a2d.get_sx()-0.1, 0, -1 + 0.09); // BottomRight == (1, 0, -1)
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

AsyncTask::DoneStatus move(GenericAsyncTask *, void *);

void init(void)
{
    // Set up the window, camera, etc.
    framework.open_framework();
    framework.set_window_title("Roaming Ralph - C++ Panda3D Samples");
    window = framework.open_window();

    // Set the background color to black
    window->set_background_type(WindowFramework::BT_black);

    // Post the instructions
    add_title(
            "Panda3D Tutorial: Roaming Ralph (Walking on Uneven Terrain)");
    add_instructions(0.06, "[ESC]: Quit");
    add_instructions(0.12, "[Left Arrow]: Rotate Ralph Left");
    add_instructions(0.18, "[Right Arrow]: Rotate Ralph Right");
    add_instructions(0.24, "[Up Arrow]: Run Ralph Forward");
    add_instructions(0.30, "[A]: Rotate Camera Left");
    add_instructions(0.36, "[S]: Rotate Camera Right");

    // Set up the environment
    //
    // This environment model contains collision meshes.  If you look
    // in the egg file, you will see the following:
    //
    //    <Collide> { Polyset keep descend }
    //
    // This tag causes the following mesh to be converted to a collision
    // mesh -- a mesh which is optimized for collision, not rendering.
    // It also keeps the original mesh, so there are now two copies ---
    // one optimized for rendering, one for collisions.

    auto render = window->get_render();
    auto environ = def_load_model("models/world");
    environ.reparent_to(render);

    // Create the main character, Ralph

    auto ralph_start_pos = environ.find("**/start_point").get_pos();
    ralph = def_load_model("models/ralph");
    run = load_anim(ralph, "models/ralph-run");
    walk = load_anim(ralph, "models/ralph-walk");
    ralph.reparent_to(render);
    ralph.set_scale(.2);
    ralph.set_pos(ralph_start_pos + LVector3(0, 0, 0.5));

    // Create a floater object, which floats 2 units above ralph.  We
    // use this as a target for the camera to look at.

    floater = NodePath("floater");
    floater.reparent_to(ralph);
    floater.set_z(2.0);

    // Accept the control keys for movement and rotation

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("arrow_left", "", EV_FN() { key_map[k_left] = true; }, 0);
    framework.define_key("arrow_right", "", EV_FN() { key_map[k_right] = true; }, 0);
    framework.define_key("arrow_up", "", EV_FN() { key_map[k_forward] = true; }, 0);
    framework.define_key("a", "", EV_FN() { key_map[k_cam_left] = true; }, 0);
    framework.define_key("s", "", EV_FN() { key_map[k_cam_right] = true; }, 0);
    framework.define_key("arrow_left-up", "", EV_FN() { key_map[k_left] = false; }, 0);
    framework.define_key("arrow_right-up", "", EV_FN() { key_map[k_right] = false; }, 0);
    framework.define_key("arrow_up-up", "", EV_FN() { key_map[k_forward] = false; }, 0);
    framework.define_key("a-up", "", EV_FN() { key_map[k_cam_left] = false; }, 0);
    framework.define_key("s-up", "", EV_FN() { key_map[k_cam_right] = false; }, 0);

    auto task = new GenericAsyncTask("move_task", move, 0);
    framework.get_task_mgr().add(task);

    // Set up the camera
    auto camera = window->get_camera_group();
    camera.set_pos(ralph.get_x(), ralph.get_y() + 10, 2);

    // We will detect the height of the terrain by creating a collision
    // ray and casting it downward toward the terrain.  One ray will
    // start above ralph's head, and the other will start above the camera.
    // A ray may hit the terrain, or it may hit a rock or a tree.  If it
    // hits the terrain, we can detect the height.  If it hits anything
    // else, we rule that the move is illegal.
    PT(CollisionRay) ralph_ground_ray = new CollisionRay;
    ralph_ground_ray->set_origin(0, 0, 9);
    ralph_ground_ray->set_direction(0, 0, -1);
    PT(CollisionNode) ralph_ground_col = new CollisionNode("ralph_ray");
    ralph_ground_col->add_solid(ralph_ground_ray);
    ralph_ground_col->set_from_collide_mask(1<<0);
    ralph_ground_col->set_into_collide_mask(0);
    auto ralph_ground_col_np = ralph.attach_new_node(ralph_ground_col);
    ralph_ground_handler = new CollisionHandlerQueue;
    c_trav.add_collider(ralph_ground_col_np, ralph_ground_handler);

    PT(CollisionRay) cam_ground_ray = new CollisionRay;
    cam_ground_ray->set_origin(0, 0, 9);
    cam_ground_ray->set_direction(0, 0, -1);
    PT(CollisionNode) cam_ground_col = new CollisionNode("cam_ray");
    cam_ground_col->add_solid(cam_ground_ray);
    cam_ground_col->set_from_collide_mask(1<<0);
    cam_ground_col->set_into_collide_mask(0);
    auto cam_ground_col_np = camera.attach_new_node(cam_ground_col);
    cam_ground_handler = new CollisionHandlerQueue;
    c_trav.add_collider(cam_ground_col_np, cam_ground_handler);

    // Uncomment this line to see the collision rays
    //ralph_ground_col_np.show();
    //cam_ground_col_np.show();

    // Uncomment this line to show a visual representation of the
    // collisions occuring
    //c_trav.show_collisions(render);

    // Create some lighting
    auto ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(.3, .3, .3, 1));
    auto directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(-5, -5, -5));
    directional_light->set_color(LColor(1, 1, 1, 1));
    directional_light->set_specular_color(LColor(1, 1, 1, 1));
    render.set_light(render.attach_new_node(ambient_light));
    render.set_light(render.attach_new_node(directional_light));
}

// Accepts arrow keys to move either the player or the menu cursor,
// Also deals with grid checking and collision detection
AsyncTask::DoneStatus move(GenericAsyncTask *, void *)
{
    // Get the time that elapsed since last frame.  We multiply this with
    // the desired speed in order to find out with which distance to move
    // in order to achieve that desired speed.
    auto dt = ClockObject::get_global_clock()->get_dt();

        // If the camera-left key is pressed, move camera left.
        // If the camera-right key is pressed, move camera right.

    auto camera = window->get_camera_group();
    if(key_map[k_cam_left])
	camera.set_x(camera, -20 * dt);
    if(key_map[k_cam_right])
	camera.set_x(camera, +20 * dt);

    // save ralph's initial position so that we can restore it,
    // in case he falls off the map or runs into something.

    auto startpos = ralph.get_pos();

    // If a move-key is pressed, move ralph in the specified direction.

    if(key_map[k_left])
	ralph.set_h(ralph.get_h() + 300 * dt);
    if(key_map[k_right])
	ralph.set_h(ralph.get_h() - 300 * dt);
    if(key_map[k_forward])
	ralph.set_y(ralph, -25 * dt);

    // If ralph is moving, loop the run animation.
    // If he is standing still, stop the animation.

    if(key_map[k_forward] || key_map[k_left] || key_map[k_right]) {
	if(!is_moving) {
	    run->loop(true);
	    is_moving = true;
        }
    } else if(is_moving) {
	run->stop();
	walk->pose(5);
	is_moving = false;
    }

    // If the camera is too far from ralph, move it closer.
    // If the camera is too close to ralph, move it farther.

    auto camvec = ralph.get_pos() - camera.get_pos();
    camvec.set_z(0);
    auto camdist = camvec.length();
    camvec.normalize();
    if(camdist > 10.0) {
	camera.set_pos(camera.get_pos() + camvec * (camdist - 10));
	camdist = 10.0;
    }
    if(camdist < 5.0) {
	camera.set_pos(camera.get_pos() - camvec * (5 - camdist));
	camdist = 5.0;
    }

    // This is only necessary in C++ because there is no automatic task
    // generated.
    auto render = window->get_render();
    c_trav.traverse(render);

    // Adjust ralph's Z coordinate.  If ralph's ray hit terrain,
    // update his Z. If it hit anything else, or didn't hit anything, put
    // him back where he was last frame.

    std::vector<CollisionEntry *> entries;
    // once again, get_num_entries() is signed when it should be unsigned
    for(int i = 0; i < ralph_ground_handler->get_num_entries(); i++)
	entries.push_back(ralph_ground_handler->get_entry(i));
    std::sort(entries.begin(), entries.end(),
	      [=] (CollisionEntry *&a, CollisionEntry *&b) {
		  return a->get_surface_point(render).get_z() <
		         b->get_surface_point(render).get_z();
	      });

    if(entries.size() > 0 && entries[0]->get_into_node()->get_name() == "terrain")
	ralph.set_z(entries[0]->get_surface_point(render).get_z());
    else
	ralph.set_pos(startpos);

    // Keep the camera at one foot above the terrain,
    // or two feet above ralph, whichever is greater.
    entries.clear();
    // once again, get_num_entries() is signed when it should be unsigned
    for(int i = 0; i < cam_ground_handler->get_num_entries(); i++)
	entries.push_back(cam_ground_handler->get_entry(i));
    std::sort(entries.begin(), entries.end(),
	      [=] (CollisionEntry *&a, CollisionEntry *&b) {
		  return a->get_surface_point(render).get_z() <
		         b->get_surface_point(render).get_z();
	      });

    if(entries.size() > 0 && entries[0]->get_into_node()->get_name() == "terrain")
	camera.set_z(entries[0]->get_surface_point(render).get_z() + 1.0);
    if(camera.get_z() < ralph.get_z() + 2.0)
	camera.set_z(ralph.get_z() + 2.0);

    // The camera should look in ralph's direction,
    // but it should also try to stay horizontal, so look at
    // a floater which hovers above ralph's head.
    camera.look_at(floater);

    return AsyncTask::DS_cont;
}
}

int main(int argc, char* argv[])
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1)
	get_model_path().prepend_directory(argv[1]);

    init();

    framework.main_loop();
    framework.close_framework();
    return 0;
}
