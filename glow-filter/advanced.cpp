/*
 * Translation of Python glow-filter sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/glow-filter
 *
 * Original C++ conversion by Thomas J. Moore July 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Kwasi Mensah (kmensah@andrew.cmu.edu)
 * Date: 7/25/2005
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/shaderPool.h>
#include <panda3d/orthographicLens.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/colorBlendAttrib.h>

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
NodePath tron, finalcard;
PT(CInterval) running;
PT(AnimControl) running_ctl;
PT(NPAnim) interval;
bool is_running, glow_on;
GraphicsOutput *glow_buffer;
Camera *glow_camera;

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
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_right);
    node.set_pos(1.0/a2d.get_sx()-0.1, 0, -1 + 0.09); // BottomRight == (1,0,-1)
    node.set_scale(0.08);
}

// This function is responsible for setting up the two blur filters.
// It just makes a temp Buffer, puts a screen aligned card, and then sets
// the appropiate shader to do all the work. Gaussian blurs are decomposable
// into a two-pass algorithm which is faster than the equivalent one-pass
// algorithm, so we do it in two passes: one pass that blurs in the horizontal
// direction, and one in the vertical direction.
GraphicsOutput *make_filter_buffer(GraphicsOutput *srcbuffer,
				   const std::string &name, int sort,
				   const std::string &prog)
{
    auto win = window->get_graphics_window();
    auto blur_buffer = win->make_texture_buffer(name, 512, 512);
    blur_buffer->set_sort(sort);
    blur_buffer->set_clear_color(LColor(1, 0, 0, 1));
    /// start of replacement for make_camera2d(blur_buffer)
    auto dr = blur_buffer->make_mono_display_region();
    dr->set_sort(10); // default sort
    PT(Camera) blur_camera = new Camera("camera2d");
    auto blur_camera_node = window->get_render().attach_new_node(blur_camera);
    auto lens = new OrthographicLens;
    lens->set_film_size(2, 2);
    lens->set_film_offset(0, 0);
    lens->set_near_far(-1000, 1000);
    blur_camera->set_lens(lens);
    dr->set_clear_depth_active(true);
    dr->set_incomplete_render(false);
    dr->set_camera(blur_camera_node);
    /// end of replacement for make_camera2d
    NodePath blur_scene("new Scene");
    blur_camera->set_scene(blur_scene);
    auto shader = def_load_shader(prog);
    auto card = srcbuffer->get_texture_card();
    card.reparent_to(blur_scene);
    card.set_shader(shader);
    return blur_buffer;
}

void toggle_glow(void), toggle_display(void);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    update_intervals();

    //Set the window title and open new window
    framework.set_window_title("Glow Filter (advanced) - C++ Panda3D Samples");
    window = framework.open_window();

    window->set_background_type(WindowFramework::BT_black);
    window->get_camera_group().set_pos(0, -50, 0);

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    // Check video card capabilities.
    if(!window->get_graphics_window()->get_gsg()->get_supports_basic_shaders()) {
	add_title("Glow Filter: Video driver reports that Cg shaders are not supported.");
	return;
    }

    // Post the instructions
    add_title("Panda3D: Tutorial - Glow Filter");
    add_instructions(0.06, "ESC: Quit");
    add_instructions(0.12, "Space: Toggle Glow Filter On/Off");
    add_instructions(0.18, "Enter: Toggle Running/Spinning");
//    add_instructions(0.24, "V: View the render-to-texture results");

    // Create the shader that will determime what parts of the scene will
    // glow
    auto glow_shader = def_load_shader("shaders/glowShader.sha");

    // load our model
    tron = def_load_model("models/tron");
    running_ctl = load_anim(tron, sample_path + "models/tron_anim");
    running = new CharAnimate(running_ctl);
    auto render = window->get_render();
    tron.reparent_to(render);
    interval = new NPAnim(tron, "interval", 60);
    interval->set_end_hpr(LPoint3(360, 0, 0));
    interval->loop();
    is_running = false;

    // put some lighting on the tron model
    auto dlight = new DirectionalLight("dlight");
    auto alight = new AmbientLight("alight");
    auto dlnp = render.attach_new_node(dlight);
    auto alnp = render.attach_new_node(alight);
    dlight->set_color(LColor(1.0, 0.7, 0.2, 1));
    alight->set_color(LColor(0.2, 0.2, 0.2, 1));
    dlnp.set_hpr(0, -60, 0);
    render.set_light(dlnp);
    render.set_light(alnp);

    // create the glow buffer. This buffer renders like a normal scene,
    // except that only the glowing materials should show up nonblack.
    auto win = window->get_graphics_window();
    glow_buffer = win->make_texture_buffer("Glow scene", 512, 512);
    glow_buffer->set_sort(-3);
    glow_buffer->set_clear_color(LColor(0, 0, 0, 1));

    // We have to attach a camera to the glow buffer. The glow camera
    // must have the same frustum as the main camera. As long as the aspect
    // ratios match, the rest will take care of itself.
    auto glow_camera_node = window->make_camera();
    glow_camera = DCAST(Camera, glow_camera_node.node());
    glow_camera->set_lens(window->get_camera(0)->get_lens());
    auto dr = glow_buffer->make_display_region();
    dr->set_camera(glow_camera_node);

    // Tell the glow camera to use the glow shader
    auto tempnode = NodePath(new PandaNode("temp node"));
    tempnode.set_shader(glow_shader);
    glow_camera->set_initial_state(tempnode.get_state());

    // set up the pipeline: from glow scene to blur x to blur y to main
    // window.
    auto blur_x_buffer = make_filter_buffer(
        glow_buffer,  "Blur X", -2, "shaders/XBlurShader.sha");
    auto blur_y_buffer = make_filter_buffer(
        blur_x_buffer, "Blur Y", -1, "shaders/YBlurShader.sha");
    finalcard = blur_y_buffer->get_texture_card();
    finalcard.reparent_to(window->get_render_2d());

    // This attribute is used to add the results of the post-processing
    // effects to the existing framebuffer image, rather than replace it.
    // This is mainly useful for glow effects like ours.
    finalcard.set_attrib(ColorBlendAttrib::make(ColorBlendAttrib::M_add));

#if 0 // Python only
    // Panda contains a built-in viewer that lets you view the results of
    // your render-to-texture operations.  This code configures the viewer.
    framework.define_key("v", "", buffer_viewer.toggle_enable, &buffer_viewer);
    framework.define_key("V", "", buffer_viewer.toggle_enable, &buffer_viewer);
    buffer_viewer.set_position("llcorner");
    buffer_viewer.set_layout("hline");
    buffer_viewer.set_card_size(0.652, 0);
#endif

    // event handling
    framework.define_key("space", "", EV_FN() { toggle_glow(); }, 0);
    framework.define_key("enter", "", EV_FN() { toggle_display(); }, 0);

    glow_on = true;
}

void toggle_glow()
{
    if(glow_on)
	finalcard.hide();
    else
	finalcard.show();
    glow_on = !glow_on;
}

void toggle_display()
{
    is_running = !is_running;
    auto camera = window->get_camera_group();
    if(!is_running) {
	camera.set_pos(0, -50, 0);
	running->finish();
	running_ctl->pose(0);
	interval->loop();
    } else {
	camera.set_pos(0, -170, 3);
	interval->finish();
	tron.set_hpr(0, 0, 0);
	running->loop();
    }
}
}

int main(int argc, const char **argv)
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
