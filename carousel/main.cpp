/*
 *  Translation of Python carousel sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/carousel
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang, Phil Saltzman, and Eddie Canaan
 * Last Updated: 2015-03-13
 *
 * This tutorial will demonstrate some uses for intervals in Panda
 * to move objects in your panda world.
 * Intervals are tools that change a value of something, like position,
 * rotation or anything else, linearly, over a set period of time. They can be
 * also be combined to work in sequence or in Parallel
 *
 * In this lesson, we will simulate a carousel in motion using intervals.
 * The carousel will spin using an hprInterval while 4 pandas will represent
 * the horses on a traditional carousel. The 4 pandas will rotate with the
 * carousel and also move up and down on their poles using a LerpFunc interval.
 * Finally there will also be lights on the outer edge of the carousel that
 * will turn on and off by switching their texture with intervals in Sequence
 * and Parallel
 */
#include <pandaFramework.h>
#include <texturePool.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <cLerpNodePathInterval.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
NodePath carousel, lights1, lights2, env, pandas[4], models[4];
PT(CInterval) moves[4];
PT(Texture) light_off_tex, light_on_tex;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;

void load_models(void);
void setup_lights(void);
void start_carousel(void);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    // The C++ framework doesn't have a global, automaticaly-run interval
    // pool.  The following line uses a support function I wrote to spawn
    // an asynchronous task to do the updates.
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Carousel - C++ Panda3D Samples");
    window = framework.open_window();

    // This creates the on screen title that is in every tutorial
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    TextNode *text_node = new TextNode("title");
    auto title = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Carousel");
    title.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    title.set_pos(0, 0, -1+0.1); // BottomCenter == (0,0,-1)
    text_node->set_align(TextNode::A_center);
    title.set_scale(0.1);

    // Allowing manual positioning of the camera is the default behavior.
    // window->disable_mouse();
    // The Python sample has no keys, but I like at least esc.
    window->enable_keyboard();
    framework.enable_default_keys();

    NodePath camera = window->get_camera_group();
    camera.set_pos_hpr(0, -8, 2.5, 0, -9, 0);  // Set the cameras' position
                                               // and orientation

    load_models();     // Load and position our models
    setup_lights();    // Add some basic lighting
    start_carousel();  // Create the needed intervals and put the
                       // carousel into motion
}

void load_models()
{
    // Load the carousel base
    carousel = def_load_model("models/carousel_base");
    carousel.reparent_to(window->get_render());  // Attach it to render

    // Load the modeled lights that are on the outer rim of the carousel
    // (not Panda lights)
    // There are 2 groups of lights. At any given time, one group will have
    // the "on" texture and the other will have the "off" texture.
    lights1 = def_load_model("models/carousel_lights");
    lights1.reparent_to(carousel);

    // Load the 2nd set of lights
    lights2 = def_load_model("models/carousel_lights");
    // We need to rotate the 2nd so it doesn't overlap with the 1st set.
    lights2.set_h(36);
    lights2.reparent_to(carousel);

    // Load the textures for the lights. One texture is for the "on" state,
    // the other is for the "off" state.
    light_off_tex = def_load_texture("models/carousel_lights_off.jpg");
    light_on_tex = def_load_texture("models/carousel_lights_on.jpg");

    // Create an array (pandas) with filled with 4 dummy nodes attached
    // to the carousel.
    for(int i = 0; i < 4; i++) {
	pandas[i] = carousel.attach_new_node("panda" + std::to_string(i));

	models[i] = def_load_model("models/carousel_panda");

	// set the position and orientation of the ith panda node we just created
	// The Z value of the position will be the base height of the pandas.
	// The headings are multiplied by i to put each panda in its own position
	// around the carousel
	pandas[i].set_pos_hpr(0, 0, 1.3, i * 90, 0, 0);

	// Load the actual panda model, and parent it to its dummy node
	models[i].reparent_to(pandas[i]);
	// Set the distance from the center. This distance is based on the way the
	// carousel was modeled in Maya
	models[i].set_y(.85);
    }

    // Load the environment (Sky sphere and ground plane)
    env = def_load_model("models/env");
    env.reparent_to(window->get_render());
    env.set_scale(7);
}

// Panda Lighting
void setup_lights(void)
{
    // Create some lights and add them to the scene. By setting the lights on
    // render they affect the entire scene
    // Check out the lighting tutorial for more information on lights
    PT(AmbientLight) ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(0.4, 0.4, 0.35, 1));
    PT(DirectionalLight) directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(0, 8, -2.5));
    directional_light->set_color(LColor(0.9, 0.8, 0.9, 1));
    window->get_render().set_light(window->get_render().attach_new_node(directional_light));
    window->get_render().set_light(window->get_render().attach_new_node(ambient_light));

    // Explicitly set the environment to not be lit
    env.set_light_off();
}

