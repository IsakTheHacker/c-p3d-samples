/*
 * Translation of Python culling sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/culling
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Josh Enes
 * Last Updated: 2015-03-13
 *
 * This is a demo of Panda's occluder-culling system. It demonstrates loading
 * occluder from an EGG file and adding them to a CullTraverser.
 */
#include <pandaFramework.h>
#include <texturePool.h>
#include <occluderNode.h>
#include <load_prc_file.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
Randomizer rands;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;
bool xray_mode, show_model_bounds;
enum k_t {
    k_left, k_right, k_up, k_down, k_w, k_a, k_s, k_d, k_num, k_pressed = 32
};
const char * const key_names[] = {
    "arrow_left", "arrow_right", "arrow_up", "arrow_down", "w", "a", "s", "d"
};
int keys[k_num] = {};
PN_stdfloat heading, pitch;
NodePath level_model, occluder_model;
std::vector<NodePath> models;

// Function to put instructions on the screen.
void add_instructions(PN_stdfloat pos, const std::string &msg)
{
    TextNode *text_node = new TextNode("instructions");
    auto path = NodePath(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    path.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_left);
    path.set_pos(-1.0+0.08, 0, 1 - pos - 0.04); // TopLeft == (-1,0,1)
    path.set_scale(0.05);
}

// Function to put title on the screen.
void add_title(const std::string &text)
{
    TextNode *text_node = new TextNode("title");
    auto path = NodePath(text_node);
    text_node->set_text(text);
    // style = 1 -> plain (default)
    path.set_pos(1.0-0.1, 0, -1 + 0.09); // BottomRight == (1,0,-1)
    path.set_scale(0.08);
    path.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
}

void push_key(const Event *, void *);
void toggle_xray_mode(const Event *, void *);
void toggle_model_bounds(const Event *, void *);
AsyncTask::DoneStatus update(GenericAsyncTask *, void *);

// Sets up the game, camera, controls, and loads models.
void init()
{
    // Load PRC data
    window_title = "Occluder Demo";
    sync_video = false;
    //show_frame_rate_meter = true; // ???can't link???
    load_prc_file_data("", "show_frame_rate_meter = true");
    texture_minfilter = SamplerState::FT_linear_mipmap_linear;
    //fake_view_frustum_cull = true; // show culled nodes in red  // ???can't link???
    //load_prc_file_data("", "fake_view_frustum_cull = true");

    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();

    //Set the window title and open new window
    //framework.set_window_title("Cull - C++ Panda3D Samples");
    window = framework.open_window();

    xray_mode = false;
    show_model_bounds = false;

    // Display instructions
    add_title("Panda3D Tutorial: Occluder Culling");
    add_instructions(0.06, "[Esc]: Quit");
    add_instructions(0.12, "[W]: Move Forward");
    add_instructions(0.18, "[A]: Move Left");
    add_instructions(0.24, "[S]: Move Right");
    add_instructions(0.30, "[D]: Move Back");
    add_instructions(0.36, "Arrow Keys: Look Around");
    add_instructions(0.42, "[F]: Toggle Wireframe");
    add_instructions(0.48, "[X]: Toggle X-Ray Mode");
    add_instructions(0.54, "[B]: Toggle Bounding Volumes");

    // Setup controls
    window->enable_keyboard();
    for(unsigned long i = 0; i < k_num; i++) {
	const std::string kname(key_names[i]);
	framework.define_key(kname, "", push_key, (void *)(i | k_pressed));
	framework.define_key("shift-" + kname, "", push_key, (void *)(i | k_pressed));
	framework.define_key(kname + "-up", "", push_key, (void *)(i));
    }
    framework.define_key("f", "", framework.event_w, 0);
    framework.define_key("x", "", toggle_xray_mode, 0);
    framework.define_key("b", "", toggle_model_bounds, 0);
    framework.define_key("escape", "", framework.event_esc, &framework);
    //window->disable_mouse();

    // Setup camera
    PT(PerspectiveLens) lens = new PerspectiveLens;
    lens->set_fov(60);
    lens->set_near(0.01);
    lens->set_far(1000.0);
    window->get_camera(0)->set_lens(lens);
    window->get_camera_group().set_pos(-9, -0.5, 1);
    heading = -95.0;
    pitch = 0.0;

    // Load level geometry
    level_model = def_load_model("models/level");
    level_model.reparent_to(window->get_render());
    level_model.set_tex_gen(TextureStage::get_default(),
			    TexGenAttrib::M_world_position);
    level_model.set_tex_projector(TextureStage::get_default(),
				  window->get_render(), level_model);
    level_model.set_tex_scale(TextureStage::get_default(), 4);
    auto tex = TexturePool::load_3d_texture(sample_path + "models/tex_#.png");
    level_model.set_texture(tex);

    // Load occluders
    occluder_model = def_load_model("models/occluders");
    auto occluder_nodepaths = occluder_model.find_all_matches("**/+OccluderNode");
    for(int i = 0; i < occluder_nodepaths.size(); i++) {
	auto &&occluder_nodepath = occluder_nodepaths[i];
	window->get_render().set_occluder(occluder_nodepath);
	DCAST(OccluderNode,occluder_nodepath.node())->set_double_sided(true);
    }

    // Randomly spawn some models to test the occluders
    models.clear();
    auto box_model = window->load_model(framework.get_models(), "box");

    for(int i = 0; i < 500; i++) {
	auto pos = LPoint3(rands.random_real(9) - 4.5,
			   rands.random_real(9) - 4.5,
			   rands.random_real(8));
	auto box = box_model.copy_to(window->get_render());
	box.set_scale(rands.random_real(0.2) + 0.1);
	box.set_pos(pos);
	box.set_hpr(rands.random_real(360),
		    rands.random_real(360),
		    rands.random_real(360));
	box.reparent_to(window->get_render());
	models.push_back(box);
    }

    auto task = new GenericAsyncTask("main loop", update, 0);
    framework.get_task_mgr().add(task);
}

