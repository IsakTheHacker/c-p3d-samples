---
layout: page
title: Lesson 7 (C++ Addendum)
---
The following expects you to first read:

[Lesson 7](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson07.html)

Naturally, substitute all references to `GameObject.py` to
`GameObject.cpp`.  Much as I assumed you substituted any references to
`Game.py` to `Game.cpp` and "Game object" with "main".  Since we're
adding another C++ file, we'll have to add it to the build
instructions (`CMakeLists.txt`):

```cmake
add_executable(${PROJECT_NAME} Game.cpp GameObject.cpp)
```

One of the primary differences between C++ and Python is the lack of
need for declaration, so naturally there is little to no separation
between API and implementation.  For the C++ version of the code,
there is separation.  The data structures are defined in header files.
The global variables from the global "object" are defined in `Game.h`
and the stuctures defined in `Gameobject.cpp` are defined in
`GameObject.h`.

Since the data structures must be defined in their entirety at the
point of declaration, it is not possible to just add a member to a
structure by referring to `this`.  Types must also be fixated at
delcaration time.

There is also a matter of member hiding, which I have done a little of
here, but sometimes rather than adding a setter/getter it's easier to
just make a member public.  As the original author states, this is not
a tutorial on game design or object-oriented programming.  It's a
tutorial on how to use Panda3D.  I will refrain on further comments on
how I think things should be structured.

One thing that needs immediate addressing is whether or not any new
classes we create are integrated into the Panda3D type system.  Doing
so makes some things easier, but adds a lot of boiler plate I'd rather
not explain in a basic tutorial.  As such, these will be simple,
straightforward C++ classes.

The shells of our new classes look like this (in `GameObject.h`):

```c++
class GameObject {
};
class Player : public GameObject {
};
class Enemy : public GameObject {
};
class WalkingEnemy : public Enemy { // public enemy #1!
};
```

In a number of places below, we'll want to access `c_trav` and our
`pusher` handler.  As already mentioned, this is done via global
variables.  Here are the global variables, declared in `Game.h` now.
This requires no change to `Global.cpp`'s actual variable definitions,
but it does require (for improved link-time data validation) including
this header there as well.  Since both C++ files need to include both
header files, the main header also includes `GameObject.h`.

```c++
#include <panda3d/windowFramework.h>
#include <panda3d/collisionHandlerPusher.h>

#include "GameObject.h"

// externs declared in main
extern WindowFramework *window;
extern Loader *loader;
extern CollisionTraverser c_trav;
extern PT(CollisionHandlerPusher) pusher;

enum { k_up, k_down, k_left, k_right, k_shoot };
```

Note the use of PT in declaring the pusher variable.  Most Panda3D
types are derived from ReferenceCount, which contains, as the name
suggests, a reference count.  Some users increase the count, and some
decrease it.  If a user decreases the count to zero, the object is
automatically deleted.  Most of the time, you don't have to think
about this, because you'll use it in a way that implicitly increases
the reference count until it's no longer needed.  Sometimes, though,
you'll call a function which increases, and then decreases the
reference count, which may cause the object to be deleted before
you're ready.  In these cases, the PT macro and its associated
PointerTo class are handy.  When a pointer is assigned to them, the
reference count is increased.  When they are themselves deleted, or
assigned a different pointer, the reference count is decreased,
possibly freeing the object.  Whether or not PT is used depends on
your personal habits (it's generally OK to use it everywhere a
ReferenceCount descendent is used) as well as usage.  In this case, I
feel that pusher will be freed too early if I don't add a reference
count that lasts the lifetime of the program.  A simple tip: if
you didn't use it, and your program crashes, it probably needed it.
Valgrind is your friend.

"GameObject" will store a given character's actor and collider, as
well as handling its velocity and movement, and the basics of its
health. It will also provide a destructor, for when we want to remove
a character.

From now on, we'll be supporting multiple animations, each loaded from
a different file.  This means that the preivous way of loading and
binding animations need to be rethought.  First of all, the
constructor for GameObject takes an array (actually initializer list)
of file names to load.  Note that this does not include the associated
name.  Instead, it assumed that animation file 0 is index 0.
Unfortunately, `auto_bind` doesn't have a reliable bind order, so it
requires names, at least temporarily, in order to use the auto-binding
results in the correct order.  Rather than now requiring a name to be
passed in, the loader simply assingns the array index as the animation
control name.  There is no problem if there is a name conflict between
models; each has its own AnimControlCollection.  The specific classes,
with their animation lists, must then bind the named animation to a
class member variable with a more appropriate name.  See Lesson 2 for
the method used to change names; the DCAST documentation is again
deferred to Lesson 8.