void oscillate_panda(double rad, NodePath &panda, double offset);

void start_carousel()
{
    // Here's where we actually create the intervals to move the carousel
    // The first type of interval we use is one created directly from a NodePath
    // This interval tells the NodePath to vary its orientation (hpr) from its
    // current value (0,0,0) to (360,0,0) over 20 seconds. Intervals created from
    // NodePaths also exist for position, scale, color, and shear
    // Generally, you allocate a new interval using new(), and once it
    // is started, the animator prevents it from being freed.  Otherwise, you
    // will need to store it using a PT(T) pointer.
    // Note also that in order for a loop to restart at the beginning, it
    // needs to either end at the same place it started, or an explicit start
    // value must be provided, or the fourth parameter, bake_in_start, must
    // be true.

    auto carousel_spin = new
	CLerpNodePathInterval("carousel_spin", 20, CLerpInterval::BT_no_blend,
			      true, false, carousel, NodePath());
    carousel_spin->set_end_hpr(LVector3(360, 0, 0));
    // Once an interval is created, we need to tell it to actually move.
    // start() will cause an interval to play once. loop() will tell an interval
    // to repeat once it finished. To keep the carousel turning, we use
    // loop()
    carousel_spin->loop();

    // The next type of interval we use is called a LerpFunc interval. It is
    // called that becuase it linearly interpolates (aka Lerp) values passed to
    // a function over a given amount of time.

    // In this specific case, horses on a carousel don't move contantly up,
    // suddenly stop, and then contantly move down again. Instead, they start
    // slowly, get fast in the middle, and slow down at the top. This motion is
    // close to a sine wave. This LerpFunc calls the function oscillate_panda
    // (which we will create below), which changes the height of the panda based
    // on the sin of the value passed in. In this way we achieve non-linear
    // motion by linearly changing the input to a function
    // Note that for the C++ code, I have two versions: LerpFunc and LerpFuncG.
    // LerpFunc is for methods, and LerpFuncG is for globals/statics.
    for(int i = 0; i < 4; i++) {
	moves[i] = new LerpFuncG(
	        // function to call
		// this takes a functor, actually, so user arguments can be
                // inclulded without extra contructor arguments.
		// Don't use references ([&]) to local variables!
		[=](double a) {
		    oscillate_panda(a, models[i], MathNumbers::pi_d * (i % 2));
		},
                0.0,  // starting value (in radians)
                2 * MathNumbers::pi_d,  // ending value (2pi radians = 360 degrees)
                3   // 3 second duration
            );
	// again, we want these to play continuously so we start them with
	// loop()
	moves[i]->loop();
    }

    // Finally, we combine Sequence, Parallel, Func, and Wait intervals,
    // to schedule texture swapping on the lights to simulate the lights turning
    // on and off.
    // Sequence intervals play other intervals in a sequence. In other words,
    // it waits for the current interval to finish before playing the next
    // one.
    // Parallel intervals play a group of intervals at the same time
    // Wait intervals simply do nothing for a given amount of time
    // Both Sequence and Parallel interval constructors use the static
    // initializer format from C++11.  This means that the constructor takes
    // a single argument, a brace-enclosed list of other interval pointers.
    // Func intervals simply make a single function call. This is helpful because
    // it allows us to schedule functions to be called in a larger sequence. They
    // take virtually no time so they don't cause a Sequence to wait.
    // Again, in the C++ version, a functor is used, and a convenience macro
    // wraps the argument in a lamba function.  No separate extra arguments.

    auto light_blink = new Sequence({
	// For the first step in our sequence we will set the on texture on one
	// light and set the off texture on the other light at the same time
	new Parallel({
	    new Func(lights1.set_texture(light_on_tex, 1)),
	    new Func(lights2.set_texture(light_off_tex, 1))
	}),
	new Wait(1),  // Then we will wait 1 second
	// Then we will switch the textures at the same time
	new Parallel({
	    new Func(lights1.set_texture(light_off_tex, 1)),
	    new Func(lights2.set_texture(light_on_tex, 1))
	}),
	new Wait(1)  // Then we will wait another second
    });

    light_blink->loop();  // Loop this sequence continuously
}

void oscillate_panda(double rad, NodePath &panda, double offset)
{
    // This is the oscillation function mentioned earlier. It takes in a
    // degree value, a NodePath to set the height on, and an offset. The
    // offset is there so that the different pandas can move opposite to
    // each other.  The .2 is the amplitude, so the height of the panda will
    // vary from -.2 to .2
    panda.set_z(sin(rad + offset) * .2);
}
}

int main(int argc, const char **argv)
{
    if(argc > 1)
	sample_path = argv[1];

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
