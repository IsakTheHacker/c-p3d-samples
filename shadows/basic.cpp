/*
 *  Translation of Python shadows sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/shadows
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample.
 * Unfortunately, there are very few.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/texturePool.h>
#include <panda3d/cardMaker.h>
#include <panda3d/spotlight.h>
#include <panda3d/ambientLight.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Globals
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;
int camera_selection, light_selection;
NodePath panda_model, teapot, light;
PT(Spotlight) slight;
PT(NPAnim) panda_movement, teapot_movement;
PT(AnimControl) panda_walk;

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
    text.set_pos(-1/a2d.get_sx() + 0.08, 0, 1 - pos - 0.04); // TopLeft = (-1, 0, 1)
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
    node.set_scale(0.07);
    text_node->set_align(TextNode::A_right);
    node.set_pos(1/a2d.get_sx()-0.1, 0, -1 + 0.09); // BottomRight == (1, 0, -1)
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

void increment_camera_position(int), increment_light_position(int);
void toggle_interval(CInterval *), toggle_anim(AnimInterface *),
     toggle_update_shadow_map(void);

void init(void)
{
    framework.open_framework();
    update_intervals();
    framework.set_window_title("Shadows (basic) - C++ Panda3D Samples");
    window = framework.open_window();

    // Preliminary capabilities check.
    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    auto win = window->get_graphics_window();
    if(!win->get_gsg()->get_supports_basic_shaders()) {
	add_title(
            "Shadow Demo: Video driver reports that shaders are not supported.");
	return;
    }
    if(!win->get_gsg()->get_supports_depth_texture()) {
	add_title(
            "Shadow Demo: Video driver reports that depth textures are not supported.");
	return;
    }

    add_instructions(0.06, "P : stop/start the Panda Rotation");
    add_instructions(0.12, "W : stop/start the Walk Cycle");
    add_instructions(0.18, "T : stop/start the Teapot");
    add_instructions(0.24, "L : move light source far or close");
    // buffer_viewer is Python-only; v key removed.
    // add_instructions(0.30, "V : View the Depth-Texture results");
    // also moved following 3 up by .06
    add_instructions(0.30, "U : toggle updating the shadow map");
    add_instructions(0.36, "Left/Right Arrow : switch camera angles");

    auto bgctrl = window->get_display_region_3d();
    bgctrl->set_clear_color(LColor(0, 0, 0.2, 1));
    bgctrl->set_clear_color_active(true);

    auto cam_lens = window->get_camera(0)->get_lens();
    cam_lens->set_near_far(1.0, 10000);
    cam_lens->set_fov(75);

    // Load the scene.

    auto floor_tex = TexturePool::load_texture("maps/envir-ground.jpg");
    CardMaker cm("");
    cm.set_frame(-2, 2, -2, 2);
    auto render = window->get_render();
    auto floor = render.attach_new_node(new PandaNode("floor"));
    for(int y = 0; y < 12; y++)
	for(int x = 0; x < 12; x++) {
	    auto nn = floor.attach_new_node(cm.generate());
	    nn.set_p(-90);
	    nn.set_pos((x - 6) * 4, (y - 6) * 4, 0);
	}
    floor.set_texture(floor_tex);
    floor.flatten_strong();

    auto panda_axis = render.attach_new_node("panda axis");
    panda_model = window->load_model(panda_axis, "panda-model");
    panda_walk = load_anim(panda_model, "panda-walk4");
    panda_model.set_pos(9, 0, 0);
    panda_model.set_scale(0.01);
    panda_walk->set_play_rate(1.8);
    panda_walk->loop(true);
    panda_movement = new NPAnim(panda_axis, "panda_movement", 20.0);
    panda_movement->set_end_hpr(LPoint3(-360, 0, 0));
    panda_movement->set_start_hpr(LPoint3(0, 0, 0));
    panda_movement->loop();

    teapot = window->load_model(render, "teapot");
    teapot.set_pos(0, -20, 10);
    teapot_movement = new NPAnim(teapot, "teapot_movement", 50);
    teapot_movement->set_end_hpr(LPoint3(0, 360, 360));
    teapot_movement->loop();

    framework.define_key("arrow_left", "", EV_FN() { increment_camera_position(-1); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { increment_camera_position(1); }, 0);
    framework.define_key("p", "", EV_FN() { toggle_interval(panda_movement); }, 0);
    framework.define_key("t", "", EV_FN() { toggle_interval(teapot_movement); }, 0);
    framework.define_key("w", "", EV_FN() { toggle_anim(panda_walk); }, 0);
    // framework.define_key("v", "", EV_FN() { window->buffer_viewer.toggle_enable(); }, 0);
    framework.define_key("u", "", EV_FN() { toggle_update_shadow_map(); }, 0);
    framework.define_key("l", "", EV_FN() { increment_light_position(1); }, 0);
    // framework.define_key("o", "", EV_FN() { base.oobe) // too much trouble

    slight = new Spotlight("Spot");
    light = render.attach_new_node(slight);
    slight->set_scene(render);
    slight->set_shadow_caster(true);
    slight->show_frustum();
    slight->get_lens()->set_fov(40);
    slight->get_lens()->set_near_far(10, 100);
    render.set_light(light);

    auto alight = new AmbientLight("Ambient");
    auto ambient = render.attach_new_node(alight);
    alight->set_color(LVector4(0.2, 0.2, 0.2, 1));
    render.set_light(ambient);

    // Important! Enable the shader generator.
    render.set_shader_auto();

    // default values
    camera_selection = 0;
    light_selection = 0;

    increment_camera_position(0);
    increment_light_position(0);
}

void toggle_interval(CInterval *ival)
{
    if(ival->is_playing())
	ival->pause();
    else
	ival->resume();
}

void toggle_anim(AnimInterface *anim)
{
    if(anim->is_playing())
	anim->stop();
    else
	anim->loop(false);
}

void toggle_update_shadow_map()
{
    auto win = window->get_graphics_window();
    auto buffer = DCAST(GraphicsOutput, slight->get_shadow_buffer(win->get_gsg()));
    buffer->set_active(!buffer->is_active());
}

void increment_camera_position(int n)
{
    camera_selection = (camera_selection + n) % 6;
    auto cam = window->get_camera_group();
    auto render = window->get_render();
    switch(camera_selection) {
      case 0:
	cam.reparent_to(render);
	cam.set_pos(30, -45, 26);
	cam.look_at(0, 0, 0);
	slight->hide_frustum();
	break;
      case 1:
	cam.reparent_to(panda_model);
	cam.set_pos(7, -3, 9);
	cam.look_at(0, 0, 0);
	slight->hide_frustum();
	break;
      case 2:
	cam.reparent_to(panda_model);
	cam.set_pos(-7, -3, 9);
	cam.look_at(0, 0, 0);
	slight->hide_frustum();
	break;
      case 3:
	cam.reparent_to(render);
	cam.set_pos(7, -23, 12);
	cam.look_at(teapot);
	slight->hide_frustum();
	break;
      case 4:
	cam.reparent_to(render);
	cam.set_pos(-7, -23, 12);
	cam.look_at(teapot);
	slight->hide_frustum();
	break;
      case 5:
	cam.reparent_to(render);
	cam.set_pos(1000, 0, 195);
	cam.look_at(0, 0, 0);
	slight->show_frustum();
	break;
    }
}

void increment_light_position(int n)
{
    light_selection = (light_selection + n) % 2;
    switch(light_selection) {
      case 0:
	light.set_pos(0, -40, 25);
	light.look_at(0, -10, 0);
	slight->get_lens()->set_near_far(10, 100);
	break;
      case 1:
	light.set_pos(0, -600, 200);
	light.look_at(0, -10, 0);
	slight->get_lens()->set_near_far(10, 1000);
	break;
    }
}
}

// Python code had check for "am I a program", but why wouldn't I be?

int main(int argc, char **argv)
{
    if(argc > 1)
	sample_path = argv[1];
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    kill_intervals();
    framework.close_framework();
    return 0;
}
