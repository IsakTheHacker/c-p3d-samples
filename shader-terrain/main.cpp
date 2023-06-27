/*
 * Translation of Python shader-terrain sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/shader-terrain
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: tobspr
 *
 * Last Updated: 2016-04-30
 *
 * This tutorial provides an example of using the ShaderTerrainMesh class
 */
#include <pandaFramework.h>
#include <glgsg.h>
#include <shaderTerrainMesh.h>
#include <load_prc_file.h>
#include <texturePool.h>
#include <shaderPool.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;

void init(void)
{
    // Load some configuration variables, its important for this to happen
    // before the framework is initialized
    textures_power_2 = ATS_none;
    //gl_coordinate_system = CS_default; // doesn't link; set below
    window_title = "Panda3D ShaderTerrainMesh Demo";
    //filled_wireframe_apply_shader = true; // doesn't link; set below

    // As an optimization, set this to the maximum number of cameras
    // or lights that will be rendering the terrain at any given time.
    //stm_max_views = 8; // doesn't link; set below

    // Further optimize the performance by reducing this to the max
    // number of chunks that will be visible at any given time.
    //stm_max_chunk_count = 2048; // doesn't link; set below

    // The following can't be set directly; use PRC "file"
    load_prc_file_data("",
            "gl-coordinate-system default\n"
            "filled-wireframe-apply-shader true\n"
            "stm-max-views 8\n"
            "stm-max-chunk-count 2048\n");

    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();

    //Set the window title and open new window
    window = framework.open_window();

    // Increase camera FOV as well as the far plane
    auto camera = window->get_camera(0);
    auto cam_lens = camera->get_lens();
    cam_lens->set_fov(90);
    cam_lens->set_near_far(0.1, 50000);

    // Construct the terrain
    auto terrain_node = new ShaderTerrainMesh;

    // Set a heightfield, the heightfield should be a 16-bit png and
    // have a quadratic size of a power of two.
    auto heightfield = def_load_texture("heightfield.png");
    heightfield->set_wrap_u(SamplerState::WM_clamp);
    heightfield->set_wrap_v(SamplerState::WM_clamp);
    terrain_node->set_heightfield(heightfield);

    // Set the target triangle width. For a value of 10.0 for example,
    // the terrain will attempt to make every triangle 10 pixels wide on screen.
    terrain_node->set_target_triangle_width(10.0);

    // Generate the terrain
    terrain_node->generate();

    // Attach the terrain to the main scene and set its scale. With no scale
    // set, the terrain ranges from (0, 0, 0) to (1, 1, 1)
    auto terrain = window->get_render().attach_new_node(terrain_node);
    terrain.set_scale(1024, 1024, 100);
    terrain.set_pos(-512, -512, -70.0);

    // Set a shader on the terrain. The ShaderTerrainMesh only works with
    // an applied shader. You can use the shaders used here in your own application
    auto terrain_shader = def_load_shader2(Shader::SL_GLSL, "terrain.vert.glsl", "terrain.frag.glsl");
    terrain.set_shader(terrain_shader);
    terrain.set_shader_input("camera", window->get_camera_group());

    // need to manually enable mouse
    window->setup_trackball();

    // Shortcut to view the wireframe mesh
    window->enable_keyboard();
    framework.define_key("f3", "", framework.event_w, &framework);
    framework.define_key("escape", "", framework.event_esc, &framework);

    // Set some texture on the terrain
    auto grass_tex = def_load_texture("textures/grass.png");
    grass_tex->set_minfilter(SamplerState::FT_linear_mipmap_linear);
    grass_tex->set_anisotropic_degree(16);
    terrain.set_texture(grass_tex);

    // Load a skybox - you can safely ignore this code
    auto skybox = def_load_model("models/skybox.bam");
    skybox.reparent_to(window->get_render());
    skybox.set_scale(20000);

    auto skybox_texture = def_load_texture("textures/skybox.jpg");
    skybox_texture->set_minfilter(SamplerState::FT_linear);
    skybox_texture->set_magfilter(SamplerState::FT_linear);
    skybox_texture->set_wrap_u(SamplerState::WM_repeat);
    skybox_texture->set_wrap_v(SamplerState::WM_mirror);
    skybox_texture->set_anisotropic_degree(16);
    skybox.set_texture(skybox_texture);

    auto skybox_shader = def_load_shader2(Shader::SL_GLSL, "skybox.vert.glsl", "skybox.frag.glsl");
    skybox.set_shader(skybox_shader);
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
