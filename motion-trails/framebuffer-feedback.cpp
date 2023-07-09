/*
 *  Translation of Python motion-trails sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/motion-trails
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Josh Yelon
 *
 * This is a tutorial to show one of the simplest applications
 * of copy-to-texture: motion trails.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/orthographicLens.h>
#include <panda3d/directionalLight.h>
#include <panda3d/ambientLight.h>
#include <panda3d/character.h>
#include <panda3d/animBundleNode.h>
#include <panda3d/cardMaker.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables.  The Python sample stored these in the class; I am not
// using a class here.  Since it's not a class, C++ doesn't do forward
// references, so they are all declared up here.
PandaFramework framework;
WindowFramework* window;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;
NodePath bcard, fcard;
PN_stdfloat clickrate, nextclick;

void add_instructions(PN_stdfloat pos, const std::string &msg)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("instructions");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(-1/a2d.get_sx() + 0.06, 0, 1 -pos - 0.03); // TopLeft = (-1, 0, 1)
    text_node->set_align(TextNode::A_left);
    text.set_scale(0.05);
}

AsyncTask::DoneStatus take_snapshot(GenericAsyncTask *, void *);
void choose_effect_ghost(void), choose_effect_paint_brush(void),
     choose_effect_double_vision(void), choose_effect_wings_of_blue(void),
     choose_effect_whirlpool(void);

void init_trails(void)
{
    framework.open_framework();
    update_intervals();
    framework.set_window_title("Motion Trails (Framebuffer Feedback) - C++ Panda3D Samples");
    window = framework.open_window();

    window->get_camera_group().set_pos(0, -26, 4);
    window->set_background_type(WindowFramework::BT_black);

    // Create a texture into which we can copy the main window.
    // We set it to RTM_triggered_copy_texture mode, which tells it that we
    // want it to copy the window contents into a texture every time we
    // call win->trigger_copy().
    PT(Texture) tex = new Texture;
    tex->set_minfilter(SamplerState::FT_linear);
    auto win = window->get_graphics_window();
    win->add_render_texture(tex,
			    GraphicsOutput::RTM_triggered_copy_texture);

    // Set the initial color to clear the texture to, before rendering it.
    // This is necessary because we don't clear the texture while rendering,
    // and otherwise the user might see garbled random data from GPU memory.
    tex->set_clear_color(LColor(0, 0, 0, 1));
    tex->clear_image();

    // Create another 2D camera. Tell it to render before the main camera.
    auto background = NodePath("background");
    /// start of replacement for make_camera2d(blur_buffer)
    auto dr = win->make_mono_display_region();
    dr->set_sort(-10); // sort override
    PT(Camera) backcam_node = new Camera("camera2d");
    auto backcam = background.attach_new_node(backcam_node); // root override
    auto lens = new OrthographicLens;
    lens->set_film_size(2, 2);
    lens->set_film_offset(0, 0);
    lens->set_near_far(-1000, 1000);
    backcam_node->set_lens(lens);
    dr->set_clear_depth_active(false); // set here rather than reset below
    dr->set_incomplete_render(false);
    dr->set_camera(backcam);
    /// end of replacement for make_camera2d
    background.set_depth_test(false);
    background.set_depth_write(false);

    // Obtain two texture cards. One renders before the dragon, the other
    // after.
    auto render2d = window->get_render_2d();
    bcard = win->get_texture_card();
    bcard.reparent_to(background);
    bcard.set_transparency(TransparencyAttrib::M_alpha);
    fcard = win->get_texture_card();
    fcard.reparent_to(render2d);
    fcard.set_transparency(TransparencyAttrib::M_alpha);

    // Initialize one of the nice effects.
    choose_effect_ghost();

    // Add the task that initiates the screenshots.
    auto task = new GenericAsyncTask("take_snapshot", take_snapshot, 0);
    framework.get_task_mgr().add(task);

    // Create some black squares on top of which we will
    // place the instructions.
    CardMaker blackmaker("blackmaker");
    blackmaker.set_color(0, 0, 0, 1);
    blackmaker.set_frame(-1.00, -0.50, 0.65, 1.00);
    auto instcard = NodePath(blackmaker.generate());
    instcard.reparent_to(render2d);
    blackmaker.set_frame(-0.5, 0.5, -1.00, -0.85);
    auto titlecard = NodePath(blackmaker.generate());
    titlecard.reparent_to(render2d);

    // Panda does its best to hide the differences between DirectX and
    // OpenGL.  But there are a few differences that it cannot hide.
    // One such difference is that when OpenGL copies from a
    // visible window to a texture, it gets it right-side-up.  When
    // DirectX does it, it gets it upside-down.  There is nothing panda
    // can do to compensate except to expose a flag and let the
    // application programmer deal with it.  You should only do this
    // in the rare event that you're copying from a visible window
    // to a texture.
    if(win->get_gsg()->get_copy_texture_inverted()) {
	std::cout << "Copy texture is inverted.\n";
	bcard.set_scale(1, 1, -1);
	fcard.set_scale(1, 1, -1);
    }

    // Put up the instructions
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto node = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Motion Trails");
    text_node->set_text_color(1, 1, 1, 1);
    node.set_pos(0, 0, -1 + 0.1); // BottomCenter == (0, 0, -1)
    node.set_scale(0.08);
    text_node->set_align(TextNode::A_center); // default

    add_instructions(0.06, "Press ESC to exit");
    add_instructions(0.12, "Press 1: Ghost effect");
    add_instructions(0.18, "Press 2: Paint Brush effect");
    add_instructions(0.24, "Press 3: Double Vision effect");
    add_instructions(0.30, "Press 4: Wings of Blue effect");
    add_instructions(0.36, "Press 5: Whirlpool effect");

    // Enable the key events
    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("1", "", EV_FN() { choose_effect_ghost(); }, 0);
    framework.define_key("2", "", EV_FN() { choose_effect_paint_brush(); }, 0);
    framework.define_key("3", "", EV_FN() { choose_effect_double_vision(); }, 0);
    framework.define_key("4", "", EV_FN() { choose_effect_wings_of_blue(); }, 0);
    framework.define_key("5", "", EV_FN() { choose_effect_whirlpool(); }, 0);
}

AsyncTask::DoneStatus take_snapshot(GenericAsyncTask *task, void *)
{
    auto win = window->get_graphics_window();
    if(!win) // closed
	return AsyncTask::DS_done;
    auto time = task->get_elapsed_time();
    if(time > nextclick) {
	nextclick += 1.0 / clickrate;
	if(nextclick < time)
	    nextclick = time;
	win->trigger_copy();
    }
    return AsyncTask::DS_cont;
}

void choose_effect_ghost()
{
    window->set_background_type(WindowFramework::BT_black);
    bcard.hide();
    fcard.show();
    fcard.set_color(1.0, 1.0, 1.0, 0.99);
    fcard.set_scale(1.00);
    fcard.set_pos(0, 0, 0);
    fcard.set_r(0);
    clickrate = 30;
    nextclick = 0;
}

void choose_effect_paint_brush()
{
    window->set_background_type(WindowFramework::BT_black);
    bcard.show();
    fcard.hide();
    bcard.set_color(1, 1, 1, 1);
    bcard.set_scale(1.0);
    bcard.set_pos(0, 0, 0);
    bcard.set_r(0);
    clickrate = 10000;
    nextclick = 0;
}

void choose_effect_double_vision()
{
    window->set_background_type(WindowFramework::BT_black);
    bcard.show();
    bcard.set_color(1, 1, 1, 1);
    bcard.set_scale(1.0);
    bcard.set_pos(-0.05, 0, 0);
    bcard.set_r(0);
    fcard.show();
    fcard.set_color(1, 1, 1, 0.60);
    fcard.set_scale(1.0);
    fcard.set_pos(0.05, 0, 0);
    fcard.set_r(0);
    clickrate = 10000;
    nextclick = 0;
}

void choose_effect_wings_of_blue()
{
    window->set_background_type(WindowFramework::BT_black);
    fcard.hide();
    bcard.show();
    bcard.set_color(1.0, 0.90, 1.0, 254.0 / 255.0);
    bcard.set_scale(1.1, 1, 0.95);
    bcard.set_pos(0, 0, 0.05);
    bcard.set_r(0);
    clickrate = 30;
    nextclick = 0;
}

void choose_effect_whirlpool()
{
    window->set_background_type(WindowFramework::BT_black);
    bcard.show();
    fcard.hide();
    bcard.set_color(1, 1, 1, 1);
    bcard.set_scale(0.999);
    bcard.set_pos(0, 0, 0);
    bcard.set_r(1);
    clickrate = 10000;
    nextclick = 0;
}

void init(void)
{
    init_trails();

    auto render = window->get_render();
    auto dancer = def_load_model("models/dancer");
    dancer.reparent_to(render);
    // Scan for the character (should be first child)
    //auto character = DCAST(Character, dancer.find("+Character").node());
    auto character = DCAST(Character, dancer.get_child(0).node());
    // Scan for the anim bundlue (should be 2nd child)
    //auto anim_node = DCAST(AnimBundleNode, dancer.find("**/+AnimBundleNode").node());
    auto anim_node = DCAST(AnimBundleNode, dancer.get_child(1).node());
    auto anim = character->get_bundle(0)->bind_anim(anim_node->get_bundle(), ANIM_BIND_FLAGS);
    anim->set_anim_model(anim_node);
    (new CharAnimate(anim))->loop();
    // auto spin = new NPAnim(dancer, "spin", 15);
    // spin->set_end_hpr({360, 0, 0});
    // spin->loop();

    // put some lighting on the model
    auto dlight = new DirectionalLight("dlight");
    auto alight = new AmbientLight("alight");
    auto dlnp = render.attach_new_node(dlight);
    auto alnp = render.attach_new_node(alight);
    dlight->set_color(LColor(1.0, 0.9, 0.8, 1));
    alight->set_color(LColor(0.2, 0.3, 0.4, 1));
    dlnp.set_hpr(0, -60, 0);
    render.set_light(dlnp);
    render.set_light(alnp);
}
}

int main(int argc, const char **argv)
{
    if(argc > 1)
	sample_path = argv[1];

    init();

    framework.main_loop();
    kill_intervals();
    framework.close_framework();
    return 0;
}
