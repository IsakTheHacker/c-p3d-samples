/*
 * Translation of Python bump-mapping sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/bump-mapping
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Bump mapping is a way of making polygonal surfaces look
 * less flat.  This sample uses normal mapping for all
 * surfaces, and also parallax mapping for the column.
 *
 * This is a tutorial to show how to do normal mapping
 * in panda3d using the Shader Generator.
 */
#include <pandaFramework.h>
#include <ambientLight.h>
#include <pointLight.h>
#include <lodNode.h> // parallax config
#include <load_prc_file.h> // but actually...

#include "anim_supt.h"

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
NodePath room, lightpivot;
TextNode *inst5;
LVector3 focus;
PN_stdfloat heading, pitch, mousex, mousey, last;
bool mousebtn[3];
bool shaderenable;

// Function to put instructions on the screen.
TextNode *add_instructions(PN_stdfloat pos, const std::string &msg)
{
    TextNode *text_node = new TextNode("instructions");
    auto text = NodePath(text_node);
    text_node->set_text(msg);
    text_node->set_text_color(1, 1, 1, 1);
    // style = 1 -> plain (default)
    text.set_scale(0.05);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text.reparent_to(window->get_aspect_2d()); // a2d
    text.set_pos(-1.0+0.08, 0, 1 - pos - 0.04); // TopLeft == (-1,0,1)
    text_node->set_align(TextNode::A_left);
    return text_node;
}