// Stores a value associated with a key.
void push_key(const Event *, void *val)
{
    auto key = reinterpret_cast<unsigned long>(val);
    keys[key & (k_pressed - 1)] = key >= k_pressed ? 1 : 0;
}

// Updates the camera based on the keyboard input.
AsyncTask::DoneStatus update(GenericAsyncTask *, void *)
{
    double delta = ClockObject::get_global_clock()->get_dt();
    auto move_x = delta * 3 * -keys[k_a] + delta * 3 * keys[k_d];
    auto move_z = delta * 3 * keys[k_s] + delta * 3 * -keys[k_w];
    auto camera = window->get_camera_group();
    camera.set_pos(camera, move_x, -move_z, 0);
    heading += (delta * 90 * keys[k_left] +
		delta * 90 * -keys[k_right]);
    pitch += (delta * 90 * keys[k_up] +
	      delta * 90 * -keys[k_down]);
    camera.set_hpr(heading, pitch, 0);
    return AsyncTask::DS_cont;
}

// Toggle X-ray mode on and off. This is useful for seeing the
// effectiveness of the occluder culling.
void toggle_xray_mode(const Event *, void *)
{
    xray_mode = !xray_mode;
    if(xray_mode) {
	level_model.set_color_scale(1, 1, 1, 0.5);
	level_model.set_transparency(TransparencyAttrib::M_dual);
    } else {
	level_model.set_color_scale_off();
	level_model.set_transparency(TransparencyAttrib::M_none);
    }
}

// Toggle bounding volumes on and off on the models.
void toggle_model_bounds(const Event *, void *)
{
    show_model_bounds = !show_model_bounds;
    if(show_model_bounds) {
	for(auto model: models)
	    model.show_bounds();
    } else {
	for(auto model: models)
	    model.hide_bounds();
    }
}
}

int main(int argc, char* argv[])
{
    if(argc > 1)
	sample_path = argv[1];

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
