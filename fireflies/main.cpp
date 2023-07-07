/*
 *  Translation of Python fireflies sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/fireflies
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Josh Yelon
 * Date: 7/11/2005
 *
 * See the associated manual page for an explanation.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/texturePool.h>
#include <panda3d/shaderPool.h>
#include <panda3d/alphaTestAttrib.h>
#include <panda3d/depthTestAttrib.h>
#include <panda3d/depthWriteAttrib.h>
#include <panda3d/colorBlendAttrib.h>
#include <panda3d/cullFaceAttrib.h>
#include <panda3d/cardMaker.h>
#include <panda3d/modelLoadRequest.h>

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
Randomizer rands;
PT(GraphicsOutput) modelbuffer, lightbuffer;
enum {
    model_mask = 1, light_mask = 2, plain_mask = 4
};
NodePath lightroot, modelroot;
NodePath forest;
TextNode *title, *inst2, *inst3;
    // Firefly parameters
std::vector<NodePath> fireflies, glowspheres;
std::vector<PT(CInterval)> sequences, scaleseqs;
PN_stdfloat fireflysize = 1.0;
NodePath spheremodel, firefly;
double nextadd = 0;

// Function to put instructions on the screen.
TextNode *add_instructions(PN_stdfloat pos, const std::string &msg)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("instructions");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text_node->set_align(TextNode::A_left);
    text.set_pos(-1.0/a2d.get_sx()+0.08, 0, 1 - pos - 0.04); // TopLeft == (-1,0,1)
    text.set_scale(0.05);
    return text_node;
}

// Function to put title on the screen.
TextNode *add_title(const std::string &text)
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
    return text_node;
}

GraphicsOutput *make_FBO(const char *name, int auxrgba);
void finish_loading(void);
void inc_firefly_count(PN_stdfloat), dec_firefly_count(PN_stdfloat),
     set_firefly_size(PN_stdfloat);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    update_intervals();

    //Set the window title and open new window
    framework.set_window_title("Fireflies - C++ Panda3D Samples");
    window = framework.open_window();
    window->set_background_type(WindowFramework::BT_black);

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    // Preliminary capabilities check.

    auto win = window->get_graphics_window();
    if(!win->get_gsg()->get_supports_basic_shaders()) {
	add_title("Firefly Demo: Video driver reports that Cg "
		  "shaders are not supported.");
	return;
    }
    if(!win->get_gsg()->get_supports_depth_texture()) {
	add_title("Firefly Demo: Video driver reports that depth "
		  "textures are not supported.");
	return;
    }

    // This algorithm uses two offscreen buffers, one of which has
    // an auxiliary bitplane, and the offscreen buffers share a single
    // depth buffer.  This is a heck of a complicated buffer setup.

    modelbuffer = make_FBO("model buffer", 1);
    lightbuffer = make_FBO("light buffer", 0);

    // Creation of a high-powered buffer can fail, if the graphics card
    // doesn't support the necessary OpenGL extensions.

    if(!modelbuffer || !lightbuffer) {
	add_title("Firefly Demo: Video driver does not support "
		  "multiple render targets");
	return;
    }

    // Create four render textures: depth, normal, albedo, and final.
    // attach them to the various bitplanes of the offscreen buffers.

    auto tex_depth = new Texture,
	tex_albedo = new Texture(),
	tex_normal = new Texture(),
	tex_final = new Texture();
    tex_depth->set_format(Texture::F_depth_stencil);

    modelbuffer->add_render_texture(tex_depth,
        GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_depth_stencil);
    modelbuffer->add_render_texture(tex_albedo,
        GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_color);
    modelbuffer->add_render_texture(tex_normal,
        GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_aux_rgba_0);

    lightbuffer->add_render_texture(tex_final,
        GraphicsOutput::RTM_bind_or_copy, GraphicsOutput::RTP_color);

    // Set the near and far clipping planes.

    auto cam = window->get_camera(0);
    auto lens = cam->get_lens();
    lens->set_near(50.0);
    lens->set_far(500.0);

    // This algorithm uses three cameras: one to render the models into the
    // model buffer, one to render the lights into the light buffer, and
    // one to render "plain" stuff (non-deferred shaded) stuff into the
    // light buffer.  Each camera has a bitmask to identify it.

    auto render = window->get_render();
    auto modelcam = window->make_camera();
    auto camm = DCAST(Camera, modelcam.node());
    camm->set_lens(lens);
    camm->set_scene(render);
    camm->set_camera_mask(model_mask);
    auto dr = modelbuffer->make_display_region();
    dr->set_camera(modelcam);
    auto lightcam = window->make_camera();
    auto caml = DCAST(Camera, lightcam.node());
    caml->set_lens(lens);
    caml->set_scene(render);
    caml->set_camera_mask(light_mask);
    dr = lightbuffer->make_display_region();
    dr->set_camera(lightcam);
    auto plaincam = window->make_camera();
    auto camp = DCAST(Camera, plaincam.node());
    camp->set_lens(lens);
    camp->set_scene(render);
    camp->set_camera_mask(plain_mask);
    dr = lightbuffer->make_display_region(); // or is it same as light?
    dr->set_camera(plaincam);

    // Panda's main camera is not used.

    cam->set_active(false);

    // Take explicit control over the order in which the three
    // buffers are rendered.

    modelbuffer->set_sort(1);
    lightbuffer->set_sort(2);
    win->set_sort(3);

    // Within the light buffer, control the order of the two cams.

    caml->get_display_region(0)->set_sort(1);
    camp->get_display_region(0)->set_sort(2);

    // By default, panda usually clears the screen before every
    // camera and before every window.  Tell it not to do that.
    // Then, tell it specifically when to clear and what to clear.

    camm->get_display_region(0)->disable_clears();
    caml->get_display_region(0)->disable_clears();
    camp->get_display_region(0)->disable_clears();
    window->get_display_region_3d()->disable_clears();
    window->get_render_2d();
    window->get_display_region_2d()->disable_clears();
    modelbuffer->disable_clears();
    win->disable_clears();

    modelbuffer->set_clear_color_active(true);
    modelbuffer->set_clear_depth_active(true);
    lightbuffer->set_clear_color_active(true);
    lightbuffer->set_clear_color(LColor(0, 0, 0, 1));

    // Miscellaneous stuff.

    //disable_mouse()
    auto camera = window->get_camera_group();
    camera.set_pos(-9.112, -211.077, 46.951);
    camera.set_hpr(0, -7.5, 2.4);

    // Calculate the projection parameters for the final shader.
    // The math here is too complex to explain in an inline comment,
    // I've put in a full explanation into the HTML intro.

    auto &proj = lens->get_projection_mat();
    auto proj_x = 0.5 * proj[3][2] / proj[0][0];
    auto proj_y = 0.5 * proj[3][2];
    auto proj_z = 0.5 * proj[3][2] / proj[2][1];
    auto proj_w = -0.5 - 0.5 * proj[1][2];

    // Configure the render state of the model camera.

    {
	NodePath tempnode(new PandaNode("temp node"));
	tempnode.set_attrib(
            AlphaTestAttrib::make(RenderAttrib::M_greater_equal, 0.5));
	tempnode.set_shader(def_load_shader("model.sha"));
	tempnode.set_attrib(DepthTestAttrib::make(RenderAttrib::M_less_equal));
	camm->set_initial_state(tempnode.get_state());
    }

    // Configure the render state of the light camera.

    {
	NodePath tempnode(new PandaNode("temp node"));
	tempnode.set_shader(def_load_shader("light.sha"));
	tempnode.set_shader_input("texnormal", tex_normal);
	tempnode.set_shader_input("texalbedo", tex_albedo);
	tempnode.set_shader_input("texdepth", tex_depth);
	tempnode.set_shader_input("proj", LVector4(proj_x, proj_y, proj_z, proj_w));
	tempnode.set_attrib(ColorBlendAttrib::make(ColorBlendAttrib::M_add,
            ColorBlendAttrib::O_one, ColorBlendAttrib::O_one));
	tempnode.set_attrib(
            CullFaceAttrib::make(CullFaceAttrib::M_cull_counter_clockwise));
	// The next line causes problems on Linux.
	// tempnode.setAttrib(DepthTestAttrib.make(RenderAttrib.MGreaterEqual))
	tempnode.set_attrib(DepthWriteAttrib::make(DepthWriteAttrib::M_off));
	caml->set_initial_state(tempnode.get_state());
    }

    // Configure the render state of the plain camera.

    auto rs = RenderState::make_empty();
    camp->set_initial_state(rs);

    // Clear any render attribs on the root node. This is necessary
    // because by default, panda assigns some attribs to the root
    // node.  These default attribs will override the
    // carefully-configured render attribs that we just attached
    // to the cameras.  The simplest solution is to just clear
    // them all out.

    render.set_state(RenderState::make_empty());

    // My artist created a model in which some of the polygons
    // don't have textures.  This confuses the shader I wrote.
    // This little hack guarantees that everything has a texture.

    auto white = def_load_texture("models/white.jpg");
    render.set_texture(white, 0);

    // Create two subroots, to help speed cull traversal.

    lightroot = NodePath(new PandaNode("lightroot"));
    lightroot.reparent_to(render);
    modelroot = NodePath(new PandaNode("modelroot"));
    modelroot.reparent_to(render);
    lightroot.hide(model_mask);
    modelroot.hide(light_mask);
    modelroot.hide(plain_mask);

    // Load the model of a forest.  Make it visible to the model camera.
    // This is a big model, so we load it asynchronously while showing a
    // load text.  We do this by creating a task for this, and emitting an
    // event upon completion.
    title = add_title("Loading models...");

    forest = NodePath(new PandaNode("Forest Root"));
    forest.reparent_to(render);
    forest.hide(light_mask | plain_mask);
    // The Python API just takes array arguments to load multiple files, and
    // a callback to indicate asynch loading, but everything has to be done
    // manually in C++.
    static const char * const model_files[] = {
	"models/background",
	"models/foliage01",
	"models/foliage02",
	"models/foliage03",
	"models/foliage04",
	"models/foliage05",
	"models/foliage06",
	"models/foliage07",
	"models/foliage08",
	"models/foliage09"
    };
    // This is done by spawning a ModelLoaderRequest (a type of AsyncTask)
    // for every file name in the above array.
    static PT(AsyncTask) models[sizeof(model_files)/sizeof(model_files[0])];
    PT(Loader) loader = new Loader; // alloc on stack causes ref integrity fail
    // Run these tasks in a separate thread (or more)
    static PT(AsyncTaskChain) mchn;
    mchn = framework.get_task_mgr().make_task_chain("loading");
    mchn->set_num_threads(1); // crashes quickly when higher
    loader->set_task_chain("loading");
    // when each task finishes, it generates the "done" event, if assigned.
    framework.get_event_handler().add_hook("load_done", [](const Event *ev,void *) {
	// The 1st arg to the "done" event is the task pointer.
	auto task = DCAST(ModelLoadRequest, ev->get_parameter(0).get_typed_ref_count_value());
	// do this here instead of in finish_loading() so locals can be used
	forest.attach_new_node(task->get_model());
	// I detect completion by the fact that I've added all models to forest
	if(forest.get_num_children() == sizeof(model_files)/sizeof(model_files[0]))
	    // so, when done, do the planned finish (other than reparenting
	    // to forest, which has been done above).
	    finish_loading();
    }, 0);
    // This is out-of-order because I wanted the event handler, above, to be
    // before the creation of tasks which generate the event.
    for(unsigned i = 0; i < sizeof(models)/sizeof(models[0]); i++) {
	// Just create and run async requests, generating "load_done" on completion
	models[i] = loader->make_async_request(sample_path + model_files[i]);
	models[i]->set_done_event("load_done");
	loader->load_async(models[i]);
    }
    // Cause the final results to be rendered into the main window on a
    // card.

    auto card = lightbuffer->get_texture_card();
    card.set_texture(tex_final);
    card.reparent_to(window->get_render_2d());

#if 0 /* Python only */
    // Panda contains a built-in viewer that lets you view the results of
    // your render-to-texture operations.  This code configures the viewer.

    buffer_viewer.set_position("llcorner");
    buffer_viewer.set_card_size(0, 0.40);
    buffer_viewer.set_layout("vline");
    toggle_cards();
    toggle_cards();
