Lesson 8 (C++ Addendum)
-----------------------

The following expects you to first read:

[Lesson 8](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson08.html)

To start with, let's make the trap enemy.

In "GameObject.h":
```c++
class TrapEnemy : public Enemy {
  public:
    TrapEnemy(LPoint3 pos);
    int move_direction;
    bool ignore_player;
  protected:
    void run_logic(Player &player, double dt);
    bool move_in_x;
};
```

In "GameObject.cpp":
```c++
TrapEnemy::TrapEnemy(LPoint3 pos) :
    Enemy(pos,
          "Models/Misc/trap",
          {
              "Models/Misc/trap-stand", // stand
              "Models/Misc/trap-walk"   // walk
          },
          100.0,
          10.0,
          "trapEnemy"), move_direction(0),
          // This will allow us to prevent multiple
          // collisions with the player during movement
          ignore_player(false),
          move_in_x(false)
{
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
```

If you're looking for `alter_health`:  "pass" is the default behavior,
so there's no need to add the method (or make `alter_health` virtual
to begin with).

If you're wondering what `PN_stdfloat` is, it's Panda3D's default
floating point type.  You can build the Panda3D library to use either
single-precision or double-precision floating point values by default.
You can use preprocessor defines to switch your code to use the
preferred default (`#ifdef STDFLOAT_DOUBLE`), or, more cleanly, use
the aliases the library provides.  For the base type, it's
`PN_stdfloat`. For vectors of various kinds, it's the type name
without a suffix.  For example, there's an LVector3f
(single-precision), an LVector3d (double-precision), and LVector3 (the
default type).  When dealing with Panda3D functions, it is probably
best to always use the default type.  There are also predefined
aliases for various standard math library functions.  For example,
rather than having to figure out if `sinf` or `sin` is the correct
function, use `csin`.  See `<panda3d/cmath.h>` (not to be confused
with the standard C++ `<cmath>`) for a list.  There are also some
constants defined by the MathNumbers class, such as pi and ln2, which
also come in single-precision (f suffix) and double-precision (d
suffix) forms. Of course you could also just stick to float or double,
as is your preference, but the library will always use the default
type unless otherwise specified.  With Python, you'll use whatever
Python likes, and be happy with it.  You do have the type aliases
there as well, though, for the vector classes.

Next, let's add a temporary testing-trap to the level, in our
"Game.cpp" file:

```c++
// In the globals:
std::unique_ptr<TrapEnemy> temp_trap;
```
```c++
// In the mainline:
temp_trap = std::make_unique<TrapEnemy>(LPoint3(-2, 7, 0));
```
```c++
// In the "update" function:
temp_trap->update(*player, dt);
```

For now, the trap will move, but never stop attempting to move.
Similarly, while it will collide with other objects, it won't do any
harm.

So we add events.  See the Python version for details on how this works.

To generate events:

In "Game.cpp":
```c++
// In the mainline:
pusher->add_in_pattern("%fn-into-%in");
```

To hook into those events, we use functions instead of macros.

```c++
// Before main
void stop_trap(const Event *ev, void *data),
     trap_hits_something(const Event *ev, void *data);
```
```c++
// in main
// using the same evhand alias we created before
evhand.add_hook("trapEnemy-into-wall", stop_trap, 0);
evhand.add_hook("trapEnemy-into-trapEnemy", stop_trap, 0);
evhand.add_hook("trapEnemy-into-player", trap_hits_something, 0);
evhand.add_hook("trapEnemy-into-walkingEnemy", trap_hits_something, 0);
```

Note that instead of crashing, the game will simply not compile if the
functions do not exist.  It's one of those errors C++ catches without
having to run the whole program.

A collision event has a single parameter attached, which is a
"collision entry".  Event structures have variant types as their
paramers.  To retrieve the first parameter from the event `ev`, use
`ev->get_parameter(0)`.  The variant type (EventParameter) can then be
converted to a more specific type using a getter.  The specific getter
we're interested in is `get_ptr()`, which converts the parameter to a
standard Panda3D named type object.  Unlike Python, with its so-called
Duck Typing (really, no typing at all:  everything is essentially a
diction ary lookup, so it doesn't matter what "type" an object is),
you can't just start using this as a CollisionEntry.  You have to cast
the pointer to the correct type.  The parameter was safely converted
to a named type pointer by virtue of its stored parameter type.  To
convert that pointer into a CollisionEntry pointer safely, you need to
use the run-time type information stored with the pointer.  There are
actually two sources of run-time type information:  the C++ standard
(which can be disabled by the compiler) and Panda3D's own (descendents
of TypedObject).  The safe way to cast using the standard is
`dynamic_cast<CollisionEntry *>(ptr)`.  The safe way using Panda3D is
`DCAST(CollisionEntry, ptr)`.  The only real difference is which type
information is used and whether or not an error is printed if the
conversion fails (only DCAST does this).  Both methods return 0
(`nullptr`) on failure.  Since you can't avoid having Panda3D's type
information, but you can, technically, disable the compiler's type
information, it's probaby safer overall to use the DCAST method.  It's
shorter, as well.  Note that in spite of my previous comment, anything
involving `dynamic_cast`, `reinterpret_cast`, or DCAST may result in
crashes if you don't check the results.  I don't, because as long as
things are working properly, it should never fail, and I want to keep
the code short.

Note the use of our tags to access the GameObjects associated with the
colliders.  They are parsed by getting the tag (an empty string is
returned if not present, thus eliminatint seprate check and get
calls), converting the string to an unsigned long (guaranteed to be at
least large enough to store a pointer), and casting it (not
dynamically, since it has no base class) to the expected type:  either
GameObject, or, when the more specific type is known, that type.

```c++
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

        // We don't want stationary traps to do damage,
        // so ignore the collision if the "moveDirection" is 0
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
```

We'll get to reactions to damage soon enough, but first, let's provide
our player with a way to fight back...

Proceed to [Lesson 9](../Lesson9) to continue.
