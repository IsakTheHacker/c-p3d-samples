Lesson 14 (C++ Addendum)
------------------------

Panda3D does not distinguish between sound effects and music, but it
recommends that you do so.  The C++ API provides no built-in sound
managers.  You must create them and update them yourself.  Much like
the collision handler added to the update task in Lesson 6, I'll just
stuff the updaters into the task that was already created, even though
in larger code you should probably make that a separate task with a
different priority and/or operating frequency (for example, the Music
Box sample only needs updating every few seconds on my macine, not
every single frame, but that's only a single function call overhead,
really, so it probably doesn't hurt).  Sounds are not loaded by the
loader, but by the manager.

In fact, although I have not mentioned this before, I should now: the
Loader class in C++ is very different from the Python version.  It
only supports loading model files (at least that's what its
documentaiotn claims), and for the framework, it's not even the
preferred method of doing that.  Textures are loaded by the texture
classes.  Shaders are loaded by the shader classes.  And sounds are
loaded by the sound classes.  There is no `loadMusic`/`load_music`
method in any class:  instead, you create a music manager, and use its
`get_sound` method.

Since sound managers depend on your underlying sound driver, you can't
create a manager directly.  Instead, you have to use the static/global
`create_AudioManager` method.

After all that, it's still pretty simple to use sounds.  Just a few
extra steps that the Python users don't need:

In "Game.cpp":

```c++
// In the include group:
#include <panda3d/audio.h>
```
```c++
// In the globals:
PT(AudioManager) music_manager;
PT_AudioSound) music;
```
```c++
// In main:
music_manager = AudioManager::create_AudioManager();
music = music_manager->get_sound("Music/Defending-the-Princess-Haunted.ogg");
music->set_loop(true);
// I find this piece to be pretty loud,
// so I've turned the volume down a lot.
// Adjust to your settings and preferences!
// tjm -- I can't even hear that.  I use 0.75.  I'll leave this here, though.
music->set_volume(0.075); 
music->play();
```

```c++
// Somewhere near the top of update:
music_manager->update();
```

You should now hear music as you play!  However, I've noticed that the
music manager crashes on shutdown.  To prevent this, I zero out the
pointer before exiting in order to control destruction order better.
So, add this line at the end of main before returning:

```c++
music_manager = nullptr;
```

Next, a simple sound-effect. Specifically, we're going to have a sound
play whenever an enemy spawns:

```c++
// In the globals:
PT(AudioManager) sfx_manager;
PT_AudioSound) enemy_spawn_sound;
```

```c++
// in main
sfx_manager = AudioManager::create_AudioManager();
enemy_spawn_sound = sfx_manager->get_sound("Sounds/enemySpawn.ogg");
```

```c++
// In the "spawn_enemy" function:
enemy_spawn_sound->play();
```

```c++
// near the top of update():
sfx_manager->update();
```

```c++
// near the end of main:
enemy_spawn_sound = nullptr;
sfx_manager = nullptr;
```

And that's it. You should now hear a sound-effect whenever an enemy spawns.

With the procedure covered, let's quickly add similar sound-effects
for some other things.  Like the enemy spawn sound, these sounds
should be cleaned up before shutting down `sfx_manager`, so we'll now
call cleanup explicitly before shutting down the sound managers.

In Game.cpp:

```c++
// In main, before clearing the sound managers:
// May require moving the cleanup prototype decl up.
cleanup();
```

Then, we have to export the sound managers.  Technically, only the sfx
one, but it doesn't hurt to export both.

In Game.h:
```c++
extern PT(AudioManager) music_manager, sfx_manager;
```

Now we move to GameObject.cpp.  Of course we need the main audio header.

```c++
// In the include group:
#include <panda3d.audio.h>
```

Then we start adding, looping, and playing sounds.

```c++
// Gameobject.h, class references:
class AudioSound;
```
```c++
// GameObject.h, GameObject class:
PT(AudioSound) death_sound;
```
```c++
// GameObject.h, Player class:
PT(AudioSound) laser_sound_no_hit, laser_sound_hit, hurt_sound;
```
```c++
// GameObject.h, WalkingEnemy class:
PT(AudioSound) attack_sound;
```

```c++
// GameObject.h, TrapEnemy class:
~TrapEnemy();
PT(AudioSound) impact_sound, stop_sound, movement_sound;
```

```c++
// In the "alter_health" method of GameObject:

// At the start:
auto previous_health = self.health

// At the end:
if(previous_health > 0 && health <= 0 && death_sound)
    death_sound->play();
```

```c++
// In Player::Player:
laser_sound_no_hit = sfx_manager->get_sound("Sounds/laserNoHit.ogg");
laser_sound_no_hit->set_loop(true);
laser_sound_hit = sfx_manager->get_sound("Sounds/laserHit.ogg");
laser_sound_hit->set_loop(true);

hurt_sound = sfx_manager->get_sound("Sounds/FemaleDmgNoise.ogg");
```

```c++
// In the "update" method of Player:

// Directly after "if(scored_hit) {":
// We've hit something, so stop the "no-hit" sound
// and play the "hit something" sound
if(laser_sound_no_hit->status() == AudioSound::PLAYING)
    laser_sound_no_hit->stop();
if(laser_sound_hit->status() != AudioSound::PLAYING)
    laser_sound_hit->play();

// Directly after the associated "else":
// We're firing, but hitting nothing, so
// stop the "hit something" sound, and play
// the "no-hit" sound.
if(laser_sound_hit->status() == AudioSound::PLAYING)
    laser_sound_hit->stop();
if(laser_sound_no_hit->status() != AudioSound::PLAYING)
    laser_sound_no_hit->play();

// And similarly, after the next "else":
// We're not firing, so stop both the 
// "hit something" and "no-hit" sounds
if(laser_sound_no_hit->status() == AudioSound::PLAYING)
    laser_sound_no_hit->stop();
if(laser_sound_hit->status() == AudioSound::PLAYING)
    laser_sound_hit->stop();

// To clarify, both of those last two sections
// go next to calls to "beam_hit_model.hide()l",
// and calls to "render.clearLight"
// Put another way, the skeleton of the code looks
// something like this:
if(keys[k_shoot]) {
    if(ray_queue->get_num_entries() > 0) {
        // ...

        if(scored_hit) {
            // Section 1, above

            // ...
        } else {
            // Section 2, above
            
            // ...
} else {
    // ...

    // Section 3, above.
```

```c++
// In the "alterHealth" method of Player:
hurt_sound->play();
```

```c++
// In Player::~Player:
laser_sound_hit->stop();
laser_sound_no_hit->stop();
```

```c++
// In WalkingEnemy::WalkingEnemy:

// This "death_sound" is the one that will
// be used by the logic that we just added
// to GameObject, above
death_sound = sfx_manager->get_sound("Sounds/enemyDie.ogg");
attack_sound = sfx_manager->get_sound("Sounds/enemyAttack.ogg");
```

```c++
// In the "run_logic" method of WalkingEnemy:

// Just after "attack_anim->play();":
attack_sound->play();
```

```c++
// In TrapEnemy::TrapEnemy:
impact_sound = sfx_manager->get_sound("Sounds/trapHitsSomething.ogg");
stop_sound = sfx_manager->get_sound("Sounds/trapStop.ogg");
movement_sound = sfx_manager->get_sound("Sounds/trapSlide.ogg");
movement_sound->set_loop(true);
```

```c++
// In the "run_logic" method of TrapEnemy:

// Replace the old if(cabs(detector) < 0.5)... with:
if(cabs(detector) < 0.5) { // add braces
    move_direction = movement < 0 ? -1 : movement > 0 ? 1 : 0;
    movement_sound->play(); // new line
}
```

```c++
// In the "TrapEnemy" class, we add
// a new destructor:
TrapEnemy::~TrapEnemy()
{
    movement_sound->stop();
}
```

Finally, the trap enemies' sounds need to be activated in the mainline
functions, since that's where the actions are handled.  That's also
why I made those sounds public rather than private/protected, by the
way.

In "Game.cpp":

```c++
// In the "stop_trap" function:

// In the "if(!owner.empty()) {" section:
trap->movement_sound->stop();
trap->stop_sound->play();
```

```c++
// In the "trap_hits_something" function:

// In the "if(!owner.empty()) {" section:
trap->impact_sound->play();
```

Now we're done!

Proceed to [Lesson 15](../Lesson15) to continue.

Or, read this competely out of context aside by TJM (as if anyone is
actually reading this but me):

This entire tutorial consists of code fragements interleaved with
documentation.  I believe the best way to present such things (and, in
fact, a long time ago, I believed the best way to present *any* code)
is via [literate
programming](http://www.literateprogramming.com/articles.html).  Just
think: not only would all the "to be clear, here is where I mean" and
"In xx.cpp:" comments above disappear, you could just comple the code
directly from the document (i.e., no "Reference Code" section any
more)!  If I had all the time in the world, I'd go ahead and convert
this entire tutorial into a literate program, but as is, I have less
than a month of relative freedom left, and I can't waste any more time
on Panda3D, or anything else that doesn't directly benefit me, for
that matter.  Besides, [my literate programming support
tools](https://github.com/darktjm/literate-build) only support
generation of standalone HTML and LaTeX, neither of which is displayed
conveniently by github.
