/*
 *  Translation of Python solar-system sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/solar-system
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang and Phil Saltzman
 * Last Updated: 2015-03-13
 *
 * This tutorial will cover events and how they can be used in Panda
 * Specifically, this lesson will use events to capture keyboard presses and
 * mouse clicks to trigger actions in the world. It will also use events
 * to count the number of orbits the Earth makes around the sun. This
 * tutorial uses the same base code from the solar system tutorial.
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/graphicsOutput.h>
#include <panda3d/texturePool.h>
#include <panda3d/throw_event.h>

#include "anim_supt.h"

// Globals
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;

// In the Python API, you have to subclass DirectObject in order to listen for
// and respond to events.  In C++, you use callbacks.  There are good reasons
// to subclass some classes, but at the top level, there is none.  This is
// part of the reason I didn't bother with classes in any of my other translations.
// For singletons, classes are not all that useful.  An anonymous namespace can
// protect from symbol pollution without having to instantiate a class.  Just
// call the init function directly.
//
// However, I will retain the class for this sample, but ignore the Python
// version's subclassing to a class which doesn't even exist in the C++ API.

struct World {
    // Macro-like function used to reduce the amount to code needed to create the
    // on screen instructions

    TextNode *gen_label_text(const char *text, int i)
    {
	TextNode *text_node = new TextNode(text);
	auto node = NodePath(text_node);
	text_node->set_text(text);
	node.reparent_to(window->get_aspect_2d());
	node.set_pos(-1.0-0.06, 0, 1 - 0.06 * (i + 0.5)); // TopLeft is (-1, 0, 1)
	text_node->set_text_color(1, 1, 1, 1);
	text_node->set_align(TextNode::A_left);
	node.set_scale(0.05);
	return text_node;
    }

    World() {
        // The standard camera position and background initialization
	window->set_background_type(WindowFramework::BT_black);
	auto camera = window->get_camera_group();
        camera.set_pos(0, 0, 45);
        camera.set_hpr(0, -90, 0);

	load_planets();  // Load, texture, and position the planets
        rotate_planets();  // Set up the motion to start them moving

        // The standard title text that's in every tutorial
        // Things to note:
        // text_color represents the forground color of the text in (r,g,b,a)
	//            format
        // pos  represents the position of the text on the screen.
        //      The coordinate system is a x-y based wih 0,0 as the center of the
        //      screen, and y increasing upwards.  The projection uses z as the
	//      2D y coordinate, and 0 for y.  When the Python GUI uses a "location"
	//      projection, the location is added to every coordinate.  For example,
	//      "a2dBottomRight" is just aspect_2d which adds (1,0,-1) to any
	//      given coordinates.
        // align sets the alingment of the text relative to the pos argument.
        //       Default is left, but in Python it's center.
        // scale set the scale of the text
	// There is no may_change argument.  Instead, just save the text_node
	// pointer and call set_text() on it to change the text later.
	TextNode *text_node = new TextNode("title");  // Create the title
	auto text = NodePath(text_node);
	// We seem to have jumped from 1 to 3 in 6 steps w/o #2...
	text_node->set_text("Panda3D: Tutorial 3 - Events");
	text.reparent_to(window->get_aspect_2d());
	text_node->set_align(TextNode::A_right);
	text.set_pos(1.0-0.1, 0, -1+0.1);
	text_node->set_text_color(1, 1, 1, 1);
	text.set_scale(0.07);

        mouse1_event_text = gen_label_text(
            "Mouse Button 1: Toggle entire Solar System [RUNNING]", 1);
        skey_event_text = gen_label_text("[S]: Toggle Sun [RUNNING]", 2);
        ykey_event_text = gen_label_text("[Y]: Toggle Mercury [RUNNING]", 3);
        vkey_event_text = gen_label_text("[V]: Toggle Venus [RUNNING]", 4);
        ekey_event_text = gen_label_text("[E]: Toggle Earth [RUNNING]", 5);
        mkey_event_text = gen_label_text("[M]: Toggle Mars [RUNNING]", 6);
        year_counter_text = gen_label_text("0 Earth years completed", 7);

        // Events
	// Events are objects of type Event.  They have names, and optional
	// typed (EventParameter) parameters.
	// Events are processed via an EventHandler object.  For framework
	// applications, use the handler obtained with the get_event_handler()
	// method.
	// To receive an event, add a hook using the handler's add_hook()
	// method.  This is equivalent to the Python API's accept() method.
	// The hook will receive any events posted to the handler with the
	// given name.  Hooks receive the full Event structure, with the
	// event parameters.  They may also recive user data (void *).  Since
	// the hook is called outside of the class context, and maybe even
	// from a different thread, this pointer is commonly used to pass
	// the main object pointer (this).  In C++, you can use non-binding
	// lambda functions ([]) as hook functions, but not binding ones,
	// at least not directly ([&], [=]).  Combined with global variables,
	// this may eliminate the need to pass user data and cast it to the
	// correct type.
        // Certain events like "mouse1", "a", "b", "c" ... "z", "1", "2", "3"..."0"
        // are references to keyboard keys and mouse buttons. You can also define
        // your own events to be used within your program. In this tutorial, the
        // event "new_year" is not tied to a physical input device, but rather
        // is sent by the function that rotates the Earth whenever a revolution
        // completes to tell the counter to update
	//framework.get_event_handler().add_hook("escape", ...);
	// Note that the framework has a special way of binding keyboard and
	// mouse events, and also associating them with help text:
	// define_key.  It is equivalent to adding a hook to the given event
	// name on the framework's event handler.  There are two advantages to
	// using this instead of add_hook:  you don't need to find and pass
	// in the event handler (it uses its own), and you can associate help
	// text with the binding.  There is a default keyboard mapping, which
	// includes a "?" hook that displays the current bindings and their
	// help text on-screen.  Since most of the samples display usage
	// information all the time, this facility is wasted here, but if
	// you want to use it, you can get the on-screen help without the
	// rest of the defaults using framework.event_question, which requires
	// &framework as the user data:
	framework.define_key("?", "Help", framework.event_question, &framework);
	// Or, to add all bindings:
	//framework.enable_default_keys();
        // Exit the program when escape is pressed
	// You shouldn't just call exit(); instead, set the exit flag in the
	// framework in order to exit cleanly after the current frame is done
	// processing.
	//define_key("escape", "Exit", [](const Event *, void *){ framework.set_exit_flag(); }), 0);
	// Since I type that [](const Event *, void *) so often, I have added
	// a macro:  EV_FN([name]) (name is if you want user data).
	//define_key("escape", "Exit", EV_FN() { framework.set_exit_flag(); }, 0);
	// Better yet, since there is already an exit function, just use that:
	framework.define_key("escape", "Exit", framework.event_esc, &framework);
	// For convenient method invocation, use EV_CM(method_call())
	framework.define_key("mouse1", "Pause", EV_CM(handle_mouse_click()));
        framework.define_key("e", "Toggle Earth", EV_CM(handle_earth()));
	// Note that Python uses the extra parameter as a list of function
	// parameters.  Instead, you should bind using [=] or global variables.
	framework.define_key("s", "Toggle Sun", // event & help text
		   EV_CMt(t, toggle_planet( // method to call
			"Sun", // arguments to be passed to toggle_planet
			       // See toggle_planet's definition below for
                               // an explanation of what they are
                        t->day_period_sun,
                        nullptr,
                        t->skey_event_text)));
        // Repeat the structure above for the other planets
        framework.define_key("y", "Toggle Mercury", EV_CMt(t, toggle_planet(
                     "Mercury", t->day_period_mercury,
                     t->orbit_period_mercury, t->ykey_event_text)));
        framework.define_key("v", "Toggle Venus", EV_CMt(t, toggle_planet(
                     "Venus", t->day_period_venus,
                     t->orbit_period_venus, t->vkey_event_text)));
        framework.define_key("m", "Toggle Mars", EV_CMt(t, toggle_planet(
                     "Mars", t->day_period_mars,
                     t->orbit_period_mars, t->mkey_event_text)));
        framework.get_event_handler().add_hook("new_year", EV_CM(inc_year()));
	// Note that unlike the Python API, the C++ API does not enable keyboard
	// events by default.  This must be manually enabled:
	window->enable_keyboard();
    }

    // The global variables we used to control the speed and size of objects
    PN_stdfloat
	yearscale = 60,
	dayscale = yearscale / 365.0 * 5,
	orbitscale = 10,
        sizescale = 0.6;

    int year_counter = 0;  // year counter for earth years
    bool sim_running = true;  // boolean to keep track of the
                              // state of the global simulation

    // The modifiable label texts
    TextNode *mouse1_event_text, *skey_event_text, *ykey_event_text,
	     *vkey_event_text, *ekey_event_text, *mkey_event_text,
	     *year_counter_text;

    void handle_mouse_click() {
        // When the mouse is clicked, if the simulation is running pause all the
        // planets and sun, otherwise resume it
        if(sim_running) {
	    std::cout << "Pausing Simulation\n";
            // changing the text to reflect the change from "RUNNING" to
            // "PAUSED"
            mouse1_event_text->set_text(
                "Mouse Button 1: Toggle entire Solar System [PAUSED]");
            // For each planet, check if it is moving and if so, pause it
            // Sun
            if(day_period_sun->is_playing())
                toggle_planet("Sun", day_period_sun, nullptr,
                                  skey_event_text);
            if(day_period_mercury->is_playing())
                toggle_planet("Mercury", day_period_mercury,
                                  orbit_period_mercury, ykey_event_text);
            // Venus
            if(day_period_venus->is_playing())
                toggle_planet("Venus", day_period_venus,
                                  orbit_period_venus, vkey_event_text);
            //Earth and moon
            if(day_period_earth->is_playing())
                toggle_planet("Earth", day_period_earth,
                                  orbit_period_earth, ekey_event_text);
                toggle_planet("Moon", day_period_moon,
                                  orbit_period_moon);
            // Mars
            if(day_period_mars->is_playing())
                toggle_planet("Mars", day_period_mars,
                                  orbit_period_mars, mkey_event_text);
        } else {
            //The simulation is paused, so resume it
	    std::cout << "Resuming Simulation\n";
            mouse1_event_text->set_text(
                "Mouse Button 1: Toggle entire Solar System [RUNNING]");
            // the not operator does the reverse of the previous code
            if(!day_period_sun->is_playing())
                toggle_planet("Sun", day_period_sun, nullptr,
                                  skey_event_text);
            if(!day_period_mercury->is_playing())
                toggle_planet("Mercury", day_period_mercury,
                                  orbit_period_mercury, ykey_event_text);
            if(!day_period_venus->is_playing())
                toggle_planet("Venus", day_period_venus,
                                  orbit_period_venus, vkey_event_text);
            if(!day_period_earth->is_playing())
                toggle_planet("Earth", day_period_earth,
                                  orbit_period_earth, ekey_event_text);
                toggle_planet("Moon", day_period_moon,
                                  orbit_period_moon);
            if(!day_period_mars->is_playing())
                toggle_planet("Mars", day_period_mars,
                                  orbit_period_mars, mkey_event_text);
	}
        // toggle sim_running
        sim_running = !sim_running;
    } // end handleMouseClick

    // The toggle_planet function will toggle the intervals that are given to it
    // between paused and playing.
    // Planet is the name to print
    // Day is the interval that spins the planet
    // Orbit is the interval that moves around the orbit
    // Text is the OnscreenText object that needs to be updated
    void toggle_planet(const char *planet, CInterval *day,
		       CInterval *orbit = nullptr, TextNode *text = nullptr) {
	const char *state;
        if(day->is_playing()) {
	    std::cout << "Pausing " << planet << '\n';
            state = " [PAUSED]";
        } else {
	    std::cout << "Resuming " << planet << '\n';
            state = " [RUNNING]";
	}

        // Update the onscreen text if it is given as an argument
        if(text) {
            auto old = text->get_text();
            // strip out the last segment of text after the last white space
            // and append the string stored in 'state'
            text->set_text(old.substr(0, old.rfind(' ')) + state);
	}

        // toggle the day interval
        toggle_interval(day);
        // if there is an orbit interval, toggle it
        if(orbit)
            toggle_interval(orbit);
    } // end toggle_planet

    // toggle_interval does exactly as its name implies
    // It takes an interval as an argument. Then it checks to see if it is playing.
    // If it is, it pauses it, otherwise it resumes it.
    void toggle_interval(CInterval *interval) {
        if(interval->is_playing())
            interval->pause();
        else
            interval->resume();
    } // end toggle_interval

    // Earth needs a special buffer function because the moon is tied to it
    // When the "e" key is pressed, toggle_planet is called on both the earth and
    // the moon.
    void handle_earth() {
        toggle_planet("Earth", day_period_earth,
                          orbit_period_earth, ekey_event_text);
        toggle_planet("Moon", day_period_moon,
                          orbit_period_moon);
    } // end handleEarth

    // the function inc_year increments the variable year_counter and then updates
    // the OnscreenText 'year_counter_text' every time the message "new_year" is
    // sent
    void inc_year() {
        year_counter++;
        year_counter_text->set_text(
            std::to_string(year_counter) + " Earth years completed");
    } // end inc_year

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Except for the one commented line below, this is all as it was before //
// Scroll down to the next comment to see an example of sending messages //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void load_planets() {
	auto render = window->get_render();
        orbit_root_mercury = render.attach_new_node("orbit_root_mercury");
	orbit_root_venus = render.attach_new_node("orbit_root_venus");
        orbit_root_mars = render.attach_new_node("orbit_root_mars");
        orbit_root_earth = render.attach_new_node("orbit_root_earth");

        orbit_root_moon = (
            orbit_root_earth.attach_new_node("orbit_root_moon"));

        sky = def_load_model("models/solar_sky_sphere");

	auto sky_tex = def_load_texture("models/stars_1k_tex.jpg"); // same but shorter
        sky.set_texture(sky_tex, 1);
        sky.reparent_to(render);
        sky.set_scale(40);

        sun = def_load_model("models/planet_sphere");
        auto sun_tex = def_load_texture("models/sun_1k_tex.jpg");
        sun.set_texture(sun_tex, 1);
        sun.reparent_to(render);
        sun.set_scale(2 * sizescale);

        mercury = def_load_model("models/planet_sphere");
        auto mercury_tex = def_load_texture("models/mercury_1k_tex.jpg");
        mercury.set_texture(mercury_tex, 1);
        mercury.reparent_to(orbit_root_mercury);
        mercury.set_pos(0.38 * orbitscale, 0, 0);
        mercury.set_scale(0.385 * sizescale);

        venus = def_load_model("models/planet_sphere");
        auto venus_tex = def_load_texture("models/venus_1k_tex.jpg");
        venus.set_texture(venus_tex, 1);
        venus.reparent_to(orbit_root_venus);
        venus.set_pos(0.72 * orbitscale, 0, 0);
        venus.set_scale(0.923 * sizescale);

        mars = def_load_model("models/planet_sphere");
        auto mars_tex = def_load_texture("models/mars_1k_tex.jpg");
        mars.set_texture(mars_tex, 1);
        mars.reparent_to(orbit_root_mars);
        mars.set_pos(1.52 * orbitscale, 0, 0);
        mars.set_scale(0.515 * sizescale);

        earth = def_load_model("models/planet_sphere");
        auto earth_tex = def_load_texture("models/earth_1k_tex.jpg");
        earth.set_texture(earth_tex, 1);
        earth.reparent_to(orbit_root_earth);
        earth.set_scale(sizescale);
        earth.set_pos(orbitscale, 0, 0);

        orbit_root_moon.set_pos(orbitscale, 0, 0);

        moon = def_load_model("models/planet_sphere");
        auto moon_tex = def_load_texture("models/moon_1k_tex.jpg");
        moon.set_texture(moon_tex, 1);
        moon.reparent_to(orbit_root_moon);
        moon.set_scale(0.1 * sizescale);
        moon.set_pos(0.1 * orbitscale, 0, 0);
    }

    NodePath sky, sun, mercury, venus, mars, earth, moon;
    NodePath orbit_root_mercury, orbit_root_venus, orbit_root_mars,
	     orbit_root_earth, orbit_root_moon;

    void rotate_planets() {
        day_period_sun = new NPAnim(sun, "sun", 20);
	day_period_sun->set_end_hpr(LVector3(360, 0, 0));

        orbit_period_mercury = new NPAnim(orbit_root_mercury, "mercuryo",
					       (0.241 * yearscale));
	orbit_period_mercury->set_end_hpr(LVector3(360, 0, 0));
	day_period_mercury = new NPAnim(mercury, "mercuryd",
					     (59 * dayscale));
	day_period_mercury->set_end_hpr(LVector3(360, 0, 0));

        orbit_period_venus = new NPAnim(orbit_root_venus, "venuso",
					     (0.615 * yearscale));
	orbit_period_venus->set_end_hpr(LVector3(360, 0, 0));
        day_period_venus = new NPAnim(venus, "venusd",
					   (243 * dayscale));
	day_period_venus->set_end_hpr(LVector3(360, 0, 0));

        // Here the earth interval has been changed to rotate like the rest of the
        // planets and send a message before it starts turning again. To send a
        // message, create an Event object and dispatch() it to the handler.
	// There is a throw_event global function overloaded with various
	// numbers of parameters that does this for you.
	// The "new_year" message is picked up by the add_hook("new_year"...)
	// statement earlier, and calls the inc_year method as a result
	NPAnim *earth_orb;
        orbit_period_earth = new Sequence({
	    (earth_orb = new NPAnim(orbit_root_earth, "eartho", yearscale)),
	    new Func(throw_event("new_year"))
	});
	earth_orb->set_end_hpr(LVector3(360, 0, 0));
        day_period_earth = new NPAnim(earth, "earthd", dayscale);
	day_period_earth->set_end_hpr(LVector3(360, 0, 0));

        orbit_period_moon = new NPAnim(orbit_root_moon, "moono",
					    (.0749 * yearscale));
	orbit_period_moon->set_end_hpr(LVector3(360, 0, 0));
        day_period_moon = new NPAnim(moon, "moond",
					  (.0749 * yearscale));
	day_period_moon->set_end_hpr(LVector3(360, 0, 0));

        orbit_period_mars = new NPAnim(orbit_root_mars, "marso",
					    (1.881 * yearscale));
	orbit_period_mars->set_end_hpr(LVector3(360, 0, 0));
        day_period_mars = new NPAnim(mars, "marsd",
					  (1.03 * dayscale));
	day_period_mars->set_end_hpr(LVector3(360, 0, 0));

        day_period_sun->loop();
        orbit_period_mercury->loop();
        day_period_mercury->loop();
        orbit_period_venus->loop();
        day_period_venus->loop();
        orbit_period_earth->loop();
        day_period_earth->loop();
        orbit_period_moon->loop();
        day_period_moon->loop();
        orbit_period_mars->loop();
        day_period_mars->loop();
    }
    // variables promoted to class
    PT(NPAnim)
	day_period_sun, day_period_mercury, day_period_venus,
	day_period_earth, day_period_moon, day_period_mars,
	orbit_period_mercury, orbit_period_venus, orbit_period_moon,
	orbit_period_mars;
    PT(CInterval) orbit_period_earth;
    // A note about pointers:  most classes in Panda3D are reference-counted.
    // Generally, when someone takes ownership, the count is incrmeented, and
    // when it's done, the count is decremented.  If the count becomes zero,
    // it's automatically freed.  Using PT(type) instead of type * takes
    // ownership in this way.  I didn't do it for most of the other allocations
    // because the scene graph takes ownership, and never loses it.  However,
    // the animations need to be saved.
}; // end class world

int main(int argc, char **argv)
{
    if(argc > 1) // if there is a command-line argument,
	sample_path = *++argv; // override baked-in path to sample data

    framework.open_framework();

    // Note that in order for CInterval animations to run, you need to
    // manually poke them using the step() function.  This is done
    // automatically by the Python API, but for C++, you need to manually
    // add a task for this.  My convenience support function init_interval()
    // does just that.  Check it out if you like:  it is very simple.
    init_interval(); // run the animations when added

    window = framework.open_window();

    // instantiate the class
    World w;

    framework.main_loop();
    framework.close_framework();
}