#endif

    spheremodel = window->load_model(framework.get_models(), "misc/sphere");

    // Create the firefly model, a fuzzy dot
    auto dot_size = 1.0;
    CardMaker cm("firefly");
    cm.set_frame(-dot_size, dot_size, -dot_size, dot_size);
    firefly = NodePath(cm.generate());
    firefly.set_texture(def_load_texture("models/firefly.png"));
    firefly.set_attrib(ColorBlendAttrib::make(ColorBlendAttrib::M_add,
        ColorBlendAttrib::O_incoming_alpha, ColorBlendAttrib::O_one));

    // these allow you to change parameters in realtime

    framework.define_key("arrow_up", "",   EV_FN() { inc_firefly_count(1.1111111); }, 0);
    framework.define_key("arrow_down", "", EV_FN() { dec_firefly_count(0.9000000); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { set_firefly_size(1.1111111); }, 0);
    framework.define_key("arrow_left", "",  EV_FN() { set_firefly_size(0.9000000); }, 0);
#if 0
    framework.define_key("v", "", EV_FN() { toggle_cards(); }, 0);
    framework.define_key("V", "", EV_FN() { toggle_cards(); }, 0);
#endif
}

void add_firefly(void);
void update_readout(void);
AsyncTask::DoneStatus spawn_task(GenericAsyncTask *, void *);

