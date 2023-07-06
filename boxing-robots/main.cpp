/*
 * Translation of Python boxing-robots sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/boxing-robots
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *   NOTE: at present, this sample does not work.  The head animations work,
 *   but the punches do not.  I have no idea why.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang, Phil Saltzman, and Eddie Caanan
 * Last Updated: 2015-03-13
 *
 * This tutorial shows how to play animations on models aka "actors".
 * It is based on the popular game of "Rock 'em Sock 'em Robots".
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/character.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
// The Python sample uses a class, which can have forward references even
// in C++.  I am using globals, though, as it saves a few headaches.  Thus,
// they need to all be delcared at the top.
PandaFramework framework;
WindowFramework *window;
NodePath ring, robot[2];
PT(AnimControl) left_punch[2], right_punch[2], head_up[2], head_down[2];
PT(Sequence) punch_left[2], punch_right[2], reset_head[2];
Randomizer rands;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;

// Macro-like function used to reduce the amount to code needed to create the
// on screen instructions
void gen_label_text(const char *text, int i)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode(text);
    auto path = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    path.set_scale(0.05);
    path.set_pos(-1.0/a2d.get_sx() + 0.1, 0, 1.0 - 0.1 - 0.07 * i); // TopLeft == (-1,0,1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_left);
}

void try_punch(const Event *, void *);
void check_punch(int);
void setup_lights(void);

void init()
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Boxing Robots - C++ Panda3D Samples");
    window = framework.open_window();

    // This code puts the standard title and instruction text on screen
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Actors");
    // style = 1 -> plain (default)
    text_node->set_text_color(0, 0, 0, 1);
    text.set_pos(1.0/a2d.get_sx()-0.2, 0, -1+0.1); // BottomRight == (1,0,-1)
    text_node->set_align(TextNode::A_right);
    text.set_scale(0.09);
    gen_label_text("ESC: Quit", 0);
    gen_label_text("[A]: Robot 1 Left Punch", 1);
    gen_label_text("[S]: Robot 1 Right Punch", 2);
    gen_label_text("[K]: Robot 2 Left Punch", 3);
    gen_label_text("[L]: Robot 2 Right Punch", 4);

    // Set the camera in a fixed position
    // note: mouse already disabled by default
    auto camera = window->get_camera_group();
    camera.set_pos_hpr(14.5, -15.4, 14, 45, -14, 0);
    window->set_background_type(WindowFramework::BT_black);

    // Add lighting so that the objects are not drawn flat
    setup_lights();

    // Load the ring
    ring = def_load_model("models/ring");
    ring.reparent_to(window->get_render());

    // Models that use skeletal animation are known as Actors instead of models
    // in the Python API, but they are Character(s) in the C++ API.  Either
    // way, they are loaded by the usual model loading mechanism.  However,
    // with separate animations, the animations must be bound to the
    // character.
    // We'll repeat the process for the second robot. The only thing that changes
    // here is the robot's color and position
    // Since they're the same models, they could've just been copied, but
    // instead, for this demo, we'll load them again.  At least they're
    // cached, so no reparsing is necessary.
    for(int r = 0; r < 2; r++) {
	// First, load the main characer model.
	robot[r] = def_load_model("models/robot");
	// Then, load and bind the animations.
	// In general, I prefer using variables or arrays over dictionaries
	// for storing things, since it speeds up the frequent lookups.
	left_punch[r] = load_anim(robot[r], sample_path + "models/robot_left_punch");
	right_punch[r] = load_anim(robot[r], sample_path + "models/robot_right_punch");
	head_up[r] = load_anim(robot[r], sample_path + "models/robot_head_up");
	head_down[r] = load_anim(robot[r], sample_path + "models/robot_head_down");
    }

    // Characters need to be positioned and parented like normal objects
    robot[0].set_pos_hpr_scale(-1, -2.5, 4, 45, 0, 0, 1.25, 1.25, 1.25);
    robot[0].reparent_to(window->get_render());

    // Set the properties of this robot
    robot[1].set_pos_hpr_scale(1, 1.5, 4, 225, 0, 0, 1.25, 1.25, 1.25);
    robot[1].set_color(LColor(.7, 0, 0, 1));
    robot[1].reparent_to(window->get_render());

    // Now we define how the animated models will move. Animations are played
    // through special intervals.  Since the Python code uses special convenience
    // wrappers, this code does, as well.  They are defined in the common
    // anim_supt.h file.  In this case we use a custom CInterval (CharAnimate,
    // located in anim_supt.h) that runs the animation directly.  This
    // is used in a sequence to play the part of the punch animation where the
    // arm extends, call a function to check if the punch landed, and then
    // play the part of the animation where the arm retracts

    for(int r = 0; r < 2; r++) {
	// Punch sequence for robot's left arm
	punch_left[r] = new Sequence({
	    // Interval for the outstreched animation
	    new CharAnimate(left_punch[r], 1, 1, 10),
	    // Function to check if the punch was successful
	    // Note that CIntervalManager uses a global lock on every
	    // method, including the one which runs this interval.  This
	    // means that calls which modify or query the interval state,
	    // such as most CInterval methods (even is_running()) will
	    // deadlock.  Instead, use FuncAsync(), which spawns a task.
	    // It's more expensive than Func(), though, so only use it
	    // if necessary.
	    new FuncAsync(check_punch(2 - r)),
            // Interval for the retract animation
	    new CharAnimate(left_punch[r], 1, 11, 31)
	});
	// Punch sequence for robot's right arm
	punch_right[r] = new Sequence({
	    new CharAnimate(right_punch[r], 1, 1, 10),
	    new FuncAsync(check_punch(2 - r)),
	    new CharAnimate(right_punch[r], 1, 11, 31)
	});

	// We use the same techinique to create a sequence for when a robot is knocked
	// out where the head pops up, waits a while, and then resets

	// Head animation for robot
	reset_head[r] = new Sequence({
	    // Interval for the head going up. Since no start or end frames were given,
	    // the entire animation is played.
	    new CharAnimate(head_up[r]),
	    new Wait(1.5),
	    // The head down animation was animated a little too quickly, so this will
	    // play it at 75% of it's normal speed
	    new CharAnimate(head_down[r], 0.75)
	});
    }

    // Now that we have defined the motion, we can define our key input.
    // Each fist is bound to a key. When a key is pressed, try_punch checks to
    // make sure that the both robots have their heads down, and if they do it
    // plays the given interval
    window->enable_keyboard();
#if 1 // Instead of defining ESC
    framework.define_key("escape", "Quit", framework.event_esc, &framework);
#else // You could use the default keybindings, but they conflict in a bad way
    framework.enable_default_keys();
#endif
    framework.define_key("a", "Robot 1 Left Punch", try_punch, (void *)punch_left[0]);
    framework.define_key("s", "Robot 1 Right Punch", try_punch, (void *)punch_right[0]);
    framework.define_key("k", "Robot 2 Left Punch", try_punch, (void *)punch_left[1]);
    framework.define_key("l", "Robot 2 Right Punch", try_punch, (void *)punch_right[1]);
}

// try_punch will play the interval passed to it only if
// neither robot has 'resetHead' playing (a head is up) AND
// the punch interval passed to it is not already playing
void try_punch(const Event *, void *parm)
{
    Sequence *seq = reinterpret_cast<Sequence *>(parm);
    if(!reset_head[0]->is_playing() &&
       !reset_head[1]->is_playing() &&
       !seq->is_playing())
	seq->start();
}

// check_punch will determine if a successful punch has been thrown
void check_punch(int robot)
{
    // if robot is playing'reset_head', do nothing
    if(reset_head[robot - 1]->is_playing())
	return;
    // if robot is not punching...
    if(!punch_left[robot - 1]->is_playing() &&
       !punch_right[robot - 1]->is_playing()) {
	// ...15% chance of successful hit
	if(rands.random_int(100) >= 85)
	    reset_head[robot - 1]->start();
	// Otherwise, only 5% chance of sucessful hit
    } else if(rands.random_int(100) >= 95)
	reset_head[robot - 1]->start();
}

// This function sets up the lighting
void setup_lights()
{
    PT(AmbientLight) ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(0.8, 0.8, 0.75, 1));
    PT(DirectionalLight) directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(0, 0, -2.5));
    directional_light->set_color(LColor(0.9, 0.8, 0.9, 1));
    const auto &render = window->get_render();
    window->get_render().set_light(render.attach_new_node(ambient_light));
    window->get_render().set_light(render.attach_new_node(directional_light));
}
}

int main(int argc, char **argv)
{
    // enable debug
    //Notify::ptr()->get_category(":chan")->set_severity(NS_spam);
    if(argc > 1)
	sample_path = argv[1];
    init();
    framework.main_loop();
    framework.close_framework();
    return 0;
}
