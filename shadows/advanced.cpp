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
#include <panda3d/shaderPool.h>
#include <panda3d/cardMaker.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Globals
PandaFramework framework;
WindowFramework *window;
PT(GraphicsOutput) buffer;
PN_stdfloat push_bias, ambient;
int camera_selection, light_selection;
NodePath panda_model, teapot;
PT(NPAnim) panda_movement, teapot_movement;
PT(AnimControl) panda_walk;
NodePath cam;
Camera *cam_node;
TextNode *inst_a;

// Function to put instructions on the screen.
TextNode *add_instructions(PN_stdfloat pos, const std::string &msg)
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
    return text_node;
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
void adjust_push_bias(PN_stdfloat);

void init(void)
{
    framework.open_framework();
    update_intervals();
    framework.set_window_title("Shadows (advanced) - C++ Panda3D Samples");
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

    // creating the offscreen buffer.

    WindowProperties winprops;
    winprops.set_size(512, 512);
    FrameBufferProperties props;
    props.set_rgb_color(1);
    props.set_alpha_bits(1);
    props.set_depth_bits(1);
    // TJM: Note: Python sample used LBuffer as a "local" alias, but there is
    // no need.  If you don't make it volatile, the optimizer will not
    // (slowly) re-read the global variable on every use.
    // In fact, even though some of the "local" versions are not exported,
    // I have removed the L prefix on all of them.
    buffer = framework.get_graphics_engine()->make_output(
        win->get_pipe(), "offscreen buffer", -2,
        props, winprops,
	GraphicsPipe::BF_refuse_window,
        win->get_gsg(), win);

    if(!buffer) {
	add_title(
            "Shadow Demo: Video driver cannot create an offscreen buffer.");
	return;
    }

    PT(Texture) depthmap = new Texture;
    buffer->add_render_texture(depthmap, GraphicsOutput::RTM_bind_or_copy,
			       GraphicsOutput::RTP_depth_stencil);
    if(win->get_gsg()->get_supports_shadow_filter()) {
	depthmap->set_minfilter(SamplerState::FT_shadow);
	depthmap->set_magfilter(SamplerState::FT_shadow);
    }

    // Adding a color texture is totally unnecessary, but it helps with
    // debugging.
    PT(Texture) colormap = new Texture;
    buffer->add_render_texture(colormap, GraphicsOutput::RTM_bind_or_copy,
			       GraphicsOutput::RTP_color);

    add_instructions(0.06, "P : stop/start the Panda Rotation");
    add_instructions(0.12, "W : stop/start the Walk Cycle");
    add_instructions(0.18, "T : stop/start the Teapot");
    add_instructions(0.24, "L : move light source far or close");
    // buffer_viewer is Python-only; v key removed.
    // add_instructions(0.30, "V : View the Depth-Texture results");
    // also moved following 3 up by .06
    add_instructions(0.30, "U : toggle updating the shadow map");
    add_instructions(0.36, "Left/Right Arrow : switch camera angles");
    inst_a = add_instructions(0.42, "Something about A/Z and push bias");

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
    auto floor = render.attach_new_node("floor");
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
    panda_model.set_shader_input("scale", LVector4(0.01, 0.01, 0.01, 1.0));
    panda_walk->set_play_rate(1.8);
    panda_walk->loop(true);
    panda_movement = new NPAnim(panda_axis, "panda_movement", 20.0);
    panda_movement->set_end_hpr(LPoint3(-360, 0, 0));
    panda_movement->set_start_hpr(LPoint3(0, 0, 0));
    panda_movement->loop();

    teapot = window->load_model(render, "teapot");
    teapot.set_pos(0, -20, 10);
    teapot.set_shader_input("texDisable", LVector4(1, 1, 1, 1));
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
    framework.define_key("a", "", EV_FN() { adjust_push_bias(1.1); }, 0);
    framework.define_key("z", "", EV_FN() { adjust_push_bias(0.9); }, 0);

    cam = window->make_camera();
    auto dr = buffer->make_display_region();
    dr->set_camera(cam);
    cam.reparent_to(render);
    cam_node = DCAST(Camera, cam.node());
    cam_node->set_scene(render);
    cam_node->get_lens()->set_fov(40);
    cam_node->get_lens()->set_near_far(10, 100);

    // default values
    push_bias = 0.04;
    ambient = 0.2;
    camera_selection = 0;
    light_selection = 0;

    // setting up shader
    render.set_shader_input("light", cam);
    render.set_shader_input("Ldepthmap", depthmap);
    render.set_shader_input("ambient", LVector4(ambient, 0, 0, 1.0));
    render.set_shader_input("texDisable", LVector4(0, 0, 0, 0));
    render.set_shader_input("scale", LVector4(1, 1, 1, 1));

    // Put a shader on the Light camera.
    NodePath lci("Light Camera Initializer");
    lci.set_shader(ShaderPool::load_shader("caster.sha"));
    cam_node->set_initial_state(lci.get_state());

    // Put a shader on the Main camera.
    // Some video cards have special hardware for shadow maps.
    // If the card has that, use it.  If not, use a different
    // shader that does not require hardware support.

    NodePath mci("Main Camera Initializer");
    if(win->get_gsg()->get_supports_shadow_filter())
	mci.set_shader(ShaderPool::load_shader("shadow.sha"));
    else
	mci.set_shader(ShaderPool::load_shader("shadow-nosupport.sha"));
    window->get_camera(0)->set_initial_state(mci.get_state());

    increment_camera_position(0);
    increment_light_position(0);
    adjust_push_bias(1.0);
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
	cam_node->hide_frustum();
	break;
      case 1:
	cam.reparent_to(panda_model);
	cam.set_pos(7, -3, 9);
	cam.look_at(0, 0, 0);
	cam_node->hide_frustum();
	break;
      case 2:
	cam.reparent_to(panda_model);
	cam.set_pos(-7, -3, 9);
	cam.look_at(0, 0, 0);
	cam_node->hide_frustum();
	break;
      case 3:
	cam.reparent_to(render);
	cam.set_pos(7, -23, 12);
	cam.look_at(teapot);
	cam_node->hide_frustum();
	break;
      case 4:
	cam.reparent_to(render);
	cam.set_pos(-7, -23, 12);
	cam.look_at(teapot);
	cam_node->hide_frustum();
	break;
      case 5:
	cam.reparent_to(render);
	cam.set_pos(1000, 0, 195);
	cam.look_at(0, 0, 0);
	cam_node->show_frustum();
	break;
    }
}

void increment_light_position(int n)
{
    light_selection = (light_selection + n) % 2;
    switch(light_selection) {
      case 0:
	cam.set_pos(0, -40, 25);
	cam.look_at(0, -10, 0);
	cam_node->get_lens()->set_near_far(10, 100);
	break;
      case 1:
	cam.set_pos(0, -600, 200);
	cam.look_at(0, -10, 0);
	cam_node->get_lens()->set_near_far(10, 1000);
	break;
    }
}

void adjust_push_bias(PN_stdfloat inc)
{
    push_bias *= inc;
    inst_a->set_text(
        "A/Z: Increase/Decrease the Push-Bias [" + std::to_string(push_bias) + ']');
    window->get_render().set_shader_input("push", push_bias, 0.0);
}
}

// Python code had check for "am I a program", but why wouldn't I be?

int main(int argc, char **argv)
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