void finish_loading()
{
    // This function is used as callback to loader.loadModel, and called
    // when all of the models have finished loading.

    // Note that the call itself attaches them to forest.

    // Show the instructions.
    title->set_text("Panda3D: Tutorial - Fireflies using Deferred Shading");
    add_instructions(0.06, "ESC: Quit");
    inst2 = add_instructions(0.12, "Up/Down: More / Fewer Fireflies (Count: unknown)");
    inst3 = add_instructions(0.18, "Right/Left: Bigger / Smaller Fireflies (Radius: unknown)");
#if 0
    add_instructions(0.24, "V: View the render-to-texture results");
#endif

    set_firefly_size(25.0);
    while(fireflies.size() < 5)
	add_firefly();
    update_readout();

    framework.get_task_mgr().add(new GenericAsyncTask("spawner", spawn_task, 0));
}

GraphicsOutput *make_FBO(const char *name, int auxrgba)
{
    // This routine creates an offscreen buffer.  All the complicated
    // parameters are basically demanding capabilities from the offscreen
    // buffer - we demand that it be able to render to texture on every
    // bitplane, that it can support aux bitplanes, that it track
    // the size of the host window, that it can render to texture
    // cumulatively, and so forth.
    WindowProperties winprops;
    FrameBufferProperties props;
    props.set_rgb_color(true);
    props.set_rgba_bits(8, 8, 8, 8);
    props.set_depth_bits(1);
    props.set_aux_rgba(auxrgba);
    auto win = window->get_graphics_window();
    return framework.get_graphics_engine()->make_output(
            win->get_pipe(), name, -2,
	    props, winprops,
            GraphicsPipe::BF_size_track_host | GraphicsPipe::BF_can_bind_every |
	    GraphicsPipe::BF_rtt_cumulative | GraphicsPipe::BF_refuse_window,
            win->get_gsg(), win);
}

