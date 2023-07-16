Lesson 12 (C++ Addendum)
------------------------

The following expects you to first read:

[Lesson 12](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson12.html)

First of all, let's display the player's health and score, both near
the top-left of the screen. 

The score is plain text.  There is no handy OnScreenText (it's
DirectGUI), but creating identical text is pretty straightforward.

The player's health will be displayed as a row of "heart"-icons. Again
there's no DirectGUI OnScreenImage class.  There is, however, a
convenient feature of `window->load_model`: if you load a texture, it
will automatically build geometry to display it.  Unfortunately, it
seems about 10 times as large, so the required scale factor is
adjusted accordingly.  There are, of course, other ways to do it, but
this seems most similar to the Python code.

Now that `alter_health` is actually overridden by a real function,
it's time to make it virtual as well.  Simply add the `virtual`
keyword in front of its definition in `GameObject.h`'s GameObject
class definition.

In "GameObject.h":
```c++
//  GameObject.h, below include group:
class TextNode;
```
```c++
// GameObject.h, Player class:
  void update_score();
  void alter_health(PN_stdfloat d_health);
protected:
  void update_health_ui();
  int score;
  NodePath score_ui;
  TextNode *score_ui_text;
  std::vector<NodePath> health_icons;
```

In "GameObject.cpp":

```c++
// In Plaer::Player:
// add , health_icons() to initializers to be safe
// replace window->get_render() with an alias near top
auto render = window->get_render();
ray_node_path = render.attach_new_node(ray_node); // replaces old line

score = 0;

// This is OnScreenText
auto a2d = window->get_aspect_2d(); // ost default
score_ui_text = new TextNode("score");
score_ui = a2d.attach_new_node(score_ui_text);
score_ui_text->set_text("0");
score_ui_text->set_align(TextNode::A_left);
score_ui.set_pos(-1.3, 0, 0.825); // default center (0,0,0)
score_ui_text->set_text_color(0, 0, 0, 1); // ost default
score_ui.set_scale(0.07); // ost default

for(int i = 0; i < max_health; i++) {
    // This is sort of OnScreenImage
    auto icon = window->load_model(a2d, "UI/health.png");
    icon.set_pos(-1.275 + i*0.075, 0, 0.95);
    icon.set_scale(0.004); // but it's 10x bigger, so .004 instead of .04
    // Since our icons have transparent regions,
    // we'll activate transparency.
    icon.set_transparency(TransparencyAttrib::M_alpha);
    health_icons.push_back(icon);
}
```

```c++
// Later...
void Player::update_score()
{
    score_ui_text->set_text(std::to_string(score));
}

void Player::alter_health(PN_stdfloat d_health)
{
    GameObject::alter_health(d_health);

    update_health_ui();
}

void Player::update_health_ui()
{
    for(unsigned i = 0; i < health_icons.size(); i++) {
	if(i < health)
	    health_icons[i].show();
	else
	    health_icons[i].hide();
    }
}
```

```c++
// And finally, in Player::~Player:
score_ui.remove_node();

for(auto &icon: health_icons)
    icon.remove_node();
```

As for the Walking Enemy's health, we'll simply shade its model to
black as it becomes more damaged.

```c++
// In the WalkingEnemy class declaration GameObject.h:
  void alter_health(PN_stdfloat d_health);
protected:
  void update_health_visual();
```

```c++
// Back to GameObject.cpp...
void WalkingEnemy::alter_health(PN_stdfloat d_health)
{
    Enemy::alter_health(d_health);
    update_health_visual();
}

void WalkingEnemy::update_health_visual()
{
    auto perc = health/max_health;
    if(perc < 0)
	perc = 0;
    // The parameters here are red, green, blue, and alpha
    actor.set_color_scale(perc, perc, perc, 1);
}
```

Next, we're going to add two effects that will show up when we hit a
Walking Enemy.

```c++
// In GameObject.h, Player class declaration:
NodePath beam_hit_model, beam_hit_light_node_path;
PN_stdfloat beam_hit_pulse_rate, beam_hit_timer;
```

```c++
// In your include group:
#include <panda3d/pointLight.h>
```

```c++
// In Player::Player:
beam_hit_model = window->load_model(render, "Models/Misc/bambooLaserHit");
beam_hit_model.set_z(1.5);
beam_hit_model.set_light_off();
beam_hit_model.hide();

beam_hit_pulse_rate = 0.15;
beam_hit_timer = 0;

auto beam_hit_light = new PointLight("beamHitLight");
beam_hit_light->set_color(LColor(0.1, 1.0, 0.2, 1));
// These "attenuation" values govern how the light
// fades with distance. They are, respectively,
// the constant, linear, and quadratic coefficients
// of the light's falloff equation.
// I experimented until I found values that
// looked nice.
beam_hit_light->set_attenuation({1.0, 0.1, 0.5});
beam_hit_light_node_path = render.attach_new_node(beam_hit_light);
// Note that we haven't yet applied the light to
// a NodePath, and so it won't yet illuminate
// anything.
```

