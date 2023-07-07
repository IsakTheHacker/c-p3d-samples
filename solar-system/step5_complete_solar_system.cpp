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
 * This tutorial is intended as a initial panda scripting lesson going over
 * display initialization, loading models, placing objects, and the scene graph.
 *
 * Step 5: Here we put the finishing touches on our solar system model by
 * making the planets move. The actual code for doing the movement is covered
 * in the next tutorial, but watching it move really shows what inheritance on
 * the scene graph is all about.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/graphicsOutput.h>
#include <panda3d/texturePool.h>

#include "sample_supt.h"

// Globals
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;

struct World {
    World() {
        // This is the initialization we had before
	auto a2d = window->get_aspect_2d();
	TextNode *text_node = new TextNode("title");  // Create the title
	auto text = a2d.attach_new_node(text_node);
	text_node->set_text("Panda3D: Tutorial 1 - Solar System");
	text_node->set_align(TextNode::A_right);
	text.set_pos(1.0/a2d.get_sx()-0.1, 0, -1+0.1);
	text_node->set_text_color(1, 1, 1, 1);
	text.set_scale(0.07);

	window->set_background_type(WindowFramework::BT_black);  // Set the background to black
        // window->setup_trackball();  // don't enable mouse control of the camera
	window->enable_keyboard();  // just enable any key bindings
	framework.enable_default_keys();  // set default key bindings
	auto camera = window->get_camera_group();
        camera.set_pos(0, 0, 45);  // Set the camera position (X, Y, Z)
        camera.set_hpr(0, -90, 0);  // Set the camera orientation
                                    //(heading, pitch, roll) in degrees

	load_planets();  // Load and position the models

        // Finally, we call the rotate_planets function which puts the planets,
        // sun, and moon into motion.
        rotate_planets();
    }

    // Here again is where we put our global variables. Added this time are
    // variables to control the relative speeds of spinning and orbits in the
    // simulation
    PN_stdfloat // there are a lot of them, all the same type
    // Number of seconds a full rotation of Earth around the sun should take
	yearscale = 60,
    // Number of seconds a day rotation of Earth should take.
    // It is scaled from its correct value for easier visability
	dayscale = yearscale / 365.0 * 5,
	orbitscale = 10,  // Orbit scale
        sizescale = 0.6;  // Planet size scale

    void load_planets() {
        // This is the same function that we completed in the previous step
        // It is unchanged in this version

        // Create the dummy nodes
	auto render = window->get_render(); // alias
        orbit_root_mercury = render.attach_new_node("orbit_root_mercury");
	orbit_root_venus = render.attach_new_node("orbit_root_venus");
        orbit_root_mars = render.attach_new_node("orbit_root_mars");
        orbit_root_earth = render.attach_new_node("orbit_root_earth");

        // The moon orbits Earth, not the sun
        orbit_root_moon = (
            orbit_root_earth.attach_new_node("orbit_root_moon"));

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Load the sky
        sky = def_load_model("models/solar_sky_sphere");
	auto sky_tex = def_load_texture("models/stars_1k_tex.jpg"); // same but shorter
        sky.set_texture(sky_tex, 1);
        sky.reparent_to(render);
        sky.set_scale(40);

        // Load the Sun
        sun = def_load_model("models/planet_sphere");
        auto sun_tex = def_load_texture("models/sun_1k_tex.jpg");
        sun.set_texture(sun_tex, 1);
        sun.reparent_to(render);
        sun.set_scale(2 * sizescale);

        // Load mercury
        mercury = def_load_model("models/planet_sphere");
        auto mercury_tex = def_load_texture("models/mercury_1k_tex.jpg");
        mercury.set_texture(mercury_tex, 1);
        mercury.reparent_to(orbit_root_mercury);
        mercury.set_pos(0.38 * orbitscale, 0, 0);
        mercury.set_scale(0.385 * sizescale);

        // Load Venus
        venus = def_load_model("models/planet_sphere");
        auto venus_tex = def_load_texture("models/venus_1k_tex.jpg");
        venus.set_texture(venus_tex, 1);
        venus.reparent_to(orbit_root_venus);
        venus.set_pos(0.72 * orbitscale, 0, 0);
        venus.set_scale(0.923 * sizescale);

        // Load Mars
        mars = def_load_model("models/planet_sphere");
        auto mars_tex = def_load_texture("models/mars_1k_tex.jpg");
        mars.set_texture(mars_tex, 1);
        mars.reparent_to(orbit_root_mars);
        mars.set_pos(1.52 * orbitscale, 0, 0);
        mars.set_scale(0.515 * sizescale);

        // Load Earth
        earth = def_load_model("models/planet_sphere");
        auto earth_tex = def_load_texture("models/earth_1k_tex.jpg");
        earth.set_texture(earth_tex, 1);
        earth.reparent_to(orbit_root_earth);
        earth.set_scale(sizescale);
        earth.set_pos(orbitscale, 0, 0);

        // Offest the moon dummy node so that it is positioned properly
        orbit_root_moon.set_pos(orbitscale, 0, 0);

        // Load the moon
        moon = def_load_model("models/planet_sphere");
        auto moon_tex = def_load_texture("models/moon_1k_tex.jpg");
        moon.set_texture(moon_tex, 1);
        moon.reparent_to(orbit_root_moon);
        moon.set_scale(0.1 * sizescale);
        moon.set_pos(0.1 * orbitscale, 0, 0);
    } // end load_planets()
    // Here are the variables above promoted into the class
    NodePath sky, sun, mercury, venus, mars, earth, moon;
    NodePath orbit_root_mercury, orbit_root_venus, orbit_root_mars,
	     orbit_root_earth, orbit_root_moon;

