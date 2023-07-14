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

#include "Game.h"
#include <panda3d/auto_bind.h>
#include <panda3d/animBundleNode.h>
#include <panda3d/collisionHandlerQueue.h>
#include <panda3d/collisionRay.h>
#include <panda3d/collisionSphere.h>

#define FRICTION 150.0

GameObject::GameObject(LPoint3 pos, const std::string &model_name,
		       std::initializer_list<std::string> model_anims,
		       PN_stdfloat _max_health, PN_stdfloat _max_speed,
		       const std::string &collider_name) :
    anims(), max_health(_max_health), health(_max_health), max_speed(_max_speed),
    velocity(0, 0, 0), acceleration(300.0), walking(false), collider()
{
    actor = window->load_model(window->get_render(), model_name);
    actor.set_pos(pos);
    int i = 0;
    for(auto anim_f: model_anims) {
	auto n = window->load_model(actor, anim_f);
	DCAST(AnimBundleNode, n.get_child(0).node())->get_bundle()->set_name(std::to_string(i));
	i++;
    }
    auto_bind(actor.node(), anims,
	      PartGroup::HMF_ok_anim_extra |
	      PartGroup::HMF_ok_wrong_root_name);
    stand_anim = anims.find_anim("0");
    walk_anim = anims.find_anim("1");

    auto collider_node = new CollisionNode(collider_name);
    collider_node->add_solid(new CollisionSphere(0, 0, 0, 0.3));
    collider = actor.attach_new_node(collider_node);
    collider.set_tag("owner", std::to_string((unsigned long)this));
}

void GameObject::update(double dt)
{
    auto speed = velocity.length();
    if(speed > max_speed) {
	velocity.normalize();
	velocity *= max_speed;
	speed = max_speed;
    }

    if(!walking) {
	auto friction_val = FRICTION*dt;
	if(friction_val > speed)
	    velocity.set(0, 0, 0);
	else {
	    auto friction_vec = -velocity;
	    friction_vec.normalize();
	    friction_vec *= friction_val;

	    velocity += friction_vec;
	}
    }

    actor.set_pos(actor.get_pos() + velocity*dt);
}

void GameObject::alter_health(PN_stdfloat d_health)
{
    health += d_health;

    if(health > max_health)
	health = max_health;
}

GameObject::~GameObject()
{
    if(!collider.is_empty()) {
	collider.clear_tag("owner");
	c_trav.remove_collider(collider);
	pusher->remove_collider(collider);
    }

    anims.clear_anims();
    if(!actor.is_empty()) {
	actor.remove_node();
	actor.clear();
    }

    collider.clear();
}

Player::Player() :
    GameObject(LPoint3(0, 0, 0),
	       "Models/PandaChan/act_p3d_chan",
	       { "Models/PandaChan/a_p3d_chan_idle", // stand
		 "Models/PandaChan/a_p3d_chan_run"   // walk
	       },
	       5,
	       10,
	       "player")
{
    actor.get_child(0).set_h(180);

    auto collider_node = DCAST(CollisionNode, collider.node());
    collider_node->set_into_collide_mask(1 << 1);
    collider_node->set_from_collide_mask(1 << 1);

    pusher->add_collider(collider, actor);
    c_trav.add_collider(collider, pusher);

    ray = new CollisionRay(0, 0, 0, 0, 1, 0);

    auto ray_node = new CollisionNode("playerRay");
    ray_node->add_solid(ray);

    ray_node->set_from_collide_mask(1 << 2);
    ray_node->set_into_collide_mask(0);

    ray_node_path = window->get_render().attach_new_node(ray_node);
    ray_queue = new CollisionHandlerQueue;

    c_trav.add_collider(ray_node_path, ray_queue);

    beam_model = window->load_model(actor, "Models/Misc/bambooLaser");
    beam_model.set_z(1.5);
    beam_model.set_light_off();
    beam_model.hide();

    damage_per_second = -5.0;

    stand_anim->loop(true);
}

