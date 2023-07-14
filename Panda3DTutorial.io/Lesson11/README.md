---
layout: page
title: Lesson 11 (C++ Addendum)
---
The following expects you to first read:

[Lesson 11](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson11.html)

To start with, the attack itself. This will be fundamentally similar
to the player's laser-attack, but instead of using an
indefinitely-long ray we will use a limited-length (in fact, quite
short) line-segment, thus creating a "melee attack".

```c++
// Below the includes in GameObject.h:
class CollisionSegment;
```
```c++
// In the WalkingEnemy class definition in GameObject.h:

// How much damage the enemy's attack does
// That is, this results in the player-character's
// health being reduced by one.
int attack_damage;
CollisionSegment *attack_segment;
NodePath attack_segment_node_path;
PT(CollisionHandlerQueue) segment_queue;
```
```c++
// In your include group (back to GameObject.cpp):
#include <panda3d/collisionSegment.h>
```

```c++
// In WalkingEnemy::WalkingEnemy:
attack_segment = new CollisionSegment(0, 0, 0, 1, 0, 0);

auto segment_node = new CollisionNode("enemyAttackSegment");
segment_node->add_solid(attack_segment);

// A mask that matches the player's, so that
// the enemy's attack will hit the player-character,
// but not the enemy-character (or other enemies)
segment_node->set_from_collide_mask(1<<1);
segment_node->set_into_collide_mask(0);

attack_segment_node_path = window->get_render().attach_new_node(segment_node);
segment_queue = new CollisionHandlerQueue;

c_trav.add_collider(attack_segment_node_path, segment_queue);
```

Also, add `attack_damage(-1)` to WalkingEnemy's construtor initializers.

Where the player-character has the attack be controlled by the
player's input, the Walking Enemy will control its attack via its
"run_logic" method.

```c++
// In the WalkingEnemy class definition in GameObject.h:
// The delay between the start of an attack,
// and the attack (potentially) landing
PN_stdfloat attack_delay, attack_delay_timer,
   // How long to wait between attacks
   attack_wait_timer;
```

```c++
// In "WalkingEnemy::WalkingEnemy"'s constructor initializers:
attackDelay(0.3), attack_delay_timer(0), attack_wait_timer(0)
```

```c++
// In the "run_logic" method of "WalkingEnemy":

// Set the segment's start- and end- points.
// "getQuat" returns a quaternion--a representation
// of orientation or rotation--that represents the
// NodePath's orientation. This is useful here,
// because Panda's quaternion class has methods to get
// forward, right, and up vectors for that orientation.
// Thus, what we're doing is making the segment point "forwards".
attack_segment->set_point_a(actor.get_pos());
attack_segment->set_point_b(actor.get_pos() + actor.get_quat().get_forward()*attack_distance);
```

Previously, we had the following in the "run_logic" method of "WalkingEnemy":

```c++
if(distance_to_player > attack_distance*0.9) {
    walking = true;
    vector_to_player.set_z(0);
    vector_to_player.normalize();
    velocity += vector_to_player*acceleration*dt;
} else {
    walking = false;
    velocity.set(0, 0, 0);
}
```

We'll now add to this a bit:

```c++
if(distance_to_player > attack_distance*0.9) {
    walking = true;
    vector_to_player.set_z(0);
    vector_to_player.normalize();
    velocity += vector_to_player*acceleration*dt;
    if(!attack_anim->is_playing()) {
        walking = true;
        vector_to_player.set_z(0);
        vector_to_player.normalize();
        velocity += vector_to_player*acceleration*dt;
        attack_wait_timer = 0.2;
        attack_delay_timer = 0;
    }
} else {
    walking = false;
    velocity.set(0, 0, 0);

    // If we're waiting for an attack to land...
    if(attack_delay_timer > 0) {
        attack_delay_timer -= dt;
        // If the time has come for the attack to land...
        if(attack_delay_timer <= 0) {
            // Check for a hit..
            if(segment_queue->get_num_entries() > 0) {
                segment_queue->sort_entries();
                auto segment_hit = segment_queue->get_entry(0);

                auto hit_node_path = segment_hit->get_into_node_path();
                auto owner = hit_node_path.get_tag("owner");
                if(!owner.empty()) {
                     // Apply damage!
                     auto hit_object = reinterpret_cast<GameObject *>(std::stoul(owner));
                     hit_object->alter_health(attack_damage);
                     attack_wait_timer = 1.0;
                }
            }
        }
    // If we're instead waiting to be allowed to attack...
    } else if(attack_wait_timer > 0) {
        attack_wait_timer -= dt;
        // If the wait has ended...
        if(attack_wait_timer <= 0) {
            // Start an attack!
            // (And set the wait-timer to a random amount,
            //  to vary things a little bit.)
            attack_wait_timer = drand48()*0.2+0.5;
            attack_delay_timer = attack_delay;
            attack_anim->play();
        }
    }
}
```

Note the use of `drand48` for random numbers.  Other options are
available, including Panda3D's own Randomizer class, but drand48 is
always reliable.  It needs to be seeded, though, for actual random
behavior between runs.

```c++
// in main (Game.cpp):
srand48(time(0));
```

And finally, as with the player-character, there's a bit of cleanup to do:

```c++
// In the WalkingEnemy class definition in GameObject.h:
~WalkingEnemy();
```
```c++
// In tGameObject.cpp:
WalkingEnemy::~WalkingEnemy()
{
    c_trav.remove_collider(attack_segment_node_path);
    attack_segment_node_path.remove_node();
}
```

And that's it!

This is all very well and good, but right now all those hits and
collisions don't have much effect. To start with, let's add some
visual feedback: beam-impacts, hit-flashes, and a bit of UI...

Proceed to Lesson12 to continue.