// Function to put title on the screen.
void add_title(const std::string &text)
{
    TextNode *text_node = new TextNode("title");
    auto node = NodePath(text_node);
    text_node->set_text(text);
    text_node->set_text_color(1, 1, 1, 1);
    // style = 1 -> plain (default)
    node.set_scale(0.08);
    node.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    node.set_pos(1.0-0.1, 0, -1 + 0.09); // BottomRight == (1,0,-1)
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

AsyncTask::DoneStatus control_camera(GenericAsyncTask *, void *);
void rotate_light(int offset);
void rotate_cam(int offset);
void toggle_shader();

void init(void)
{
    // Configure the parallax mapping settings (these are just the defaults)
    //parallax_mapping_samples = 3; ?? won't link ??
    //parallax_mapping_scale = 0.1; ?? won't link ??
    load_prc_file_data("", "parallax-mapping-samples 3\n"
		           "parallax-mapping-scale 0.1");

    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Bump Mapping - C++ Panda3D Samples");
    window = framework.open_window();

    // Check video card capabilities.
    if(!window->get_graphics_window()->get_gsg()->get_supports_basic_shaders()) {
	add_title("Bump Mapping: "
		  "Video driver reports that Cg shaders are not supported.");
	exit(1);
    }

    // Post the instructions
    add_title("Panda3D: Tutorial - Bump Mapping");
    add_instructions(0.06, "Press ESC to exit");
    add_instructions(0.12, "Move mouse to rotate camera");
    add_instructions(0.18, "Left mouse button: Move forwards");
    add_instructions(0.24, "Right mouse button: Move backwards");
    inst5 = add_instructions(0.30, "Enter: Turn bump maps Off");

    // Load the 'abstract room' model.  This is a model of an
    // empty room containing a pillar, a pyramid, and a bunch
    // of exaggeratedly bumpy textures.

    room = window->load_model(framework.get_models(), sample_path + "models/abstractroom");
    room.reparent_to(window->get_render());

    // Make the mouse invisible, turn off normal mouse controls
    // framework.disable_mouse() -- not enabled by default
    WindowProperties props;
    props.set_cursor_hidden(true);
    window->get_graphics_window()->request_properties(props);
    auto camera = window->get_camera(0);
    camera->get_lens()->set_fov(60);

    // Set the current viewing target
    focus = LVector3(55, -55, 20);
    heading = 180;

    // Start the camera control task:
    auto task = new GenericAsyncTask("camera-task", control_camera, 0);
    framework.get_task_mgr().add(task);
    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("mouse1", "", EV_FN() { mousebtn[0] = true; }, 0);
    framework.define_key("mouse1-up", "", EV_FN() { mousebtn[0] = false; }, 0);
    framework.define_key("mouse2", "", EV_FN() { mousebtn[1] = true; }, 0);
    framework.define_key("mouse2-up", "", EV_FN() { mousebtn[1] = false; }, 0);
    framework.define_key("mouse3", "", EV_FN() { mousebtn[2] = true; }, 0);
    framework.define_key("mouse3-up", "", EV_FN() { mousebtn[2] = false; }, 0);
    framework.define_key("enter", "", EV_FN() { toggle_shader(); }, 0);
    framework.define_key("j", "", EV_FN() { rotate_light(-1); }, 0);
    framework.define_key("k", "", EV_FN() { rotate_light(1); }, 0);
    framework.define_key("arrow_left", "", EV_FN() { rotate_cam(-1); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { rotate_cam(1); }, 0);

    // Add a light to the scene.
    lightpivot = window->get_render().attach_new_node("lightpivot");
    lightpivot.set_pos(0, 0, 25);
    auto lpa = new NPAnim(lightpivot, "lightpivot", 10);
    lpa->set_end_hpr(LPoint3(360, 0, 0));
    lpa->loop();
    auto plight = new PointLight("plight");
    plight->set_color(LColor(1, 1, 1, 1));
    plight->set_attenuation(LVector3(0.7, 0.05, 0));
    auto plnp = lightpivot.attach_new_node(plight);
    plnp.set_pos(45, 0, 0);
    room.set_light(plnp);

    // Add an ambient light
    auto alight = new AmbientLight("alight");
    alight->set_color(LColor(0.2, 0.2, 0.2, 1));
    auto alnp = window->get_render().attach_new_node(alight);
    room.set_light(alnp);

    // Create a sphere to denote the light
    auto sphere = def_load_model("models/icosphere");
    sphere.reparent_to(plnp);

    // Tell Panda that it should generate shaders performing per-pixel
    // lighting for the room.
    room.set_shader_auto();

    shaderenable = true;
}

void rotate_light(int offset)
{
    lightpivot.set_h(lightpivot.get_h() + offset * 20);
}

void rotate_cam(int offset)
{
    heading -= offset * 10;
}

void toggle_shader()
{
    if (shaderenable) {
	inst5->set_text("Enter: Turn bump maps On");
	shaderenable = false;
	room.set_shader_off();
    } else {
	inst5->set_text("Enter: Turn bump maps Off");
	shaderenable = true;
	room.set_shader_auto();
    }
}

AsyncTask::DoneStatus control_camera(GenericAsyncTask *task, void *data)
{
    // figure out how much the mouse has moved (in pixels)
    auto win = window->get_graphics_window();
    auto md = win->get_pointer(0);
    auto x = md.get_x();
    auto y = md.get_y();
    if(win->move_pointer(0, 100, 100)) {
	heading = heading - (x - 100) * 0.2;
	pitch = pitch - (y - 100) * 0.2;
        if(pitch < -45)
            pitch = -45;
        if(pitch > 45)
            pitch = 45;
    }
    auto camera = window->get_camera_group();
    camera.set_hpr(heading, pitch, 0);
    auto dir = camera.get_mat().get_row3(1);
    auto elapsed = task->get_elapsed_time() - last;
    if(!last)
	elapsed = 0;
    if(mousebtn[0])
	focus += dir * elapsed * 30;
    if(mousebtn[1] || mousebtn[2])
	focus -= dir * elapsed * 30;
    camera.set_pos(focus - (dir * 5));
    if(camera.get_x() < -59.0)
	camera.set_x(-59);
    if(camera.get_x() > 59.0)
	camera.set_x(59);
    if(camera.get_y() < -59.0)
	camera.set_y(-59);
    if(camera.get_y() > 59.0)
	camera.set_y(59);
    if(camera.get_z() < 5.0)
	camera.set_z(5);
    if(camera.get_z() > 45.0)
	camera.set_z(45);
    focus = camera.get_pos() + (dir * 5);
    last = task->get_elapsed_time();
    return AsyncTask::DS_cont;
}

int main(int argc, char* argv[]) {
    if(argc > 1)
	sample_path = argv[1];

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