```c++
// In Player::update:

// In short, run a timer, and use the timer in a sine-function
// to pulse the scale of the beam-hit model. When the timer
// runs down (and the scale is at its lowest), reset the timer
// and randomise the beam-hit model's rotation.
beam_hit_timer -= dt;
if(beam_hit_timer <= 0) {
    beam_hit_timer = beam_hit_pulse_rate;
    beam_hit_model.set_h(drand48()*360.0);
}
beam_hit_model.set_scale(csin(beam_hit_timer*M_PI/beam_hit_pulse_rate)*0.4 + 0.9);
```

We'll be adding lines within the `if keys[k_shoot]` code-section that
we already have in Player's "update" method. So, instead of just
showing the new lines, I'm going to show the full section of code and
mark the new lines with "// NEW!!!":

```c++
// In the "update" method of Player:

if(key_map[k_shoot]) {
    if(ray_queue->get_num_entries() > 0) {
        auto scored_hit = false;

        ray_queue->sort_entries();
        auto ray_hit = ray_queue->get_entry(0);
        auto hit_pos = ray_hit->get_surface_point(window->get_render());

        auto hit_node_path = ray_hit->get_into_node_path();
        auto owner = hit_node_path.get_tag("owner");
        if(!owner.empty()) {
            auto hit_object = reinterpret_cast<GameObject *>(std::stoul(owner));
            if(!dynamic_cast<TrapEnemy *>(hit_object)) {
                hit_object->alter_health(damage_per_second*dt);
                scored_hit = true;
            }
        }
        auto beam_length = (hit_pos - actor.get_pos()).length();
        beam_model.set_sy(beam_length);

        beam_model.show();

        // NEW!!!
        if(scored_hit) {
            beam_hit_model.show();

            beam_hit_model.set_pos(hit_pos);
            beam_hit_light_node_path.set_pos(hit_pos + LVector3(0, 0, 0.5));

            // If the light hasn't already been set here, set it
            if(!render.has_light(beam_hit_light_node_path))
                // Apply the light to the scene, so that it
                // illuminates things
                render.set_light(beam_hit_light_node_path);
        } else {
            // If the light has been set here, remove it
            // See explanation in the tutorial-text below...
            if(render.has_light(beam_hit_light_node_path))
                // Clear the light from the scene, so that it
                // no longer illuminates anything
                render.clear_light(beam_hit_light_node_path);

             beam_hit_model.hide();
        }
    }
} else {
    // NEW!!!
    if(render.has_light(beam_hit_light_node_path))
        // Clear the light from the scene, so that it
        // no longer illuminates anything
        render.clear_light(beam_hit_light_node_path);

    beam_model.hide();

    // NEW!!!
    beam_hit_model.hide();
}
```

And finally, some more cleaning up:

```c++
// In Player::~Player:
beam_hit_model.remove_node();

window->get_render().clear_light(beam_hit_light_node_path);
beam_hit_light_node_path.remove_node();
```

And finally, we'll turn the tables and add a hit-flash that shows that
the player has taken damage.

```c++
// In the Player class declaration in GameObject.h:
NodePath damage_taken_model, beam_hit_light_node_path;
PN_stdfloat damage_taken_model_timer, damage_taken_model_duration;
```
```c++
// In Player::Player:

damage_taken_model = window->load_model(actor, "Models/Misc/playerHit");
damage_taken_model.set_light_off();
damage_taken_model.set_z(1.0);
damage_taken_model.hide();

damage_taken_model_timer = 0;
damage_taken_model_duration = 0.15;
```
```c++
// In the "update" method of Player:

if(damage_taken_model_timer > 0) {
    damage_taken_model_timer -= dt;
    damage_taken_model.set_scale(2.0 - damage_taken_model_timer/damage_taken_model_duration);
    if(damage_taken_model_timer <= 0)
        damage_taken_model.hide();
}
```

```c++
// In the "alterHealth" method of Player:

damage_taken_model.show();
damage_taken_model.set_h(drand48()*360.0);
damage_taken_model_timer = damage_taken_model_duration;
```

Proceed to [Lesson 13](../Lesson13) to continue.
