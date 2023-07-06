/*
 *  Translation of Python looking-and-gripping sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/looking-and-gripping
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang and Phil Saltzman
 * Models and Textures by: Shaun Budhram, Will Houng, and David Tucker
 * Last Updated: 2015-03-13
 *
 * This tutorial will cover exposing joints and manipulating them. Specifically,
 * we will take control of the neck joint of a humanoid character and rotate that
 * joint to always face the mouse cursor.  This will in turn make the head of the
 * character "look" at the mouse cursor.  We will also expose the hand joint and
 * use it as a point to "attach" objects that the character can hold.  By
 * parenting an object to a hand joint, the object will stay in the character's
 * hand even if the hand is moving through an animation.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/character.h>
#include <panda3d/characterJoint.h>
#include <panda3d/mouseWatcher.h>
#include <panda3d/modelNode.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
// The Python sample uses a class, which can have forward references even
// in C++.  I am using globals, though, as it saves a few headaches.  Thus,
// they need to all be delcared at the top.
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;
NodePath eve_neck, right_hand,
    models[4]; // A list that will store our models objects
PT(CInterval) anim;

// A simple function to make sure a value is in a given range, -1 to 1 by
// default
template<class T> T clamp(T i, T mn=(T)-1, T mx=(T)1)
{
    return std::min({std::max({i, mn}), mx});
}

// Macro-like function used to reduce the amount to code needed to create the
// on screen instructions
void gen_label_text(const char *text, int i)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("label_text");
    auto path = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    path.set_scale(0.06);
    path.set_pos(-1.0/a2d.get_sx() + 0.06, 0, 1.0 - 0.08 * i); // TopLeft == (-1,0,1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, .5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text_node->set_align(TextNode::A_left);
}

void switch_object(int i);
AsyncTask::DoneStatus turn_head(GenericAsyncTask* task, void* mouse_watcher_node);
void setup_lights(void);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Looking and Gripping - C++ Panda3D Samples");
    window = framework.open_window();

    // This code puts the standard title and instruction text on screen
    auto a2d = window->get_aspect_2d();
    auto text_node = new TextNode("title");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Joint Manipulation");
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_right);
    text.set_pos(1.0/a2d.get_sx()-0.1, 0, -1+0.1); // BottomRight == (1,0,-1)
    text_node->set_shadow_color(0, 0, 0, 0.5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text.set_scale(0.08);

    gen_label_text("ESC: Quit", 1);
    gen_label_text("[1]: Teapot", 2);
    gen_label_text("[2]: Candy cane", 3);
    gen_label_text("[3]: Banana", 4);
    gen_label_text("[4]: Sword", 5);

    // Set up key input
    window->enable_keyboard();
    framework.define_key("escape", "Quit", framework.event_esc, &framework);
    framework.define_key("1", "Teapot", EV_FN() { switch_object(0); }, 0);
    framework.define_key("2", "Candy cane", EV_FN() { switch_object(1); }, 0);
    framework.define_key("3", "Banana", EV_FN() { switch_object(2); }, 0);
    framework.define_key("4", "Sword", EV_FN() { switch_object(3); }, 0);

    // window->disable_mouse()  // Disable mouse-based camera-control
    window->get_camera_group().set_pos(0, -15, 2);  // Position the camera

    auto eve = def_load_model("models/eve");  // Load our animated charachter
    auto walk = load_anim(eve, sample_path + "models/eve_walk");
    eve.reparent_to(window->get_render());  // Put it in the scene

    // Now we use control_joint() to control her neck
    // This must be done before any animations are played
    // In Python, this is one line of code.  C++ requires re-implementation
    // of the same procedure:
    // First, create a node.
    eve_neck = framework.get_models().attach_new_node(new ModelNode("Neck"));
    // Next, initialize to the starting position.  This means finding the
    // starting position.
    // Scan for the character (should be first child)
    //auto eve_char = DCAST(Character, eve.find("+Character").node());
    auto eve_char = DCAST(Character, eve.get_child(0).node());
    // Scan for the joint in the Character's joint bundle
    auto neckj = eve_char->find_joint("Neck");
    // Grab the initial (default) matrix, and store it in the node.
    eve_neck.set_mat(neckj->get_default_value());
    // Set the control relationship
    eve_char->get_bundle(0)->control_joint("Neck", eve_neck.node());

    // We now play an animation. An animation must be played, or at least posed
    // for the nodepath we just got from control_joint to actually affect the
    // model
    (new CharAnimate(walk, 2))->loop();
    // Now we add a task that will take care of turning the head
    framework.get_task_mgr().add(new GenericAsyncTask("turn_head", turn_head,
						      (void*)window->get_mouse().node()));

    // Now we will expose the joint the hand joint. add_net_transform allows
    // us to get the position of a joint while it is animating. This is different
    // than control_jonit which stops that joint from animating but lets us move it.
    // This is particularly usefull for putting an object (like a weapon) in a
    // character's hand
    // Once again, this is one line in Python, but a little harder in C++.
    // First, create a node.
    right_hand = eve.attach_new_node(new ModelNode("RightHand"));
    // Scan for the joint in the Character's joint bundle; there is only 1 bundle
    auto right_handj = eve_char->find_joint("RightHand");
    // Set the control relationship
    right_handj->add_net_transform(right_hand.node());
    right_hand.set_mat(right_handj->get_value());

    // This is a table with models, positions, rotations, and scales of objects to
    // be attached to our exposed joint. These are stock models and so they needed
    // to be repositioned to look right.
    const struct {
	const char *name;
	LPoint3 pos;
	LVector3 rot;
	PN_stdfloat scale;
    } positions[] = {
	{"teapot", {0, -.66, -.95}, {90, 0, 90}, .4},
	{"models/candycane", {.15, -.99, -.22}, {90, 0, 90}, 1},
	{"models/banana", {.08, -.1, .09}, {0, -90, 0}, 1.75},
	{"models/sword", {.11, .19, .06}, {0, 0, 90}, 1}
    };

    for(int i = 0; i < 4; i++) {
	auto np = i ? def_load_model(positions[i].name) :  // Load the model
	    // the standard model doesn't get a path prefix
	    // instead of !i, I could've said !strchr(name, '/')
	    window->load_model(framework.get_models(), positions[i].name);
	np.set_pos(positions[i].pos);  // Position it
	np.set_hpr(positions[i].rot);  // Rotate it
	np.set_scale(positions[i].scale);  // Scale it
	// Reparent the model to the exposed joint. That way when the joint moves,
	// the model we just loaded will move with it.
	np.reparent_to(right_hand);
	models[i] = np;  // Add it to our models list
    }
    switch_object(0);  // Make object 0 the first shown
    setup_lights();  // Put in some default lighting
}

void switch_object(int i)
{
    // This is what we use to change which object it being held. It just hides all of
    // the objects and then unhides the one that was selected
    for(int m = 0; m < 4; m++)
	models[m].hide();
    models[i].show();
}

// This task gets the position of mouse each frame, and rotates the neck based
// on it.
AsyncTask::DoneStatus turn_head(GenericAsyncTask* task, void* mouse_watcher_node)
{
    // Check to make sure the mouse is readable
    auto mouse_watcher = DCAST(MouseWatcher, (PandaNode*)mouse_watcher_node);
    if(mouse_watcher->has_mouse()) {
	// get the mouse position as a LVector2. The values for each axis are from -1 to
	// 1. The top-left is (-1,-1), the bottom right is (1,1)
	auto mpos = mouse_watcher->get_mouse();
	// Here we multiply the values to get the amount of degrees to turn
	// Restrain is used to make sure the values returned by getMouse are in the
	// valid range. If this particular model were to turn more than this,
	// significant tearing would be visable
	eve_neck.set_p(clamp(mpos.get_x()) * 50);
	eve_neck.set_h(clamp(mpos.get_y()) * 20);
    }
    return AsyncTask::DS_cont;  // Task continues infinitely
}

void setup_lights()  // Sets up some default lighting
{
    auto ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(.4, .4, .35, 1));
    auto directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(0, 8, -2.5));
    directional_light->set_color(LColor(0.9, 0.8, 0.9, 1));
    auto render = window->get_render();
    render.set_light(render.attach_new_node(directional_light));
    render.set_light(render.attach_new_node(ambient_light));
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
