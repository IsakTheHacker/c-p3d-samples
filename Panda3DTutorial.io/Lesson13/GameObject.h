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

#pragma once
#include <panda3d/animControlCollection.h>
#include <initializer_list>

class CollisionHandlerQueue;
class CollisionSegment;
class CollisionRay;
class TextNode;

class GameObject {
  public:
    GameObject(LPoint3 pos, const std::string &model_name,
	       std::initializer_list<std::string> model_anims,
	       PN_stdfloat max_health, PN_stdfloat max_speed,
	       const std::string &collider_name);
    virtual ~GameObject();
    void update(double dt);
    virtual void alter_health(PN_stdfloat d_health);
    const NodePath &get_actor() { return actor; }
    PN_stdfloat get_health() { return health; }
    void disable_collider() { collider.remove_node(); };
  protected:
    NodePath actor;
    AnimControlCollection anims;
    AnimControl *stand_anim, *walk_anim;
    PN_stdfloat max_health, health, max_speed;
    LVector3 velocity;
    PN_stdfloat acceleration;
    bool walking;
    NodePath collider;
};

class Player : public GameObject {
  public:
    Player();
    ~Player();
    void update(bool key_map[], double dt);
    void update_score();
    void alter_health(PN_stdfloat d_health);
    void add_score(int adj) { score += adj; }
  protected:
    void update_health_ui();
    PT(CollisionRay) ray;
    PT(CollisionHandlerQueue) ray_queue;
    NodePath ray_node_path;
    NodePath beam_model, beam_hit_model;
    PN_stdfloat damage_per_second;
    PN_stdfloat beam_hit_pulse_rate, beam_hit_timer;
    int score;
    NodePath score_ui;
    TextNode *score_ui_text;
    std::vector<NodePath> health_icons;
    NodePath damage_taken_model, beam_hit_light_node_path;
    PN_stdfloat damage_taken_model_timer, damage_taken_model_duration;
    LPoint2 last_mouse_pos;
    LPlane ground_plane;
    LVector2 y_vector;
};

class Enemy : public GameObject {
  public:
    void update(Player &player, double dt);
    int get_score_value() { return score_value; }
    AnimControl *die_anim;
  protected:
    Enemy(LPoint3 pos, const std::string &model_name,
	  std::initializer_list<std::string> model_anims,
	  PN_stdfloat max_health, PN_stdfloat max_speed,
	  const std::string &collider_name);
    virtual void run_logic(Player &player, double dt) = 0;
    int score_value;
    AnimControl *attack_anim, *spawn_anim;
};

class WalkingEnemy : public Enemy {
  public:
    WalkingEnemy(LPoint3 pos);
    ~WalkingEnemy();
    void alter_health(PN_stdfloat d_health);
  protected:
    void update_health_visual();
    void run_logic(Player &player, double dt);
    PN_stdfloat attack_distance;
    int attack_damage;
    LVector2 y_vector;
    PN_stdfloat attack_delay, attack_delay_timer, attack_wait_timer;
    CollisionSegment *attack_segment;
    NodePath attack_segment_node_path;
    PT(CollisionHandlerQueue) segment_queue;
};

class TrapEnemy : public Enemy {
  public:
    TrapEnemy(LPoint3 pos);
    int move_direction;
    bool ignore_player;
  protected:
    void run_logic(Player &player, double dt);
    bool move_in_x;
};