void Player::update(bool key_map[], double dt)
{
    GameObject::update(dt);

    walking = false;

    if(key_map[k_up]) {
	walking = true;
	velocity.add_y(acceleration*dt);
    }
    if(key_map[k_down]) {
	walking = true;
	velocity.add_y(-acceleration*dt);
    }
    if(key_map[k_left]) {
	walking = true;
	velocity.add_x(-acceleration*dt);
    }
    if(key_map[k_right]) {
	walking = true;
	velocity.add_x(acceleration*dt);
    }

    if(walking) {
	if(stand_anim->is_playing())
	    stand_anim->stop();

	if(!walk_anim->is_playing())
	    walk_anim->loop(true);
    } else {
	if(!stand_anim->is_playing()) {
	    walk_anim->stop();
	    stand_anim->loop(true);
	}
    }

    if(key_map[k_shoot]) {
	if(ray_queue->get_num_entries() > 0) {
	    ray_queue->sort_entries();
	    auto ray_hit = ray_queue->get_entry(0);
	    auto hit_pos = ray_hit->get_surface_point(window->get_render());

	    auto hit_node_path = ray_hit->get_into_node_path();
	    auto owner = hit_node_path.get_tag("owner");
	    if(!owner.empty()) {
		auto hit_object = reinterpret_cast<GameObject *>(std::stoul(owner));
		if(!dynamic_cast<TrapEnemy *>(hit_object))
		    hit_object->alter_health(damage_per_second*dt);
	    }
	    auto beam_length = (hit_pos - actor.get_pos()).length();
	    beam_model.set_sy(beam_length);

	    beam_model.show();
        }
    } else
	beam_model.hide();
}

Player::~Player()
{
    c_trav.remove_collider(ray_node_path);
}

Enemy::Enemy(LPoint3 pos, const std::string &model_name,
	     std::initializer_list<std::string> model_anims,
	     PN_stdfloat max_health, PN_stdfloat max_speed,
	     const std::string &collider_name) :
    GameObject(pos, model_name, model_anims, max_health, max_speed, collider_name),
    score_value(1)
{
    attack_anim = anims.find_anim("2");
    die_anim = anims.find_anim("3");
    spawn_anim = anims.find_anim("4");
}

void Enemy::update(Player &player, double dt)
{
    GameObject::update(dt);

    run_logic(player, dt);

    if(walking) {
	if(!walk_anim->is_playing())
	    walk_anim->loop(true);
    } else
	if((!spawn_anim || !spawn_anim->is_playing()) &&
	   (!attack_anim || !attack_anim->is_playing()) &&
	   !stand_anim->is_playing())
	    stand_anim->loop(true);
}

WalkingEnemy::WalkingEnemy(LPoint3 pos) :
    Enemy(pos,
	  "Models/Misc/simpleEnemy",
          {
	      "Models/Misc/simpleEnemy-stand",  // stand
	      "Models/Misc/simpleEnemy-walk",   // walk
	      "Models/Misc/simpleEnemy-attack", // attack
	      "Models/Misc/simpleEnemy-die",    // die
	      "Models/Misc/simpleEnemy-spawn"   // spawn
	  },
	  3,
	  7,
	  "walkingEnemy"),
    attack_distance(0.75), y_vector(0, 1)
{
    acceleration = 100.0;

    DCAST(CollisionNode, collider.node())->set_into_collide_mask(1 << 2);
}

void WalkingEnemy::run_logic(Player &player, double dt)
{
    auto vector_to_player = player.get_actor().get_pos() - actor.get_pos();

    auto vector_to_player_2d = vector_to_player.get_xy();
    auto distance_to_player = vector_to_player_2d.length();

    vector_to_player_2d.normalize();

    auto heading = y_vector.signed_angle_deg(vector_to_player_2d);

    if(distance_to_player > attack_distance*0.9) {
	walking = true;
	vector_to_player.set_z(0);
	vector_to_player.normalize();
	velocity += vector_to_player*acceleration*dt;
    } else {
	walking = false;
	velocity.set(0, 0, 0);
    }

    actor.set_h(heading);
}

TrapEnemy::TrapEnemy(LPoint3 pos) :
    Enemy(pos,
	  "Models/Misc/trap",
          {
	      "Models/Misc/trap-stand", // stand
	      "Models/Misc/trap-walk"   // walk
	  },
	  100.0,
	  10.0,
	  "trapEnemy"), move_direction(0), ignore_player(false), move_in_x(false)
{
    auto collider_node = DCAST(CollisionNode, collider.node());
    collider_node->set_into_collide_mask((1 << 2) | (1 << 1));
    collider_node->set_from_collide_mask((1 << 2) | (1 << 1));

    pusher->add_collider(collider, actor);
    c_trav.add_collider(collider, pusher);
}

void TrapEnemy::run_logic(Player &player, double dt)
{
    if(move_direction != 0) {
	walking = true;
	if(move_in_x)
	    velocity.add_x(move_direction*acceleration*dt);
	else
	    velocity.add_y(move_direction*acceleration*dt);
    } else {
	walking = false;
	auto diff = player.get_actor().get_pos() - actor.get_pos();
	PN_stdfloat detector, movement;
	if(move_in_x) {
	    detector = diff[1];
	    movement = diff[0];
	} else {
	    detector = diff[0];
	    movement = diff[1];
	}

	if(cabs(detector) < 0.5)
	    move_direction = movement < 0 ? -1 : movement > 0 ? 1 : 0;
    }
}
