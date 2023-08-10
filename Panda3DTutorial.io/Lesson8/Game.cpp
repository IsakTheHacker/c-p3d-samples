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
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include <panda3d/genericAsyncTask.h>
#include <panda3d/collisionSphere.h>
#include <panda3d/collisionTube.h>

#include "Game.h"

PandaFramework framework;
WindowFramework *window;
CollisionTraverser c_trav;
PT(CollisionHandlerPusher) pusher;
std::unique_ptr<Player> player;
std::unique_ptr<WalkingEnemy> temp_enemy;
std::unique_ptr<TrapEnemy> temp_trap;

AsyncTask::DoneStatus update(GenericAsyncTask *task, void *data);
void stop_trap(const Event *ev, void *data),
     trap_hits_something(const Event *ev, void *data);

int main(void)
{
    framework.open_framework();
    WindowProperties properties;
    framework.get_default_window_props(properties);
    properties.set_size(1000, 750);
    window = framework.open_window(properties, 0);

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

    pusher = new CollisionHandlerPusher;

    pusher->set_horizontal(true);

    pusher->add_in_pattern("%fn-into-%in");
    evhand.add_hook("trapEnemy-into-wall", stop_trap, 0);
    evhand.add_hook("trapEnemy-into-trapEnemy", stop_trap, 0);
    evhand.add_hook("trapEnemy-into-player", trap_hits_something, 0);
    evhand.add_hook("trapEnemy-into-walkingEnemy", trap_hits_something, 0);

    auto wall_solid = new CollisionTube(-8.0, 0, 0, 8.0, 0, 0, 0.2);
    auto wall_node = new CollisionNode("wall");
    wall_node->add_solid(wall_solid);
    auto wall = render.attach_new_node(wall_node);
    wall.set_y(8.0);

    wall_solid = new CollisionTube(-8.0, 0, 0, 8.0, 0, 0, 0.2);
    wall_node = new CollisionNode("wall");
    wall_node->add_solid(wall_solid);
    wall = render.attach_new_node(wall_node);
    wall.set_y(-8.0);

    wall_solid = new CollisionTube(0, -8.0, 0, 0, 8.0, 0, 0.2);
    wall_node = new CollisionNode("wall");
    wall_node->add_solid(wall_solid);
    wall = render.attach_new_node(wall_node);
    wall.set_x(8.0);

    wall_solid = new CollisionTube(0, -8.0, 0, 0, 8.0, 0, 0.2);
    wall_node = new CollisionNode("wall");
    wall_node->add_solid(wall_solid);
    wall = render.attach_new_node(wall_node);
    wall.set_x(-8.0);

    auto update_task = new GenericAsyncTask("update", update, key_map);
    framework.get_task_mgr().add(update_task);

    player = std::make_unique<Player>();

    temp_enemy = std::make_unique<WalkingEnemy>(LPoint3(5, 0, 0));
    temp_trap = std::make_unique<TrapEnemy>(LPoint3(-2, 7, 0));

    framework.main_loop();
    framework.close_framework();
    return 0;
}

void stop_trap(const Event *ev, void *)
{
    auto entry = DCAST(CollisionEntry, ev->get_parameter(0).get_ptr());
    auto collider = entry->get_from_node_path();
    auto owner = collider.get_tag("owner");
    if(!owner.empty()) {
	auto trap = reinterpret_cast<TrapEnemy *>(std::stoul(owner));
	trap->move_direction = 0;
	trap->ignore_player = false;
    }
}

void trap_hits_something(const Event *ev, void *)
{
    auto entry = DCAST(CollisionEntry, ev->get_parameter(0).get_ptr());
    auto collider = entry->get_from_node_path();
    auto owner = collider.get_tag("owner");
    if(!owner.empty()) {
	auto trap = reinterpret_cast<TrapEnemy *>(std::stoul(owner));
	if(trap->move_direction == 0)
	    return;

	collider = entry->get_into_node_path();
	owner = collider.get_tag("owner");
	if(!owner.empty()) {
	    auto obj = reinterpret_cast<GameObject *>(std::stoul(owner));
	    auto player = dynamic_cast<Player *>(obj);
	    if(player) {
		if(!trap->ignore_player) {
		    obj->alter_health(-1);
		    trap->ignore_player = true;
		}
	    } else
		obj->alter_health(-10);
	}
    }
}

AsyncTask::DoneStatus update(GenericAsyncTask *, void *data)
{
    c_trav.traverse(window->get_render());

    auto dt = ClockObject::get_global_clock()->get_dt();

    auto key_map = reinterpret_cast<bool *>(data);
    player->update(key_map, dt);

    temp_enemy->update(*player, dt);
    temp_trap->update(*player, dt);

    return AsyncTask::DS_cont;
}
