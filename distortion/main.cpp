/*
 *  Translation of Python distortion sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/distortion
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Tree Form starplant@gmail.com
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/texturePool.h>
#include <panda3d/shaderPool.h>

#include "anim_supt.h"

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
PT(GraphicsOutput) distortion_buffer;
NodePath distortion_camera, distortion_object;
bool distortion_on;

// Function to put instructions on the screen.
void add_instructions(PN_stdfloat pos, const std::string &msg)
{
    TextNode *text_node = new TextNode("instructions");
    auto text = NodePath(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text.reparent_to(window->get_aspect_2d()); // a2d
    text.set_pos(-1.0+-0.25, 0, 1 - pos); // TopLeft == (-1,0,1)
    text_node->set_align(TextNode::A_left);
    text.set_scale(0.05);
}

// Function to put title on the screen.
void add_title(const std::string &text)
{
    TextNode *text_node = new TextNode("title");
    auto node = NodePath(text_node);
    text_node->set_text(text);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    node.set_pos(1.0+0.25, 0, -1 + 0.05); // BottomRight == (1,0,-1)
    node.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    node.set_scale(0.07);
}

GraphicsOutput *make_FBO(const char *name);
void toggle_distortion(void);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Distortion - C++ Panda3D Samples");
    window = framework.open_window();

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    if(!window->get_graphics_window()->get_gsg()->get_supports_basic_shaders()) {
	add_title("Distortion Demo: Video driver says Cg shaders not supported.");
	return;
    }

    window->set_background_type(WindowFramework::BT_black);

    // Show the instructions
    add_title("Panda3D: Tutorial - Distortion Effect");
    add_instructions(0.04, "ESC: Quit");
    add_instructions(0.10, "Space: Toggle distortion filter On/Off");
#if 0
    add_instructions(0.16, "V: View the render-to-texture results");
#endif

    // Load background
    auto seascape = def_load_model("models/plane");
    seascape.reparent_to(window->get_render());
    seascape.set_pos_hpr(0, 145, 0, 0, 0, 0);
    seascape.set_scale(100);
    seascape.set_texture(def_load_texture("models/ocean.jpg"));

    // Create the distortion buffer. This buffer renders like a normal
    // scene,
    distortion_buffer = make_FBO("model buffer");
    distortion_buffer->set_sort(-3);
    distortion_buffer->set_clear_color(LColor(0, 0, 0, 0));

    // We have to attach a camera to the distortion buffer. The distortion camera
    // must have the same frustum as the main camera. As long as the aspect
    // ratios match, the rest will take care of it
    auto rcam = window->get_camera(0);
    distortion_camera = window->make_camera();
    auto cam = DCAST(Camera, distortion_camera.node());
    cam->set_lens(rcam->get_lens());
    cam->set_scene(window->get_render());
    cam->set_camera_mask(1<<4);
    auto dr = distortion_buffer->make_display_region();
    dr->set_camera(distortion_camera);

    // load the object with the distortion
    distortion_object = def_load_model("models/boat");
    distortion_object.set_scale(1);
    distortion_object.set_pos(0, 20, -3);
    auto spin = new NPAnim(distortion_object, "spin", 10);
    spin->set_end_hpr(LVector3(360, 0, 0));
    spin->loop();
    distortion_object.reparent_to(window->get_render());

    // Create the shader that will determime what parts of the scene will
    // distortion
    auto distortion_shader = def_load_shader("distortion.sha");
    distortion_object.set_shader(distortion_shader);
    distortion_object.hide(1<<4);

    // Textures
    auto tex1 = def_load_texture("models/water.png");
    distortion_object.set_shader_input("waves", tex1);

    auto tex_distortion = new Texture;
    distortion_buffer->add_render_texture(
            tex_distortion, GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_color);
    distortion_object.set_shader_input("screen", tex_distortion);

#if 0 /**** Buffer Viewer is a Direct/Python feature only *****/
    // Panda contains a built-in viewer that lets you view the results of
    // your render-to-texture operations.  This code configures the viewer.
    framework.define_key("v", buffer_viewer.toggle_enable, &buffer_viewer);
    framework.define_key("V", buffer_viewer.toggle_enable, &buffer_viewer);
    buffer_viewer.set_position("llcorner");
    buffer_viewer.set_layout("hline");
    buffer_viewer.set_card_size(0.652, 0);
#endif

    // event handling
    framework.define_key("space", "", EV_FN() { toggle_distortion(); }, 0);
    distortion_on = true;
}

GraphicsOutput *make_FBO(const char *name)
{
    // This routine creates an offscreen buffer.  All the complicated
    // parameters are basically demanding capabilities from the offscreen
    // buffer - we demand that it be able to render to texture on every
    // bitplane, that it can support aux bitplanes, that it track
    // the size of the host window, that it can render to texture
    // cumulatively, and so forth.
    WindowProperties winprops;
    FrameBufferProperties props;
    props.set_rgb_color(1);
    auto win = window->get_graphics_window();
    return framework.get_graphics_engine()->make_output(
            win->get_pipe(), "model buffer", -2, props, winprops,
            GraphicsPipe::BF_size_track_host | GraphicsPipe::BF_refuse_window,
            win->get_gsg(), win);
}

void toggle_distortion()
{
    // Toggles the distortion on/off.
    if(distortion_on)
	distortion_object.hide();
    else
	distortion_object.show();
    distortion_on = !distortion_on;
}
}

int main(int argc, char **argv)
{
    if(argc > 1)
	sample_path = argv[1];
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
