///////////////////////////////////////////////////////
//                                                   //
// Original code copyright (c) 2019 Ian Eborn.       //
// http://thaumaturge-art.com                        //
// Ported to C++ 2023 by Thomas J. Moore.  All of my //
// work is in the Public Domain, except as required  //
// by compliance with the license on the original    //
// code, stated below.                               //
//                                                   //
// Licensed under the MIT license.                   //
// See "FinalGame/codeLicense".txt, or               //
// https://opensource.org/licenses/MIT               //
//                                                   //
///////////////////////////////////////////////////////

#include <panda3d/pandaFramework.h>
#include <panda3d/auto_bind.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/genericAsyncTask.h>

PandaFramework framework;
NodePath temp_actor;

enum { k_up, k_down, k_left, k_right, k_shoot };

AsyncTask::DoneStatus update(GenericAsyncTask *task, void *data);

int main(void)
{
    framework.open_framework();
    WindowProperties properties;
    framework.get_default_window_props(properties);
    properties.set_size(1000, 750);
    auto window = framework.open_window(properties, 0);

    auto render = window->get_render();
    auto main_light = new DirectionalLight("main light");
    auto main_light_node_path = render.attach_new_node(main_light);
    main_light_node_path.set_hpr(45, -45, 0);
    render.set_light(main_light_node_path);

    auto ambient_light = new AmbientLight("ambient light");
    ambient_light->set_color(LColor(0.2, 0.2, 0.2, 1));
    auto ambient_light_node_path = render.attach_new_node(ambient_light);
    render.set_light(ambient_light_node_path);

    render.set_shader_auto();

    auto environment = window->load_model(render, "Models/Misc/environment");

    temp_actor = window->load_model(render, "Models/PandaChan/act_p3d_chan");
    window->load_model(temp_actor, "Models/PandaChan/a_p3d_chan_run");
    AnimControlCollection anims;
    auto_bind(temp_actor.node(), anims, PartGroup::HMF_ok_anim_extra);
    auto walk = anims.get_anim(0);
    temp_actor.get_child(0).set_h(180);
    walk->loop(true);

    auto camera = window->get_camera_group();
    camera.set_pos(0, 0, 32);
    camera.set_p(-90);

    window->enable_keyboard();
    bool key_map[(int)k_shoot + 1] = {};
#define EVH(p) [](const Event *, void *p)
#define key_setter(k, v) EVH(kmap) { ((bool *)kmap)[k] = v; }, key_map
    auto &evhand = framework.get_event_handler();
    evhand.add_hook("w", key_setter(k_up, true));
    evhand.add_hook("w-up", key_setter(k_up, false));
    evhand.add_hook("s", key_setter(k_down, true));
    evhand.add_hook("s-up", key_setter(k_down, false));
    evhand.add_hook("a", key_setter(k_left, true));
    evhand.add_hook("a-up", key_setter(k_left, false));
    evhand.add_hook("d", key_setter(k_right, true));
    evhand.add_hook("d-up", key_setter(k_right, false));
    evhand.add_hook("mouse1", key_setter(k_shoot, true));
    evhand.add_hook("mouse1-up", key_setter(k_shoot, false));

    auto update_task = new GenericAsyncTask("update", update, key_map);
    framework.get_task_mgr().add(update_task);

    framework.main_loop();
    framework.close_framework();
    return 0;
}

AsyncTask::DoneStatus update(GenericAsyncTask *, void *data)
{
    auto dt = ClockObject::get_global_clock()->get_dt();

    auto key_map = reinterpret_cast<bool *>(data);
    if(key_map[k_up])
	temp_actor.set_pos(temp_actor.get_pos() + LVector3(0, 5.0*dt, 0));
    if(key_map[k_down])
	temp_actor.set_pos(temp_actor.get_pos() + LVector3(0, -5.0*dt, 0));
    if(key_map[k_left])
	temp_actor.set_pos(temp_actor.get_pos() + LVector3(-5.0*dt, 0, 0));
    if(key_map[k_right])
	temp_actor.set_pos(temp_actor.get_pos() + LVector3(5.0*dt, 0, 0));
    if(key_map[k_shoot])
	std::cout << "Zap!\n";

    return AsyncTask::DS_cont;
}