    void rotate_planets() {
        // rotate_planets creates intervals to actually use the hierarchy we created
        // to turn the sun, planets, and moon to give a rough representation of the
        // solar system. The next lesson will go into more depth on intervals.
	// [editor note: If, as I am, you are looking at the samples without
	// reference to its associated documentation, I believe the "next
	// lesson" referred to here is carousel.]
        auto day_period_sun = new NPAnim(sun, "sun", 20);
	day_period_sun->set_end_hpr(LVector3(360, 0, 0));

        auto orbit_period_mercury = new NPAnim(orbit_root_mercury, "mercuryo",
					       (0.241 * yearscale));
	orbit_period_mercury->set_end_hpr(LVector3(360, 0, 0));
	auto day_period_mercury = new NPAnim(mercury, "mercuryd",
					     (59 * dayscale));
	day_period_mercury->set_end_hpr(LVector3(360, 0, 0));

        auto orbit_period_venus = new NPAnim(orbit_root_venus, "venuso",
					     (0.615 * yearscale));
	orbit_period_venus->set_end_hpr(LVector3(360, 0, 0));
        auto day_period_venus = new NPAnim(venus, "venusd",
					   (243 * dayscale));
	day_period_venus->set_end_hpr(LVector3(360, 0, 0));

        auto orbit_period_earth = new NPAnim(orbit_root_earth, "eartho",
					     yearscale);
	orbit_period_earth->set_end_hpr(LVector3(360, 0, 0));
        auto day_period_earth = new NPAnim(earth, "earthd",
					   dayscale);
	day_period_earth->set_end_hpr(LVector3(360, 0, 0));

        auto orbit_period_moon = new NPAnim(orbit_root_moon, "moono",
					    (.0749 * yearscale));
	orbit_period_moon->set_end_hpr(LVector3(360, 0, 0));
        auto day_period_moon = new NPAnim(moon, "moond",
					  (.0749 * yearscale));
	day_period_moon->set_end_hpr(LVector3(360, 0, 0));

        auto orbit_period_mars = new NPAnim(orbit_root_mars, "marso",
					    (1.881 * yearscale));
	orbit_period_mars->set_end_hpr(LVector3(360, 0, 0));
        auto day_period_mars = new NPAnim(mars, "marsd",
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
}; // end class world

int main(int argc, char **argv)
{
    if(argc > 1) // if there is a command-line argument,
	sample_path = *++argv; // override baked-in path to sample data

    framework.open_framework();

    update_intervals(); // run the animations when added

    window = framework.open_window();

    // instantiate the class
    World w;

    framework.main_loop();
    kill_intervals(); // stop the animations to avoid crashing
    framework.close_framework();
}
