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
 * This tutorial is intended as a initial panda programming lesson going over
 * display initialization, loading models, placing objects, and the scene graph.
 *
 * Step 2: After initializing panda, we define a class called World. We put
 * all of our code in a class to provide a convenient way to keep track of
 * all of the variables our project will use, and in later tutorials to handle
 * keyboard input.
 * The code contained in the World() method is executed when we instantiate
 * the class (at the end of this file).  Inside World() we will first change
 * the background color of the window.  We then set the camera position.
 * Unlike the Python version, the mouse-based controls need not be disabled,
 * since they require manual enabling in the first place.
 *
 * One of the first things the Python version does is use GUI features.  The
 * GUI is exclusive to Python.  As such, one of the first complex things
 * done here is to implement a replacement for the called GUI function,
 * but in-line rather than as a generic function that can be called.
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/graphicsOutput.h>
PandaFramework framework;
WindowFramework *window;

struct World {  // Our main class (struct: public by default)
    World() {  // The constructor method caused when a
               // world object is created
        // Create some text overlayed on our screen.
        // We will use similar commands in all of our tutorials to create titles and
        // instruction guides.
	// Ignore most of this; it's just a re-implementation of OnscreenText().
	auto a2d = window->get_aspect_2d(); // coordinate space: 2d aspect-corrected
	TextNode *text_node = new TextNode("title"); // a scene node to hold title
	auto text = a2d.attach_new_node(text_node); // a path to the node
	// the scene node is text.  Set its attributes:
	text_node->set_text("Panda3D: Tutorial 1 - Solar System"); // text
	text_node->set_align(TextNode::A_right); // bottom-right: right-align
	text.set_pos(1.0/a2d.get_sx()-0.1, 0, -1+0.1); // Bottom-Right base is(1,0,-1) (3D Z is 2D Y; 3D Y is 0)
	text_node->set_text_color(1, 1, 1, 1); // color
	text.set_scale(0.07);  // font size: ~28 lines (2/.07) on screen

        // Make the background color black (R=0, G=0, B=0)
        // instead of the default grey
//	auto bgctrl = window->get_display_region_3d();
//	bgctrl->set_clear_color(LPoint3(0, 0, 0));
//	bgctrl->set_clear_color_active(true);
	// Or, since it's so common, the framework provides a shortcut:
	window->set_background_type(WindowFramework::BT_black);

	// The mouse controls the camera only if we explicitly enable it.
        // Often, we don't enable that so that the camera can be placed
	// manually (if we don't do this, our placement commands will be
	// overridden by the mouse control)
        // window->setup_trackball();

	// The Python ShowBase also enables the keyboard by default, so
	// you will always have to manually enable it.  There is a
	// convenient way to get a default debugging key mapping, with
	// on-screen help text (press ?).
	window->enable_keyboard();  // just enable any key bindings
	framework.enable_default_keys();  // set default key bindings

        // Set the camera position (x, y, z)
	// You may notice the increased complexity in determing what
	// the camera is.  In fact, it gets worse.  The folllwing is the
	// root camera node, whose position and orientation affects all
	// cameras rooted on this node.  In addition, the group has members,
	// obtained by e.g. window->get_camera(0), which can be individually
	// controlled, and must be controlled to adjust some settings, such as
	// lens properties.  All of this is hidden inside the Python API's
	// "camera" object.  When you are trying to translate code or
	// documentation written with the Python API, you must determine
	// which is the correct one to use.  If you're using the wrong one,
	// the compiler will complain, so at least you aren't wondering why
	// your program doesn't work or behaves weirdly.
	auto camera = window->get_camera_group();
        camera.set_pos(0, 0, 45);

        // Set the camera orientation (heading, pitch, roll) in degrees
        camera.set_hpr(0, -90, 0);
    }
}; // end class world

int main(void)
{
    // Initialize Panda and create a window
    framework.open_framework();

    window = framework.open_window();

    // Now that our class is defined, we create an instance of it.
    // Doing so calls the World() method set up above
    World w;

    // As usual - main_loop() must be called before anything can be shown on screen
    framework.main_loop();

    // cleanup
    framework.close_framework();
}