void add_firefly()
{
    auto pos1 = LPoint3(rands.random_real(100) - 50, rands.random_real(250) - 100, rands.random_real(90) - 10);
    auto dir = LVector3(rands.random_real(2) - 1, rands.random_real(2) - 1, rands.random_real(2) - 1);
    dir.normalize();
    auto pos2 = pos1 + (dir * 20);
    auto fly = lightroot.attach_new_node(new PandaNode("fly"));
    auto glow = fly.attach_new_node(new PandaNode("glow"));
    auto dot = fly.attach_new_node(new PandaNode("dot"));
    auto color_r = 1.0;
    auto color_g = rands.random_real(0.2) + 0.8;
    auto color_b = std::min(color_g, rands.random_real(0.5) + 0.5);
    fly.set_color(color_r, color_g, color_b, 1.0);
    fly.set_shader_input("lightcolor", LColor(color_r, color_g, color_b, 1.0));
#define anm(x) #x + std::to_string(fireflies.size())
    auto int1 = new NPAnim(fly, anm(int1), rands.random_real(5) + 7);
    int1->set_start_pos(pos1);
    int1->set_end_pos(pos2);
    auto int2 = new NPAnim(fly, anm(int2), rands.random_real(5) + 7);
    int1->set_start_pos(pos2);
    int1->set_end_pos(pos1);
    auto si1 = new NPAnim(fly, anm(si1), rands.random_real(0.7) + 0.8);
    si1->set_start_scale(LPoint3(0.2, 0.2, 0.2));
    si1->set_end_scale(LPoint3(0.2, 0.2, 0.2));
    auto si2 = new NPAnim(fly, anm(si2), rands.random_real(0.7) + 0.8); // weird: python had 1.5..0.8, a null range
    si2->set_start_scale(LPoint3(1.0, 1.0, 1.0));
    si2->set_end_scale(LPoint3(0.2, 0.2, 0.2));
    auto si3 = new NPAnim(fly, anm(si3), rands.random_real(1.0) + 1.0);
    si3->set_start_scale(LPoint3(0.2, 0.2, 0.2));
    si3->set_end_scale(LPoint3(1.0, 1.0, 1.0));
    auto siseq = new Sequence({si1, si2, si3});
    siseq->loop();
    siseq->set_t(rands.random_real(1000));
    auto seq = new Sequence({int1, int2});
    seq->loop();
    spheremodel.instance_to(glow);
    firefly.instance_to(dot);
    glow.set_scale(fireflysize * 1.1);
    glow.hide(model_mask | plain_mask);
    dot.hide(model_mask | light_mask);
    dot.set_color(color_r, color_g, color_b, 1.0);
    fireflies.push_back(fly);
    sequences.push_back(seq);
    glowspheres.push_back(glow);
    scaleseqs.push_back(siseq);
}

