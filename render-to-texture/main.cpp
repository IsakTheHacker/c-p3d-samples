/*
 *  Translation of Python render-to-texture sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/render-to-texture
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/directionalLight.h>
#include <panda3d/ambientLight.h>
#include <panda3d/pointLight.h>
#include <panda3d/character.h>
#include <panda3d/animBundleNode.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables.  The Python sample stored these in the class; I am not
// using a class here.  Since it's not a class, C++ doesn't do forward
// references, so they are all declared up here.
PandaFramework framework;
WindowFramework* window;
NodePath alt_cam;
std::vector<PT(AnimControl)> tv_men;

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
    text.set_pos(-1/a2d.get_sx() + 0.08, 0, 1 -pos - 0.04); // TopLeft = (-1, 0, 1)
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
    node.set_scale(0.08);
    text_node->set_align(TextNode::A_right);
    node.set_pos(1/a2d.get_sx()-0.1, 0, -1 + 0.09); // BottomRight = (1, 0, -1)
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

void make_tv_man(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, Texture *tex,
		 PN_stdfloat playrate);
void zoom_in(void), zoom_out(void), move_left(void), move_right(void);

void init(void)
{
    framework.open_framework();
    update_intervals();

    // tjm: hmmm.. maybe I should call it Teapot on TV, like the sample class...
    framework.set_window_title("Render To Texture - C++ Panda3D Samples");
    window = framework.open_window();
    window->set_background_type(WindowFramework::BT_black);

    // Post the instructions.
    add_title("Panda3D: Tutorial - Using Render-to-Texture");
    add_instructions(0.06, "ESC: Quit");
    add_instructions(0.12, "Up/Down: Zoom in/out on the Teapot");
    add_instructions(0.18, "Left/Right: Move teapot left/right");
    // add_instructions(0.24, "V: View the render-to-texture results");

    // we now get buffer thats going to hold the texture of our new scene
    auto win = window->get_graphics_window();
    PT(GraphicsOutput) alt_buffer = win->make_texture_buffer("hello", 256, 256);

    // now we have to setup a new scene graph to make this scene
    NodePath alt_render("new render");

    // this takes care of setting up ther camera properly
    alt_cam = window->make_camera();
    auto dr = alt_buffer->make_display_region();
    dr->set_camera(alt_cam);

    alt_cam.reparent_to(alt_render);
    alt_cam.set_pos(0, -10, 0);

    // get the teapot and rotates it for a simple animation
    auto teapot = window->load_model(alt_render, "teapot");
    teapot.set_pos(0, 0, -1);
    auto tpanim = new NPAnim(teapot, "teapot", 1.5);
    tpanim->set_end_hpr(LPoint3(360, 360, 360));
    tpanim->loop();

    // put some lighting on the teapot
    auto dlight = new DirectionalLight("dlight");
    auto alight = new AmbientLight("alight");
    auto dlnp = alt_render.attach_new_node(dlight);
    auto alnp = alt_render.attach_new_node(alight);
    dlight->set_color(LColor(0.8, 0.8, 0.5, 1));
    alight->set_color(LColor(0.2, 0.2, 0.2, 1));
    dlnp.set_hpr(0, -60, 0);
    alt_render.set_light(dlnp);
    alt_render.set_light(alnp);

    // Put lighting on the main scene
    auto render = window->get_render();
    auto plight = new PointLight("plight");
    auto plnp = render.attach_new_node(plight);
    plnp.set_pos(0, 0, 10);
    render.set_light(plnp);
    render.set_light(alnp);

#if 0 // it's Python-only
    // Panda contains a built-in viewer that lets you view the results of
    // your render-to-texture operations.  This code configures the viewer.

    framework.define_key("v", "", EV_FN() { buffer_viewer.toggle_enable(); }, 0);
    framework.define_key("V", EV_FN() { buffer_viewer.toggle_enable(); }, 0);
    buffer_viewer.set_position("llcorner");
    buffer_viewer.set_card_size(1.0, 0.0);
#endif

    // Create the tv-men. Each TV-man will display the
    // offscreen-texture on his TV screen.
    make_tv_man(-5, 30, 1, alt_buffer->get_texture(), 0.9);
    make_tv_man(5, 30, 1, alt_buffer->get_texture(), 1.4);
    make_tv_man(0, 23, -3, alt_buffer->get_texture(), 2.0);
    make_tv_man(-5, 20, -6, alt_buffer->get_texture(), 1.1);
    make_tv_man(5, 18, -5, alt_buffer->get_texture(), 1.7);

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("arrow_up", "", EV_FN() { zoom_in(); }, 0);
    framework.define_key("arrow_down", "", EV_FN() { zoom_out(); }, 0);
    framework.define_key("arrow_left", "", EV_FN() { move_left(); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { move_right(); }, 0);
}

void make_tv_man(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z, Texture *tex,
		 PN_stdfloat playrate)
{
    auto man = def_load_model("models/mechman_idle");
    man.set_pos(x, y, z);
    man.reparent_to(window->get_render());
    auto faceplate = man.find("**/faceplate");
    faceplate.set_texture(tex, 1);
    // Scan for the character (should be first child)
    //auto character = DCAST(Character, char_model.find("+Character").node());
    auto character = DCAST(Character, man.get_child(0).node());
    // Scan for the anim bundlue (should be 2nd child)
    //auto anim_node = DCAST(AnimBundleNode, char_model.find("**/+AnimBundleNode").node());
    auto anim_node = DCAST(AnimBundleNode, man.get_child(1).node());
    auto anim = character->get_bundle(0)->bind_anim(anim_node->get_bundle(), ANIM_BIND_FLAGS);
    anim->set_anim_model(anim_node);
    anim->set_play_rate(playrate);
    tv_men.push_back(anim);
    tv_men.back()->loop(true);
}

void zoom_in()
{
    alt_cam.set_y(alt_cam.get_y() * 0.9);
}

void zoom_out()
{
    alt_cam.set_y(alt_cam.get_y() * 1.2);
}

void move_left()
{
    alt_cam.set_x(alt_cam.get_x() + 1);
}

void move_right()
{
    alt_cam.set_x(alt_cam.get_x() - 1);
}
}

int main(int argc, char **argv)
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1)
	get_model_path().prepend_directory(argv[1]);
    init();
    framework.main_loop();
    kill_intervals();
    framework.close_framework();
    return 0;
}
