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
 * Step 4: In this step, we will load the rest of the planets up to Mars.
 * In addition to loading them, we will organize how the planets are grouped
 * hierarchically in the scene. This will help us rotate them in the next step
 * to give a rough simulation of the solar system.  You can see them move by
 * running step_5_complete_solar_system.py.
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/graphicsOutput.h>
#include <panda3d/texturePool.h>

#include "sample_supt.h"

// Globals
PandaFramework framework;
WindowFramework *window;

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

	load_planets();  // Load our models and make them render
    }

    // This section has our variables. This time we are adding a variable to
    // control the relative size of the orbits.
    PN_stdfloat sizescale = 0.6;  // relative size of planets
    PN_stdfloat orbitscale = 10;  // relative size of orbits

    void load_planets() {
        // Here is where we load all of the planets, and place them.
        // The first thing we do is create a dummy node for each planet. A dummy
        // node is simply a node path that does not have any geometry attached to it.
        // This is done by <NodePath>.attach_new_node("name_of_new_node")

        // We do this because positioning the planets around a circular orbit could
        // be done with a lot of messy sine and cosine operations. Instead, we define
        // our planets to be a given distance from a dummy node, and when we turn the
        // dummy, the planets will move along with it, kind of like turning the
        // center of a disc and having an object at its edge move. Most attributes,
        // like position, orientation, scale, texture, color, etc., are inherited
        // this way. Panda deals with the fact that the objects are not attached
        // directly to render (they are attached through other NodePaths to render),
        // and makes sure the attributes inherit.

        // This system of attaching NodePaths to each other is called the Scene
        // Graph
	auto render = window->get_render(); // alias
        orbit_root_mercury = render.attach_new_node("orbit_root_mercury");
	orbit_root_venus = render.attach_new_node("orbit_root_venus");
        orbit_root_mars = render.attach_new_node("orbit_root_mars");
        orbit_root_earth = render.attach_new_node("orbit_root_earth");

        // orbit_root_moon is like all the other orbit_root dummy nodes except that
        // it will be parented to orbit_root_earth so that the moon will orbit the
        // earth instead of the sun. So, the moon will first inherit
        // orbit_root_moon's position and then orbit_root_earth's. There is no hard
        // limit on how many objects can inherit from each other.
        orbit_root_moon = (
            orbit_root_earth.attach_new_node("orbit_root_moon"));

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // These are the same steps used to load the sky model that we used in the
        // last step
        // Load the model for the sky
        sky = def_load_model("models/solar_sky_sphere");
        // Load the texture for the sky.
	auto sky_tex = TexturePool::load_texture("models/stars_1k_tex.jpg"); // same but shorter
        sky.set_texture(sky_tex, 1);
        // Parent the sky model to the render node so that the sky is rendered
        sky.reparent_to(render);
        // Scale the size of the sky.
        sky.set_scale(40);

        // These are the same steps we used to load the sun in the last step.
        // Again, we use loader.loadModel since we're using planet_sphere more
        // than once.
        sun = def_load_model("models/planet_sphere");
        auto sun_tex = TexturePool::load_texture("models/sun_1k_tex.jpg");
        sun.set_texture(sun_tex, 1);
        sun.reparent_to(render);
        sun.set_scale(2 * sizescale);

        // Now we load the planets, which we load using the same steps we used to
        // load the sun. The only difference is that the models are not parented
        // directly to render for the reasons described above.
        // The values used for scale are the ratio of the planet's radius to Earth's
        // radius, multiplied by our global scale variable. In the same way, the
        // values used for orbit are the ratio of the planet's orbit to Earth's
        // orbit, multiplied by our global orbit scale variable

        // Load mercury
        mercury = def_load_model("models/planet_sphere");
        auto mercury_tex = TexturePool::load_texture("models/mercury_1k_tex.jpg");
        mercury.set_texture(mercury_tex, 1);
        mercury.reparent_to(orbit_root_mercury);
        // Set the position of mercury. By default, all nodes are pre assigned the
        // position (0, 0, 0) when they are first loaded. We didn't reposition the
        // sun and sky because they are centered in the solar system. Mercury,
        // however, needs to be offset so we use .set_pos to offset the
        // position of mercury in the X direction with respect to its orbit radius.
        // We will do this for the rest of the planets.
        mercury.set_pos(0.38 * orbitscale, 0, 0);
        mercury.set_scale(0.385 * sizescale);

        // Load Venus
        venus = def_load_model("models/planet_sphere");
        auto venus_tex = TexturePool::load_texture("models/venus_1k_tex.jpg");
        venus.set_texture(venus_tex, 1);
        venus.reparent_to(orbit_root_venus);
        venus.set_pos(0.72 * orbitscale, 0, 0);
        venus.set_scale(0.923 * sizescale);

        // Load Mars
        mars = def_load_model("models/planet_sphere");
        auto mars_tex = TexturePool::load_texture("models/mars_1k_tex.jpg");
        mars.set_texture(mars_tex, 1);
        mars.reparent_to(orbit_root_mars);
        mars.set_pos(1.52 * orbitscale, 0, 0);
        mars.set_scale(0.515 * sizescale);

        // Load Earth
        earth = def_load_model("models/planet_sphere");
        auto earth_tex = TexturePool::load_texture("models/earth_1k_tex.jpg");
        earth.set_texture(earth_tex, 1);
        earth.reparent_to(orbit_root_earth);
        earth.set_scale(sizescale);
        earth.set_pos(orbitscale, 0, 0);

        // The center of the moon's orbit is exactly the same distance away from
        // The sun as the Earth's distance from the sun
        orbit_root_moon.set_pos(orbitscale, 0, 0);

        // Load the moon
        moon = def_load_model("models/planet_sphere");
        auto moon_tex = TexturePool::load_texture("models/moon_1k_tex.jpg");
        moon.set_texture(moon_tex, 1);
        moon.reparent_to(orbit_root_moon);
        moon.set_scale(0.1 * sizescale);
        moon.set_pos(0.1 * orbitscale, 0, 0);
    } // end load_planets()
    // Here are the variables above promoted into the class
    NodePath sky, sun, mercury, venus, mars, earth, moon;
    NodePath orbit_root_mercury, orbit_root_venus, orbit_root_mars,
	     orbit_root_earth, orbit_root_moon;
}; // end class world

int main(int argc, char **argv)
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1) // if there is a command-line argument,
	get_model_path().prepend_directory(*++argv); // override baked-in path to sample data

    framework.open_framework();
    window = framework.open_window();

    // instantiate the class
    World w;

    framework.main_loop();
    framework.close_framework();
}