void update_readout()
{
    inst2->set_text(
        "Up/Down: More / Fewer Fireflies (Currently: " + std::to_string(fireflies.size()) + ")");
    inst3->set_text(
        "Right/Left: Bigger / Smaller Fireflies (Radius: " + std::to_string((int)fireflysize) + " ft)");
}

#if 0 // BufferViewer is Python-only
void toggle_cards()
{
    buffer_viewer.toggle_enable();
    // When the cards are not visible, I also disable the color clear.
    // This color-clear is actually not necessary, the depth-clear is
    // sufficient for the purposes of the algorithm.
    if (buffer_viewer.is_enabled())
	modelbuffer.set_clear_color_active(true);
    else
	modelbuffer.set_clear_color_active(false);
}
#endif

void inc_firefly_count(PN_stdfloat scale)
{
    unsigned n = fireflies.size() * scale + 1;
    while(n > fireflies.size())
	add_firefly();
    update_readout();
}

void dec_firefly_count(PN_stdfloat scale)
{
    unsigned n = fireflies.size() * scale;
    if(n < 1)
	n = 1;
    while(fireflies.size() > n) {
	glowspheres.pop_back();
	glowspheres.pop_back();
	sequences.back()->finish();
	sequences.pop_back();
	scaleseqs.back()->finish();
	scaleseqs.pop_back();
	fireflies.back().remove_node();
	fireflies.pop_back();
    }
    update_readout();
}

void set_firefly_size(PN_stdfloat n)
{
    n *= fireflysize;
    fireflysize = n;
    for(auto &x: glowspheres)
	x.set_scale(fireflysize * 1.1);
    update_readout();
}

AsyncTask::DoneStatus spawn_task(GenericAsyncTask *task, void *)
{
    if(task->get_elapsed_time() > nextadd) {
	nextadd = task->get_elapsed_time() + 1.0;
	if (fireflies.size() < 300)
	    inc_firefly_count(1.03);
    }
    return AsyncTask::DS_cont;
}
}

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
