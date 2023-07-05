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
 * Step 3: In this step, we create a function called load_planets, which will
 * eventually be used to load all of the planets in our simulation. For now
 * we will load just the sun and and the sky-sphere we use to create the
 * star-field.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/graphicsOutput.h>
#include <panda3d/texturePool.h>

#include "anim_supt.h" // some macros I creaed to ease porting

PandaFramework framework;
WindowFramework *window;

// The following is in all of my C++ conversions that load files.
// SAMPLE_DIR is a baked-in path to the Python samples, if installed.
// In any case, if the demo is given a command-line argument, it is
// a path to override this directory.  Don't forget to append a /.
// This way, I don't have to distrubute the assets.
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;

struct World {
    World() {
        // This is the initialization we had before
	TextNode *text_node = new TextNode("title");  // Create the title
	auto text = NodePath(text_node);
	text_node->set_text("Panda3D: Tutorial 1 - Solar System");
	text.reparent_to(window->get_aspect_2d());
	text_node->set_align(TextNode::A_right);
	text.set_pos(1.0-0.1, 0, -1+0.1);
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

	// Now that we have finished basic initialization, we call load_planets which
	// will handle actually getting our objects in the world
	load_planets();
    }

    // We will now define a variable to help keep a consistent scale in
    // our model. As we progress, we will continue to add variables here as we
    // need them

    // The value of this variable scales the size of the planets. True scale size
    // would be 1
    PN_stdfloat sizescale = 0.6;
    // You may be wondering:  what is PN_stdfloat?  Panda3D can be compiled
    // with either the C++ native "float" type or the the native "double" type
    // as its default.  PN_stdfloat is a typedef to that default type.
    // Similarly, LVector3 (above), LPoint3, LColor, etc. are all types
    // whose components are of type PN_stdfloat.  If you want the standard
    // floating point types, you can still use LVector3f and LVector3d for
    // float and double versions, respectively.  It also doesn't matter
    // much for single-valued variables, unless a pointer to that variable
    // is passed around.  The language will automatically cast the value to
    // the correct type on assignment.  If you're using the standard type,
    // you should also look into the functions provided by the MathFunctions
    // class, as they also come in three varieties (f, d, and default).  There
    // are also i (integer) composite types (Lvector3i, etc.), but they
    // are probably not as useful.

    void load_planets() {
        // Here, inside our class, is where we are creating the load_planets function
        // For now we are just loading the star-field and sun. In the next step we
        // will load all of the planets

        // Loading objects in Panda can be done in a variety of ways.  The
	// preferred way is probably to use the framework's method:
	// window->load_model(parent, file).  parent is a node to attach
	// the model to, and file is a file name.  The framework provides
	// a staging/storage area for models that you can use for the parent:
	// framework.get_models().  For these samples, I also prepend the
	// path to the Python sample (sample_path) to all file names so that
	// I don't have to distribute the assets.  In fact, I do this so
	// often in my samples, that I have added a support header file,
	// called anim_supt.h, which defines a macro, def_load_model,
	// which does just that.
	// You can, of course, also just use a loader, but using the window
	// loader allows you to use the framework's virtual filesystem feature
	// transparently.  In addition, the usage of loaders diverges greately
	// from Pythons loader, so it's best to just ignore it unless you have
	// advanced uses.
	// Native models in Panda come in two types, .egg (which is readable
	// in a text editor), and .bam (which is not readable but makes
	// smaller files).  The size advantage is lost when you consider that
	// the pzip and punzip commands can convert to and from a compressed
	// form which Panda can automatically uncompress if it has an
	// additional pz extension, such as .egg.pz.  Plugins can also be used
	// to transparently load other model formats.  When you load a file
	// you leave the extension off so that it can choose the right version

        // Load model returns a NodePath, which you can think of as an object
        // containing your model

        // Here we load the sky model. For all the planets we will use the same
        // sphere model and simply change textures. However, even though the sky is
        // a sphere, it is different from the planet model because its polygons
        // (which are always one-sided in Panda) face inside the sphere instead of
        // outside (this is known as a model with reversed normals). Because of
        // that it has to be a separate model.
	//sky = window->load_model(framework.get_models(), sample_path + "models/solar_sky_sphere");
        sky = def_load_model("models/solar_sky_sphere"); // same thing, shorter

        // After the object is loaded, it must be placed in the scene. We do this by
        // changing the parent of sky to render, which is a special NodePath.
        // Each frame, Panda starts with render and renders everything attached to
        // it.
	// The render node is obtained via window->get_render().
	// Note that if you use window->load_model(),
	// If you are going to reparent anyway, a shorter way of doing this
	// is to give the parent in the window->load_model() to begin with.
        sky.reparent_to(window->get_render());

        // You can set the position, orientation, and scale on a NodePath the same
        // way that you set those properties on the camera. In fact, the camera is
        // just another special NodePath
        sky.set_scale(40);

        // Very often, the egg file will know what textures are needed and load them
        // automatically. But sometimes we want to set our textures manually, (for
        // instance we want to put different textures on the same planet model)
        // Unlike the Python API, a different API must be used to load textures.
	// The preferred method is to either create a Texture and use its read()
	// method, or, better yet, use the global TexturePool's load_texture()
	// method.  Once again, I have enapsulated prefixing sample_path and
	// using TexturePool into a shorter macro.
	//auto sky_tex = TexturePool::load_texture(sample_path + "models/stars_1k_tex.jpg");
	auto sky_tex = def_load_texture("models/stars_1k_tex.jpg"); // same but shorter

        // Finally, the following line sets our new sky texture on our sky model.
        // The second argument must be one or the command will be ignored.
        sky.set_texture(sky_tex, 1);

        // Now we load the sun.
        sun = def_load_model("models/planet_sphere");
        // Now we repeat our other steps
        sun.reparent_to(window->get_render());
        auto sun_tex = def_load_texture("models/sun_1k_tex.jpg");
        sun.set_texture(sun_tex, 1);
        // The sun is really much bigger than
        sun.set_scale(2 * sizescale);
        // this, but to be able to see the
        // planets we're making it smaller
    } // end load_planets()
    NodePath sky, sun; // used above
}; // end class world

int main(int argc, char **argv)
{
    if(argc > 1) // if there is a command-line argument,
	sample_path = *++argv; // override baked-in path to sample data

    framework.open_framework();
    window = framework.open_window();

    // instantiate the class
    World w;

    framework.main_loop();
    framework.close_framework();
}
