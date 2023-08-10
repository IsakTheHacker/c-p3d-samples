Lesson 13 (C++ Addendum)
------------------------

The following expects you to first read:

[Lesson 13](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson13.html)

To start with, two minor changes in "GameObject.cpp". These simply have
the Walking Enemy play a "spawn" animation when it's constructed, and
skip the behaviour in "run_logic" until the "spawn" animation is done:

```c++
// In the WalkingEnemy::WalkingEnemy:
spawn_anim->play();
```

```c++
// In the "run_logic" method of WalkingEnemy:

// At the start of the method:
if(spawn_anim && spawn_anim->is_playing())
    return;
```

Now, on to "Game.cpp".

First, delete the code related to "temp_enemy" and temp_trap", in the
global variables and the main and update functions.

What we're going to do is keep a few lists: one of enemies, one of
traps, one of "dead enemies", and one of spawn-points for
enemies.  Once again, we'll be using standard C++ automatic pointers.
In this case, since there may be multiple references to enemies, I
used `shared_ptr` instead of `unique_ptr`.

We'll also create a new "start_game" function to contain the logic for
the spawning of traps, creation of the player, and a few other things.
Since creation of the player has moved, remove the `player =
std::unique_ptr<Player>()` line from main as well.

```c++
// In the global variables:
typedef std::shared_ptr<Enemy> EnemyPtr; // saves typing
std::vector<EnemyPtr> enemies;
std::vector<EnemyPtr> trap_enemies;
std::vector<EnemyPtr> dead_enemies;
std::vector<LPoint3> spawn_points;

// Values to control when to spawn enemies, and
// how many enemies there may be at once
PN_stdfloat initial_spawn_interval, minimum_spawn_interval, spawn_interval;
PN_stdfloat spawn_timer;
unsigned max_enemies, maximum_max_enemies, num_traps_per_side;

PN_stdfloat difficulty_interval, difficulty_timer;

// forward decl.
void start_game(void);
```

```c++
// In main:
// Set up some spawn points
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

temp_enemy = std::make_unique<WalkingEnemy>(LPoint3(5, 0, 0));
temp_trap = std::make_unique<TrapEnemy>(LPoint3(-2, 7, 0));
num_traps_per_side = 2;

difficulty_interval = 5.0;
difficulty_timer = difficulty_interval;

// Start the game!
start_game();
```

```c++
// Elsewhere in Game.cpp:

void cleanup(void);
void start_game()
{
    // We'll add this function presently.
    // In short, clean up anything in the
    // level--enemies, traps, etc.--before
    // starting a new one.
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

    // Create one trap on each side, repeating
    // for however many traps there should be
    // per side.
    // TJM - note: I consolidated the code a bit.  Probably should've
    // done that in a few other places as well.
    for(unsigned i = 0; i < num_traps_per_side; i++) {
        // Instead of 4x the code, use a loop
        for(unsigned j = 0; j < 4; j++) {
            unsigned slotno = floor(drand48() * side_trap_slots[j].size());
            auto slot = side_trap_slots[j][slotno];
            // Note that we "erase" the chosen location,
            // so that it won't be chosen again.
            side_trap_slots[j].erase(side_trap_slots[j].begin() + slotno);
            auto other = (j % 2) ? -7.0 : 7.0;
            LPoint3 pos(j/2 ? other : slot, j/2 ? slot : other, 0.0);
            trap_enemies.push_back(std::make_unique<TrapEnemy>(pos));
        }
    }
}
```

Next, we'll add a function to clean up a level once we're done with
it.  Since all the object cleanup was done using destructors, it will
happen automatically once the object is freed.  Freeing also happens
automatically, due to our use of reference counted pointers.  So, just
clear out all those pointers.  This is exactly what happens
automatically on system exit, as well.  Personally, I would prefer a
way to do a faster exit, without all that cleanup, since it's mostly
internal structures, and the OS automatically cleans up the rest,
anyway (or at least it should).

```c++
// Elsewhere in Game.cpp:
void cleanup()
{
    enemies.clear();
    dead_enemies.clear();
    trap_enemies.clear();
    player = nullptr;
}
```

Next, we'll add a function to spawn an enemy.

```c++
void spawn_enemy()
{
    if(enemies.size() < max_enemies) {
	auto spawn_point = spawn_points[drand48()*spawn_points.size()];

	enemies.push_back(std::make_shared<WalkingEnemy>(spawn_point));
    }
}
```

And finally, we'll implement the logic to make it all do something.

```c++
// In the "update" function:

// If the player is dead, or we're not
// playing yet, ignore this logic.
if(player != nullptr && player->get_health() > 0) {
    auto key_map = reinterpret_cast<bool *>(data);
    player->update(key_map, dt);

    // Wait to spawn an enemy...
    spawn_timer -= dt;
    if(spawn_timer <= 0) {
        spawn_timer = spawn_interval;
        spawn_enemy();
    }

    // Update all enemies and traps
    for(auto &enemy: enemies)
        enemy->update(*player, dt);
    for(auto &trap: trap_enemies)
        trap->update(*player, dt);

    // Find the enemies that have just
    // died, if any
    std::vector<std::shared_ptr<Enemy>> newly_dead_enemies;
    for(auto &enemy: enemies)
        if(enemy->get_health() <= 0)
            newly_dead_enemies.push_back(enemy);
    // And re-build the enemy-list to exclude
    // those that have just died.
    // This is awkward due to my trying to match the Python code (mostly).
    // A better way would've been to loop using indices and do it all
    // in one loop.
    // This is improved in C++-20, but I'm sticking to 11.
    // std::erase_if(enemies, [] { ... });
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                 [](EnemyPtr enemy) {
                                     return enemy->get_health() <= 0;
                                 }), enemies.end());

    // Newly-dead enemies should have no collider,
    // and should play their "die" animation.
    // In addition, increase the player's score.
    for(auto enemy : newly_dead_enemies) {
        enemy->disable_collider();
        enemy->die_anim->play();
        player->add_score(enemy->get_score_value());
    }
    if(newly_dead_enemies.size() > 0)
        player->update_score();

    dead_enemies.insert(dead_enemies.end(), newly_dead_enemies.begin(),
                        newly_dead_enemies.end());

    // Check our "dead enemies" to see
    // whether they're still animating their
    // "die" animation. In not, clean them up,
    // and drop them from the "dead enemies" list.
    // Once again a bit awkward.
    // This is improved in C++-20, but I'm sticking to 11.
    // std::erase_if(dead_enemies, [] { ... });
    dead_enemies.erase(std::remove_if(dead_enemies.begin(),
                                      dead_enemies.end(), [](EnemyPtr enemy) {
                                          auto anim = enemy->die_anim;
                                          return !anim || !anim->is_playing();
                                      }), dead_enemies.end());

    // Make the game more difficult over time!
    difficulty_timer -= dt;
    if(difficulty_timer <= 0) {
        difficulty_timer = difficulty_interval;
        if(max_enemies < maximum_max_enemies)
            max_enemies++;
        if(spawn_interval > minimum_spawn_interval)
            spawn_interval -= 0.1;
    }
}
```

Proceed to [Lesson 14](../Lesson14) to continue.
