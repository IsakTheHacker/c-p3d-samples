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
typedef std::shared_ptr<Enemy> EnemyPtr;
std::vector<EnemyPtr> enemies;
std::vector<EnemyPtr> trap_enemies;
std::vector<EnemyPtr> dead_enemies;
std::vector<LPoint3> spawn_points;

PN_stdfloat initial_spawn_interval, minimum_spawn_interval, spawn_interval;
PN_stdfloat spawn_timer;
unsigned max_enemies, maximum_max_enemies, num_traps_per_side;

PN_stdfloat difficulty_interval, difficulty_timer;

AsyncTask::DoneStatus update(GenericAsyncTask *task, void *data);
void stop_trap(const Event *ev, void *data),
     trap_hits_something(const Event *ev, void *data);

void start_game(void);

int main(void)
{
    srand48(time(0));
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

    unsigned num_points_per_wall = 5;
    for(unsigned i = 0; i < num_points_per_wall; i++) {
	auto coord = 7.0/num_points_per_wall + 0.5;
	spawn_points.push_back(LPoint3(-7.0, coord, 0));
	spawn_points.push_back(LPoint3(7.0, coord, 0));
	spawn_points.push_back(LPoint3(coord, -7.0, 0));
	spawn_points.push_back(LPoint3(coord, 7.0, 0));
    }

    initial_spawn_interval = 1.0;
    minimum_spawn_interval = 0.2;
    spawn_interval = initial_spawn_interval;
    spawn_timer = spawn_interval;
    max_enemies = 2;
    maximum_max_enemies = 20;

    num_traps_per_side = 2;

    difficulty_interval = 5.0;
    difficulty_timer = difficulty_interval;

    start_game();

    framework.main_loop();
    framework.close_framework();
    return 0;
}

void cleanup(void);

void start_game()
{
    cleanup();

    player = std::make_unique<Player>();

    max_enemies = 2;

    spawn_interval = initial_spawn_interval;

    difficulty_timer = difficulty_interval;

    std::vector<PN_stdfloat> side_trap_slots[4];
    auto trap_slot_distance = 0.4;
    auto slot_pos = -8 + trap_slot_distance;
    while(slot_pos < 8) {
	if(cabs(slot_pos) > 1.0) {
	    side_trap_slots[0].push_back(slot_pos);
	    side_trap_slots[1].push_back(slot_pos);
	    side_trap_slots[2].push_back(slot_pos);
	    side_trap_slots[3].push_back(slot_pos);
	}
	slot_pos += trap_slot_distance;
    }

    for(unsigned i = 0; i < num_traps_per_side; i++) {
	for(unsigned j = 0; j < 4; j++) {
	    unsigned slotno = floor(drand48() * side_trap_slots[j].size());
	    auto slot = side_trap_slots[j][slotno];
	    side_trap_slots[j].erase(side_trap_slots[j].begin() + slotno);
	    auto other = (j % 2) ? -7.0 : 7.0;
	    LPoint3 pos(j/2 ? other : slot, j/2 ? slot : other, 0.0);
	    trap_enemies.push_back(std::make_unique<TrapEnemy>(pos));
	}
    }
}

void spawn_enemy()
{
    if(enemies.size() < max_enemies) {
	auto spawn_point = spawn_points[drand48()*spawn_points.size()];

	enemies.push_back(std::make_shared<WalkingEnemy>(spawn_point));
    }
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
    if(!window->get_graphics_window())
	return AsyncTask::DS_done;

    c_trav.traverse(window->get_render());

    auto dt = ClockObject::get_global_clock()->get_dt();

    if(player != nullptr && player->get_health() > 0) {
	auto key_map = reinterpret_cast<bool *>(data);
	player->update(key_map, dt);

	spawn_timer -= dt;
	if(spawn_timer <= 0) {
	    spawn_timer = spawn_interval;
	    spawn_enemy();
	}

	for(auto &enemy: enemies)
	    enemy->update(*player, dt);
	for(auto &trap: trap_enemies)
	    trap->update(*player, dt);

	std::vector<std::shared_ptr<Enemy>> newly_dead_enemies;
	for(auto &enemy: enemies)
	    if(enemy->get_health() <= 0)
		newly_dead_enemies.push_back(enemy);
	// this is improvied in C++-20, but I'm sticking to 11.
	// std::erase_if(enemies, [] { ... });
	enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
				     [](EnemyPtr enemy) {
					 return enemy->get_health() <= 0;
				     }), enemies.end());

	for(auto enemy : newly_dead_enemies) {
	    enemy->disable_collider();
	    enemy->die_anim->play();
	    player->add_score(enemy->get_score_value());
	}
	if(newly_dead_enemies.size() > 0)
	    player->update_score();

	dead_enemies.insert(dead_enemies.end(), newly_dead_enemies.begin(),
			    newly_dead_enemies.end());

	// this is improvied in C++-20, but I'm sticking to 11.
	// std::erase_if(dead_enemies, [] { ... });
	dead_enemies.erase(std::remove_if(dead_enemies.begin(),
					  dead_enemies.end(), [](EnemyPtr enemy) {
					      auto anim = enemy->die_anim;
					      return !anim || !anim->is_playing();
					  }), dead_enemies.end());

	difficulty_timer -= dt;
	if(difficulty_timer <= 0) {
	    difficulty_timer = difficulty_interval;
	    if(max_enemies < maximum_max_enemies)
		max_enemies++;
	    if(spawn_interval > minimum_spawn_interval)
		spawn_interval -= 0.1;
	}
    }

    return AsyncTask::DS_cont;
}

void cleanup()
{
    // clean up previous run
    enemies.clear();
    dead_enemies.clear();
    trap_enemies.clear();
    player = nullptr;
}
