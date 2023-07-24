/*
 * Translation of Python cartoon-shader sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/cartoon-shader
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Kwasi Mensah
 * Date: 7/11/2005
 *
 * This is a tutorial to show some of the more advanced things
 * you can do with Cg. Specifically, with Non Photo Realistic
 * effects like Toon Shading. It also shows how to implement
 * multiple buffers in Panda.
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/shaderPool.h>
#include <panda3d/character.h>
#include <panda3d/animBundleNode.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables.  The Python sample stored these in the class; I am not
// using a class here.  Since it's not a class, C++ doesn't do forward
// references, so they are all declared up here.
PandaFramework framework;
WindowFramework* window;
PN_stdfloat separation = 0.001, cutoff = 0.3;
NodePath drawn_scene;
PT(AnimControl) anim;

// Function to put instructions on the screen.
void add_instructions(PN_stdfloat pos, const std::string &msg)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("instructions");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_left);
    text.set_pos(-1.0/a2d.get_sx()+0.08, 0, 1 - pos - 0.04); // TopLeft == (-1,0,1)
    text.set_scale(0.05);
}

// Function to put title on the screen.
void add_title(const std::string &text)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto node = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    // style = 1 -> plain (default)
    node.set_pos(1.0/a2d.get_sx()-0.1, 0, -1 + 0.09); // BottomRight == (1,0,-1)
    node.set_scale(0.08);
    text_node->set_align(TextNode::A_right);
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

// note: tjm: I combined the increase/decrease function to reduce duplication
void adjust_separation(PN_stdfloat);
void adjust_cutoff(PN_stdfloat);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    update_intervals();

    //Set the window title and open new window
    framework.set_window_title("Cartoon Shader (advanced) - C++ Panda3D Samples");
    window = framework.open_window();

    window->get_camera_group().set_pos(0, -50, 0);

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    // Check video card capabilities.
    if(!window->get_graphics_window()->get_gsg()->get_supports_basic_shaders()) {
	add_title("Toon Shader: Video driver reports that Cg shaders are not supported.");
	return;
    }

    // Show instructions in the corner of the window.
    add_title("Panda3D: Tutorial - Toon Shading with Normals-Based Inking");
    add_instructions(0.06, "ESC: Quit");
    add_instructions(0.12, "Up/Down: Increase/Decrease Line Thickness");
    add_instructions(0.18, "Left/Right: Decrease/Increase Line Darkness");
#if 0
    add_instructions(0.24, "V: View the render-to-texture results");
#endif

    // This shader's job is to render the model with discrete lighting
    // levels.  The lighting calculations built into the shader assume
    // a single nonattenuating point light.

    auto render = window->get_render();
    {
	NodePath tempnode(new PandaNode("temp node"));
	tempnode.set_shader(ShaderPool::load_shader("lightingGen.sha"));
	window->get_camera(0)->set_initial_state(tempnode.get_state());
    }

    // This is the object that represents the single "light", as far
    // the shader is concerned.  It's not a real Panda3D LightNode, but
    // the shader doesn't care about that.

    auto light = render.attach_new_node("light");
    light.set_pos(30, -50, 0);

    // this call puts the light's nodepath into the render state.
    // this enables the shader to access this light by name.

    render.set_shader_input("light", light);

    // The "normals buffer" will contain a picture of the model colorized
    // so that the color of the model is a representation of the model's
    // normal at that point.

    auto normals_buffer = window->get_graphics_window()->make_texture_buffer("normals_buffer", 0, 0);
    normals_buffer->set_clear_color(LVecBase4(0.5, 0.5, 0.5, 1));
    auto normals_camera = window->make_camera();
    auto cam = DCAST(Camera, normals_camera.node());
    cam->set_lens(window->get_camera(0)->get_lens());
    auto dr = normals_buffer->make_display_region();
    dr->set_camera(normals_camera);
    cam->set_scene(render);

    {
	NodePath tempnode(new PandaNode("temp node"));
	tempnode.set_shader(ShaderPool::load_shader("normalGen.sha"));
	cam->set_initial_state(tempnode.get_state());
    }

    // what we actually do to put edges on screen is apply them as a texture to
    // a transparent screen-fitted card

    drawn_scene = normals_buffer->get_texture_card();
    drawn_scene.set_transparency(TransparencyAttrib::M_alpha);
    drawn_scene.set_color(1, 1, 1, 0);
    drawn_scene.reparent_to(window->get_render_2d());

    // this shader accepts, as input, the picture from the normals buffer.
    // it compares each adjacent pixel, looking for discontinuities.
    // wherever a discontinuity exists, it emits black ink.

    auto ink_gen = ShaderPool::load_shader("inkGen.sha");
    drawn_scene.set_shader(ink_gen);
    drawn_scene.set_shader_input("separation", LVecBase4(separation, 0, separation, 0));
    drawn_scene.set_shader_input("cutoff", LVecBase4(cutoff));

#if 0 // The viewer is entiirely implemented in Python.  Forget it for now.
    // Panda contains a built-in viewer that lets you view the results of
    // your render-to-texture operations.  This code configures the viewer.

    framework.define_key("v", "", buffer_viewer.toggle_enable, &buffer_viewer);
    framework.define_key("V", "", buffer_viewer.toggle_enable, &buffer_viewer);
    buffer_viewer.set_position("llcorner");
#endif

    // Load a dragon model and start its animation.
    auto char_model = def_load_model("models/nik-dragon");
    char_model.reparent_to(render);
    // Scan for the character (should be first child)
    //auto character = DCAST(Character, char_model.find("+Character").node());
    auto character = DCAST(Character, char_model.get_child(0).node());
    // Scan for the anim bundlue (should be 2nd child)
    //auto anim_node = DCAST(AnimBundleNode, char_model.find("**/+AnimBundleNode").node());
    auto anim_node = DCAST(AnimBundleNode, char_model.get_child(1).node());
    anim = character->get_bundle(0)->bind_anim(anim_node->get_bundle(), ANIM_BIND_FLAGS);
    anim->set_anim_model(anim_node);
    anim->loop(true);
    auto spin = new NPAnim(char_model, "spin", 15);
    spin->set_end_hpr({360, 0, 0});
    spin->loop();

    // These allow you to change cartooning parameters in realtime
    framework.define_key("arrow_up", "", EV_FN() { adjust_separation(10.0/9); }, 0);
    framework.define_key("arrow_down", "", EV_FN() { adjust_separation(0.9); }, 0);
    framework.define_key("arrow_left", "", EV_FN() { adjust_cutoff(10.0/9); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { adjust_cutoff(0.9); }, 0);
}

void adjust_separation(PN_stdfloat adj)
{
    separation *= adj;
    std::cout << "separation: " << separation << '\n';
    drawn_scene.set_shader_input(
            "separation", LVecBase4(separation, 0, separation, 0));
}

void adjust_cutoff(PN_stdfloat adj)
{
    cutoff *= adj;
    std::cout << "cutoff: " << cutoff << '\n';
    drawn_scene.set_shader_input("cutoff", LVecBase4(cutoff));
}
}

int main(int argc, const char **argv)
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1)
	get_model_path().prepend_directory(argv[1]);

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    kill_intervals();
    framework.close_framework();
    return 0;
}