```c++
// GameObject.h
#include <panda3d/nodePath.h> // needs includes for any fully used structure
#include <initializer_list>   // for convenient multi-animation support

class AnimControl; // only pointers here, so no include

class GameObject {
  public:
    GameObject(LPoint3 pos, const std::string &model_name,
               std::initializer_list<std::string> model_anims, int max_health,
               int max_speed, const std::string &collider_name);
    virtual ~GameObject();
    void update(double dt);
    void alter_health(int d_health);
    const NodePath &get_actor() { return actor; }
  protected:
    NodePath actor;
    AnimControlCollection anims;
    AnimControl *stand_anim, *walk_anim;
    int max_health, health, max_speed;
    LVector3 velocity;
    PN_stdfloat acceleration;
    bool walking;
    NodePath collider;
};
```

```c++
// GameObject.cpp
#include "Game.h"
#include <panda3d/character.h>
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
    stand_anim = anims.find_anim("0"); // 1st anim must always be "stand"
    walk_anim = anims.find_anim("1");  // 2nd anim must always be "walk"

    // Note the "colliderName"--this will be used for
    // collision-events, later...
    auto collider_node = new CollisionNode(collider_name);
    collider_node->add_solid(new CollisionSphere(0, 0, 0, 0.3));
    collider = actor.attach_new_node(collider_node);
    // See below for an explanation of this!
    collider.set_tag("owner", std::to_string((unsigned long)this));
}

void GameObject::update(double dt)
{
    // If we're going faster than our maximum speed,
    // set the velocity-vector's length to that maximum
    auto speed = velocity.length();
    if(speed > max_speed) {
        velocity.normalize();
        velocity *= max_speed;
        speed = max_speed;
    }

    // If we're walking, don't worry about friction.
    // Otherwise, use friction to slow us down.
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

    // Move the character, using our velocity and
    // the time since the last update.
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
    // Remove various nodes; expect the reset to auto-delete
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
```

Note that the destructor above does less than the Python code's
equivalent cleanup method.  The reason is that once certain structures
are cleared or deallocated, everything related should clear or
deallocate as well.

The above statement applies to tags as well, so the only tag-related
line is:

```c++
collider.set_tag("owner", std::to_string((unsigned long)this));
```

This is a back pointer to the game object.  The C++ interface's tags
only store strings, so the game object's pointer (`this`) is converted
to a decimal string first.  Converting it back to a GameObject pointer
is a matter for the next lesson.  This does not constitute a reference
to the object, so there is no need for special cleanup.  If there
were, I'd use a weak reference, anyway, and there still wouldn't be a
need for special cleanup. The lifetime of the collider is less than
the lifetime of `this`, so there is also no need to worry about the
pointer becoming invalid.

The "Player" class holds pretty much the same player-logic as we've
thus far had in our mainline, save that the movement-controls now
alter its velocity, rather than just moving it:

```c++
// GameObject.h
class Player : public GameObject {
  public:
    Player();
    void update(bool key_map[], double dt);
};
```

```c++
// GameObject.cpp
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
    // Panda-chan faces "backwards", so we just turn
    // the first sub-node of our Actor-NodePath
    // to have it face as we want.
    actor.get_child(0).set_h(180);

    // Yep.  Global variables.
    pusher->add_collider(collider, actor);
    c_trav.add_collider(collider, pusher);

    stand_anim->loop(true);
}

void Player::update(bool key_map[], double dt)
{
    GameObject::update(dt);

    walking = false;

    // If we're  pushing a movement key, add a relevant amount
    // to our velocity.
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

    // Run the appropriate animation for our current state.
    // See original tutorial for an explanation.
    // Note however that there is no C++ FSM class in Panda3D.
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
```

The "Enemy" class provides a stub "run_logic" method, which is intended
to be overridden by its sub-classes, and calls this method in its
"update" method. It also provides a "score" value, for when it's
killed:

