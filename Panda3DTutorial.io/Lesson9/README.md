Lesson 9 (C++ Addendum)
-----------------------

The following expects you to first read:

[Lesson 9](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson09.html)

For the moment, since we don't have a visual representation of the
laser, we'll just print out what it hits.

In "GameObject.h":
```c++
// Below the include group:
class CollistionHandlerQueue;
class CollisionRay;
```
```c++
// In the Player class definition:
    ~Player();
  protected:
    PT(CollisionRay) ray;
    PT(CollisionHandlerQueue) ray_queue;
    NodePath ray_node_path;
    PN_stdfloat damage_per_second;
```

In "GameObject.cpp":
```c++
// In your include group:
#include <panda3d/collisionHandlerQueue.h>
#include <panda3d/collisionRay.h>
```
```c++
// In Player::Player:
ray = new CollisionRay(0, 0, 0, 0, 1, 0);

auto ray_node = new CollisionNode("playerRay");
ray_node->add_solid(ray);

ray_node->set_from_collide_mask(1 << 2);
ray_node->set_into_collide_mask(0);

ray_node_path = window->get_render().attach_new_node(ray_node);
ray_queue = new CollisionHandlerQueue;

// We want this ray to collide with things, so
// tell our traverser about it. However, note that,
// unlike with "CollisionHandlerPusher", we don't
// have to tell our "CollisionHandlerQueue" about it.
c_trav.add_collider(ray_node_path, ray_queue);

damage_per_second = -5.0;
```
```c++
// In the "update" method of the "Player" class:

// If we're pressing the "shoot" button, check
// whether the ray has hit anything, and if so,
// examine the collision-entry for the first hit.
// If the thing hit has an "owner" Python-tag, then
// it's a GameObject, and should try to take damage--
// with the exception if "TrapEnemies",
// which are invulnerable.
if(key_map[k_shoot]) {
    if(ray_queue->get_num_entries() > 0) {
        ray_queue->sort_entries();
        auto ray_hit = ray_queue->get_entry(0);
        auto hit_pos = ray_hit->get_surface_point(window->get_render());

        std::cout << hit_node_path << '\n';
        auto hit_node_path = ray_hit->get_into_node_path();
        auto owner = hit_node_path.get_tag("owner");
        if(!owner.empty()) {
            auto hit_object = reinterpret_cast<GameObject *>(std::stoul(owner));
            if(!dynamic_cast<TrapEnemy *>(hit_object))
                 hit_object->alter_health(damage_per_second*dt);
        }
```
```c++
// And finally, a bit of extra cleaning up.
Player::~Player()
{
    c_trav.remove_collider(ray_node_path);
}
```

Note that I originally assumed that health was an integer (couldn't
really tell with Python, since variables have no type).  As it turns
out, the code above adjusts health by a floating point number, so I
converted all health variables from `int` to `PN_stdfloat`.  You can
find them all by just searching for `health` in the code.

Note in the above code that the Python InstanceOf function was replaced
by `dynamic_cast`.  We can't use DCAST because these are plain C++
types, but even if we could, we wouldn't want to, because it's not an
error to fail.  Instead, we use success as an indicator that the
object is, in fact, a TrapEnemy.

I don't know if BitMask objects are really necessary in Python, but
they definitely aren't explicitly needed in C++.  Just use the
standard way of doing bit masks: `1<<`*n* to represent bit *n*.  Or
them together if there's more than one.  I imagine the same thing
could be done in Python with `2**`*n*.  The C++ API also supports the
BitMask class, but I won't be using it.

In our case, we want to make sure that the player has a mask, and that
the player's ray has different mask, so that they don't collide.
Conversely, we want our enemy to have a mask that matches the ray's,
so that they _do_ collide:

```c++
// In Player::Player:

// This is the important one for preventing ray-collisions.
// The other is more a gameplay decision.
// Note that as usual, node() must be cast to the correct type first
// unlike Python.
auto collider_node = DCAST(CollisionNode, collider.node());
collider_node->set_into_collide_mask(1 << 1);

collider_node->set_from_collide_mask(1 << 1);

// After we've made our ray-node:

// Note that we set a different bit here!
// This means that the ray's mask and
// the collider's mask don't match, and
// so the ray won't collide with the
// collider.
ray_node->set_from_collide_mask(1 << 2);

ray_node->set_into_collide_mask(0);
```
```c++
// Then, in WalkingEnemy::WalkingEnemy:

// Note that this is the same bit as we used for the ray!
DCAST(CollisionNode, collider.node())->set_into_collide_mask(1 << 2);
```
```c++
// And in TrapEnemy::TrapEnemy:

// Trap-enemies should hit both the player and "walking" enemies,
// so we set _both_ bits here!
auto collider_node = DCAST(CollisionNode, collider.node());
collider_node->set_into_collide_mask((1 << 2) | (1 << 1));
collider_node->set_from_collide_mask((1 << 2) | (1 << 1));
```

We'll also want some means of representing our laser, and of showing
that it has hit something.

```c++
// In the Player class of GameObject.h:
NodePath beam_model;
```
```c++
// In Player::Player:

// A nice laser-beam model to show our laser
beam_model = window->load_model(actor, "Models/Misc/bambooLaser");
beam_model.set_z(1.5);
// This prevents lights from affecting this particular node
beam_model.set_light_off();
// We don't start out firing the laser, so 
// we have it initially hidden.
beam_model.hide();
```
```c++
// In the "update" method of the "Player" class:

// We've seen this bit before--the new stuff is inside
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

        // NEW STUFF STARTS HERE

        // Find out how long the beam is, and scale the
        // beam-model accordingly.
        auto beam_length = (hit_pos - actor.get_pos()).length();
        beam_model.set_sy(beam_length);

        beam_model.show();
     }
} else
    // If we're not shooting, don't show the beam-model.
    beam_model.hide();
```

We'll have our laser fire towards the mouse-cursor, and start at the
player-character's position. Furthermore, we'll have our character
face the direction in which we're firing. As it happens, these two
things work together nicely...

Proceed to [Lesson 10](../Lesson10) to continue.
