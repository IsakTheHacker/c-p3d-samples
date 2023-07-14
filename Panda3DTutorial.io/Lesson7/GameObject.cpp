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
#include <panda3d/collisionSphere.h>

#define FRICTION 150.0

GameObject::GameObject(LPoint3 pos, const std::string &model_name,
		       std::initializer_list<std::string> model_anims, int _max_health,
		       int _max_speed, const std::string &collider_name) :
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

void GameObject::alter_health(int d_health)
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

    pusher->add_collider(collider, actor);
    c_trav.add_collider(collider, pusher);

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
}

Enemy::Enemy(LPoint3 pos, const std::string &model_name,
	     std::initializer_list<std::string> model_anims, int max_health,
	     int max_speed, const std::string &collider_name) :
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