```c++
// GameObject.h
class Enemy : public GameObject {
  public:
    void update(Player &player, double dt);
  protected:
    Enemy(LPoint3 pos, const std::string &model_name,
          std::initializer_list<std::string> model_anims, int max_health,
          int max_speed, const std::string &collider_name);
    virtual void run_logic(Player &player, double dt) = 0;
    // This is the number of points to award
    // if the enemy is killed.
    int score_value;
};
```

```c++
// GameObject.cpp
Enemy::Enemy(LPoint3 pos, const std::string &model_name,
             std::initializer_list<std::string> model_anims, int max_health,
             int max_speed, const std::string &collider_name) :
    GameObject(pos, model_name, model_anims, max_health, max_speed, collider_name),
    score_value(1)
{
    // Finish binding anims, if present
    attack_anim = anims.find_anim("2"); // attack must be anim #3
    die_anim = anims.find_anim("3");    // die must be anim #4
    spawn_anim = anims.find_anim("4");  // spawn must be anim #5
}

void Enemy::update(Player &player, double dt)
{
    // In short, update as a GameObject, then
    // run whatever enemy-specific logic is to be done.
    // The use of a separate "runLogic" method
    // allows us to customise that specific logic
    // to the enemy, without re-writing the rest.

    GameObject::update(dt);

    run_logic(player, dt);

    // As with the player, play the appropriate animation.
    if(walking) {
        if(!walking_anim->is_playing())
            walking_anim->loop(true);
    } else
        if((!spawn_anim || !spawn_anim->is_playing()) &&
           (!attack_anim || !attack_anim->is_playing()) &&
           !stand_anim->is_playing())
            stand_anim->loop(true);
}
```

And finally, the "WalkingEnemy" class overrides the "run_logic" method
of the "Enemy" class, providing code that has it walk towards the
player until it reaches an "attack distance". It also turns to face
the player, using the vector between their positions, and the
"signed_angle_deg" (i.e. "get the signed angle, in degrees") method
provided by Panda's vector-classes.

```c++
// GameObject.h
class WalkingEnemy : public Enemy {
  public:
    WalkingEnemy(LPoint3 pos);
  protected:
    void run_logic(Player &player, double dt);
    PN_stdfloat attack_distance;
    LVector2 y_vector;
};
```

```c++
// GameObject.cpp
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
    attack_distance(0.75),
    // A reference vector, used to determine
    // which way to face the Actor.
    // Since the character faces along
    // the y-direction, we use the y-axis.
    y_vector(0, 1)
{
    acceleration = 100.0;
}

void WalkingEnemy::run_logic(Player &player, double dt)
{
    // In short: find the vector between
    // this enemy and the player.
    // If the enemy is far from the player,
    // use that vector to move towards the player.
    // Otherwise, just stop for now.
    // Finally, face the player.

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
```

With all that done, we want to use these classes in our game.

First we remove the player-code that we had in the mainline:
"temp_actor", its collider, the logic that adds that collider to
"pusher" and "c_trav", and the code that checks the key-map to move
"temp_actor". Note that "GameObject", above, now handles that actor and
collider logic, and "Player" handles the key-map checking.

Then we--for now--create a temporary instance of each of the "Player"
and "WalkingEnemy" classes. This isn't how we'll handle them in the
end, but it will serve for testing as we build up the classes.

Note that we want to allocate the objects, and have them
auto-deallocate.  Deriving from Panda3D's ReferenceCount was already
rejected, so we'll be using std::unique_ptr`, one of the standard ways
to do reference counting.  Unfortunately, its usage is far less
convenient and far more verbose than PT.  Given other changes in
C++11, I expect that was intentional.

So, in "Game.cpp":

```c++
// In your include group:
#include "Game.h"
```
```c++
// Globals
PT(CollisionHandlerPusher) pusher; // remove "auto " in assignment
std::unique_ptr<Player> player;
std::unique_ptr<WalkingEnemy> temp_enemy;
```
```c++
// in main:
player = std::make_unique<Player>();

temp_enemy = std::make_unique<WalkingEnemy>(LPoint3(5, 0, 0));
```
```c++
// In the update function:
player->update(key_map, dt);

temp_enemy->update(*player, dt);
```

If you run the game now, you should be able to move the player, much
as before--but you'll also be chased around (harmlessly--for now) by
an enemy!

Next, let's see how we handle collision events, via our trap enemy...

Proceed to Lesson8 to continue.
