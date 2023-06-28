/*
 * Translation of Python disco-lights sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/disco-lights
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * 
 * Author: Jason Pratt (pratt@andrew.cmu.edu)
 * Last Updated: 2015-03-13
 *
 * This project demonstrates how to use various types of
 * lighting
 */
#include <pandaFramework.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <spotlight.h>
#include <pointLight.h>
#include <sstream>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
// The Python sample uses a class, which can have forward references even
// in C++.  I am using globals, though, as it saves a few headaches.  Thus,
// they need to all be delcared at the top.
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;
TextNode *ambient_text, *directional_text, *spotlight_text, *point_light_text,
         *spinning_text, *ambient_brightness_text,
         *directional_brightness_text, *spotlight_brightness_text,
         *spotlight_exponent_text, *lighting_per_pixel_text,
         *lighting_shadows_text;
NodePath ambient_light, directional_light, spotlight, red_point_light,
         green_point_light, blue_point_light;
AmbientLight *ambient_light_node; // avoid DCAST
DirectionalLight *directional_light_node;
Spotlight *spotlight_node;
PT(NPAnim) point_lights_spin;
bool are_point_lights_spinning, per_pixel_enabled, shadows_enabled;

// Simple function to keep a value in a given range (by default 0 to 1)
template<class T> T clamp(T i, T mn=(T)0, T mx=(T)1)
{
    return std::min({std::max({i, mn}), mx});
}

// Macro-like function to reduce the amount of code needed to create the
// onscreen instructions
TextNode *make_status_label(int i)
{
    TextNode *text_node = new TextNode("status_label");
    auto path = NodePath(text_node);
    path.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_left);
    // style=1: default
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, .4);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    path.set_pos(-1.0 + 0.06, 0, 1.0 - 0.1 - 0.06 * i); // TopLeft == (-1,0,1)
    path.set_scale(0.05);
    return text_node; // may_change
}

void update_status_label(void);
void toggle_lights(std::vector<NodePath>lights);
void toggle_spinning_point_lights(void);
void toggle_per_pixel_lighting(void);
void toggle_shadows(void);
void adjust_spotlight_exponent(Spotlight *light, int amount);
void add_brightness(Light *light, PN_stdfloat amount);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Disco Lights - C++ Panda3D Samples");
    window = framework.open_window();

    // The main initialization
    // This creates the on screen title that is in every tutorial
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    // This code was modified slightly to match the other demos
    auto text_node = new TextNode("title");
    auto text = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Lighting");
    // style=1: default
    text_node->set_text_color(1, 1, 0, 1);
    text_node->set_shadow_color(0, 0, 0, 0.5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    text.set_pos(1.0-0.13, 0, -1+0.05); // BottomRight == (1,0,-1)
    text.set_scale(0.07);

    // Creates labels used for onscreen instructions
    ambient_text = make_status_label(0);
    directional_text = make_status_label(1);
    spotlight_text = make_status_label(2);
    point_light_text = make_status_label(3);
    spinning_text = make_status_label(4);
    ambient_brightness_text = make_status_label(5);
    directional_brightness_text = make_status_label(6);
    spotlight_brightness_text = make_status_label(7);
    spotlight_exponent_text = make_status_label(8);
    lighting_per_pixel_text = make_status_label(9);
    lighting_shadows_text = make_status_label(10);

    auto disco = def_load_model("models/disco_hall");
    disco.reparent_to(window->get_render());
    disco.set_pos_hpr(0, 50, -4, 90, 0, 0);

    // First we create an ambient light. All objects are affected by ambient
    // light equally
    // Create and name the ambient light
    ambient_light_node = new AmbientLight("ambient_light");
    ambient_light = window->get_render().attach_new_node(ambient_light_node);

    // Set the color of the ambient light
    ambient_light_node->set_color(LColor(.1, .1, .1, 1));
    // add the newly created light to the light_attrib

    // Now we create a directional light. Directional lights add shading from a
    // given angle. This is good for far away sources like the sun
    directional_light_node = new DirectionalLight("directional_light");
    directional_light = window->get_render().attach_new_node(directional_light_node);
    directional_light_node->set_color(LColor(.35, .35, .35, 1));
    // The direction of a directional light is set as a 3D vector
    directional_light_node->set_direction(LVector3(1, 1, -2));
    // These settings are necessary for shadows to work correctly
    directional_light.set_z(6);
    auto dlens = directional_light_node->get_lens();
    dlens->set_film_size(41, 21);
    dlens->set_near_far(50, 75);
    // directional_light_node->show_frustum();

    // Now we create a spotlight. Spotlights light objects in a given cone
    // They are good for simulating things like flashlights
    auto camera = window->get_camera_group();
    spotlight_node = new Spotlight("spotlight");
    spotlight = camera.attach_new_node(spotlight_node);
    spotlight_node->set_color(LColor(.45, .45, .45, 1));
    spotlight_node->set_specular_color(LColor(0, 0, 0, 1));
    // The cone of a spotlight is controlled by its lens. This creates the
    // lens
    spotlight_node->set_lens(new PerspectiveLens);
    // This sets the Field of View (fov) of the lens, in degrees for width
    // and height.  The lower the numbers, the tighter the spotlight.
    spotlight_node->get_lens()->set_fov(16, 16);
    // Attenuation controls how the light fades with distance.  The three
    // values represent the three attenuation constants (constant, linear,
    // and quadratic) in the internal lighting equation. The higher the
    // numbers the shorter the light goes.
    spotlight_node->set_attenuation(LVector3(1, 0.0, 0.0));
    // This exponent value sets how soft the edge of the spotlight is.
    // 0 means a hard edge. 128 means a very soft edge.
    spotlight_node->set_exponent(60.0);

    // Now we create three colored Point lights. Point lights are lights that
    // radiate from a single point, like a light bulb. Like spotlights, they
    // are given position by attaching them to NodePaths in the world
    auto red_helper = def_load_model("models/sphere");
    red_helper.set_color(LColor(1, 0, 0, 1));
    red_helper.set_pos(-6.5, -3.75, 0);
    red_helper.set_scale(.25);
    auto red_point_light_node = new PointLight("red_point_light");
    red_point_light = red_helper.attach_new_node(red_point_light_node);
    red_point_light_node->set_color(LColor(.35, 0, 0, 1));
    red_point_light_node->set_attenuation(LVector3(.1, 0.04, 0.0));

    // The green point light and helper
    auto green_helper = def_load_model("models/sphere");
    green_helper.set_color(LColor(0, 1, 0, 1));
    green_helper.set_pos(0, 7.5, 0);
    green_helper.set_scale(.25);
    auto green_point_light_node = new PointLight("green_point_light");
    green_point_light = green_helper.attach_new_node(green_point_light_node);
    green_point_light_node->set_attenuation(LVector3(.1, .04, .0));
    green_point_light_node->set_color(LColor(0, .35, 0, 1));

    // The blue point light and helper
    auto blue_helper = def_load_model("models/sphere");
    blue_helper.set_color(LColor(0, 0, 1, 1));
    blue_helper.set_pos(6.5, -3.75, 0);
    blue_helper.set_scale(.25);
    auto blue_point_light_node = new PointLight("blue_point_light");
    blue_point_light = blue_helper.attach_new_node(blue_point_light_node);
    blue_point_light_node->set_attenuation(LVector3(.1, 0.04, 0.0));
    blue_point_light_node->set_color(LColor(0, 0, .35, 1));
    blue_point_light_node->set_specular_color(LColor(1, 1, 1, 1));

    // Create a dummy node so the lights can be spun with one command
    auto point_light_helper = window->get_render().attach_new_node("point_light_helper");
    point_light_helper.set_pos(0, 50, 11);
    red_helper.reparent_to(point_light_helper);
    green_helper.reparent_to(point_light_helper);
    blue_helper.reparent_to(point_light_helper);

    // Finally we store the lights on the root of the scene graph.
    // This will cause them to affect everything in the scene.
    window->get_render().set_light(ambient_light);
    window->get_render().set_light(directional_light);
    window->get_render().set_light(spotlight);
    window->get_render().set_light(red_point_light);
    window->get_render().set_light(green_point_light);
    window->get_render().set_light(blue_point_light);

    // Create and start interval to spin the lights, and a variable to
    // manage them.
    point_lights_spin = new NPAnim(point_light_helper, "point_lights_spin", 6);
    point_lights_spin->set_end_hpr(LVector3(360, 0, 0));
    point_lights_spin->loop();
    are_point_lights_spinning = true;

    // Per-pixel lighting and shadows are initially off
    per_pixel_enabled = false;
    shadows_enabled = false;

    // listen to keys for controlling the lights
    window->enable_keyboard();
    framework.define_key("escape", "Quit", framework.event_esc, &framework);
    // instead of awkward casting, I use lambdas
    framework.define_key("a", "", EV_FN() { toggle_lights({ambient_light}); }, 0);
    framework.define_key("d", "", EV_FN() { toggle_lights({directional_light}); }, 0);
    framework.define_key("s", "", EV_FN() { toggle_lights({spotlight}); }, 0);
    framework.define_key("p", "", EV_FN() { toggle_lights({red_point_light,
				 green_point_light,
				 blue_point_light}); }, 0);
    framework.define_key("r", "", EV_FN() { toggle_spinning_point_lights(); }, 0);
    framework.define_key("l", "", EV_FN() { toggle_per_pixel_lighting(); }, 0);
    framework.define_key("e", "", EV_FN() { toggle_shadows(); }, 0);
    framework.define_key("z", "", EV_FN() { add_brightness(ambient_light_node, -.05); }, 0);
    framework.define_key("x", "", EV_FN() { add_brightness(ambient_light_node,  .05); }, 0);
    framework.define_key("c", "", EV_FN() { add_brightness(directional_light_node, -.05); }, 0);
    framework.define_key("v", "", EV_FN() { add_brightness(directional_light_node, .05); }, 0);
    framework.define_key("b", "", EV_FN() { add_brightness(spotlight_node, -.05); }, 0);
    framework.define_key("n", "", EV_FN() { add_brightness(spotlight_node, .05); }, 0);
    framework.define_key("q", "", EV_FN() { adjust_spotlight_exponent(spotlight_node, -1); }, 0);
    framework.define_key("w", "", EV_FN() { adjust_spotlight_exponent(spotlight_node, 1); }, 0);

    // Finally call the function that builds the instruction texts
    update_status_label();
}

    // This function takes a list of lights and toggles their state. It takes in a
    // list so that more than one light can be toggled in a single command
void toggle_lights(std::vector<NodePath>lights)
{
    for(auto light: lights) {
	// If the given light is in our lightAttrib, remove it.
	// This has the effect of turning off the light
	if(window->get_render().has_light(light))
	    window->get_render().clear_light(light);
	// Otherwise, add it back. This has the effect of turning the light
	// on
	else
	    window->get_render().set_light(light);
    }
    update_status_label();
}

// This function toggles the spinning of the point intervals by pausing and
// resuming the interval
void toggle_spinning_point_lights()
{
    if(are_point_lights_spinning)
	point_lights_spin->pause();
    else
	point_lights_spin->resume();
    are_point_lights_spinning = !are_point_lights_spinning;
    update_status_label();
}

// This function turns per-pixel lighting on or off.
void toggle_per_pixel_lighting()
{
    if(per_pixel_enabled) {
	per_pixel_enabled = false;
	window->get_render().clear_shader();
    } else {
	per_pixel_enabled = true;
	window->get_render().set_shader_auto();
    }
    update_status_label();
}

// This function turns shadows on or off.
void toggle_shadows()
{
    if(shadows_enabled) {
	shadows_enabled = false;
	directional_light_node->set_shadow_caster(false);
    } else {
	if(!per_pixel_enabled)
	    toggle_per_pixel_lighting();
	shadows_enabled = true;
	directional_light_node->set_shadow_caster(true, 512, 512);
    }
    update_status_label();
}

// This function changes the spotlight's exponent. It is kept to the range
// 0 to 128. Going outside of this range causes an error
void adjust_spotlight_exponent(Spotlight *light, int amount)
{
    auto e = clamp((int)(light->get_exponent()) + amount, 0, 128);
    light->set_exponent(e);
    update_status_label();
}

// defined below
template<class T> T rgb_to_hsv(T rgb);
template<class T> T hsv_to_rgb(T rgb);

// This function reads the color of the light, uses a built-in python function
//(from the library colorsys) to convert from RGB (red, green, blue) color
// representation to HSB (hue, saturation, brightness), so that we can get the
// brighteness of a light, change it, and then convert it back to rgb to chagne
// the light's color
void add_brightness(Light *light, PN_stdfloat amount)
{
    auto color = light->get_color();
    auto hsv = rgb_to_hsv(LPoint3(color[0], color[1], color[2]));
    hsv[2] = clamp(hsv[2] + amount);
    auto rgb = hsv_to_rgb(hsv);
    light->set_color(rgb);

    update_status_label();
}

void update_label(TextNode *, const char *, bool);
std::string get_brightness_string(Light *light);

// Builds the onscreen instruction labels
void update_status_label()
{
    update_label(ambient_text, "(a) ambient is",
		 window->get_render().has_light(ambient_light));
    update_label(directional_text, "(d) directional is",
		 window->get_render().has_light(directional_light));
    update_label(spotlight_text, "(s) spotlight is",
		 window->get_render().has_light(spotlight));
    update_label(point_light_text, "(p) point lights are",
		 window->get_render().has_light(red_point_light));
    update_label(spinning_text, "(r) point light spinning is",
		 are_point_lights_spinning);
    ambient_brightness_text->set_text(
        "(z,x) Ambient Brightness: " +
        get_brightness_string(ambient_light_node));
    directional_brightness_text->set_text(
            "(c,v) Directional Brightness: " +
            get_brightness_string(directional_light_node));
    spotlight_brightness_text->set_text(
        "(b,n) spotlight Brightness: " +
        get_brightness_string(spotlight_node));
    spotlight_exponent_text->set_text(
        "(q,w) spotlight Exponent: " +
        std::to_string((int)(spotlight_node->get_exponent())));
    update_label(lighting_per_pixel_text, "(l) Per-pixel lighting is",
		 per_pixel_enabled);
    update_label(lighting_shadows_text, "(e) Shadows are",
		 shadows_enabled);
}

// Appends either (on) or (off) to the base string based on the bassed value
void update_label(TextNode *obj, const char *base, bool var)
{
    const char *s = var ? " (on)" : " (off)";
    obj->set_text(std::string(base) + s);
}

// Returns the brightness of a light as a string to put it in the instruction
// labels
std::string get_brightness_string(Light *light)
{
    auto color = light->get_color();
    auto hsv = rgb_to_hsv(LPoint3(color[0], color[1], color[2]));
    std::ostringstream ret;
    ret << std::fixed << std::setprecision(2) << hsv[2];
    return ret.str();
}

// Missing functions in standard C++ and panda3d:
// works with 3-value real arrays, such as LPoint3f and LColor
// rgb is assumed 0..1
// resulting H=0..2π S=0..1 V=0..1
// formula courtesy of https://www.rapidtables.com/convert/color/rgb-to-hsv.html
template<class T> T rgb_to_hsv(T rgb)
{
    T hsv;
    hsv[2] = std::max({rgb[0], rgb[1], rgb[2]}); // V
    if(!hsv[2]) {
	hsv[0] = hsv[1] = 0;
	return hsv;
    } else {
	hsv[1] = hsv[2] - std::min({rgb[0], rgb[1], rgb[2]}); // Δ
	if(!hsv[1]) {
	    hsv[0] = 0;
	    return hsv;
	}
	if(hsv[2] == rgb[0])
	    hsv[0] = (rgb[1] - rgb[2]) / hsv[1];
	else if(hsv[2] == rgb[1])
	    hsv[0] = (rgb[2] - rgb[0]) / hsv[1] + 2;
	else
	    hsv[0] = (rgb[0] - rgb[1]) / hsv[1] + 4;
	// clamp h to 0..2π
	// since I haven't multipled by π/3 yet, clamp to 0..6
	hsv[0] = fmod(hsv[0], 6);
	if(hsv[0] < 0)
	    hsv[0] += 6;
	hsv[0] *= M_PI / 3; // H, in radians
	hsv[1] /= hsv[2]; // S
    }
    return hsv;
}

// Inverse of the above conversion
// Derived from the above; I didn't use the site for this one.
template<class T> T hsv_to_rgb(T hsv)
{
    T rgb, tmp;
    if(!hsv[1]) { // S == 0: R = G = B = V
	rgb[0] = rgb[1] = rgb[2] = hsv[2];
	return rgb;
    }
    // Normalize H to [-1,5)
    tmp[0] = fmod(hsv[0] / (M_PI / 3), 6);
    if(tmp[0] < 0)
	tmp[0] += 6;
    if(tmp[0] >= 5)
	tmp[0] -= 6;
    // Normalize H to [-1,1) and compute which colors fit what role
    int colmax, cola, colb; // H=(a-b)/Δ
    if(tmp[0] < 1) {
	colmax = 0;
	cola = 1;
	colb = 2;
    } else if(tmp[0] < 3) {
	tmp[0] -= 2;
	colmax = 1;
	cola = 2;
	colb = 0;
    } else {
	tmp[0] -= 4;
	colmax = 2;
	cola = 0;
	colb = 1;
    }
    rgb[colmax] = hsv[2];
    // S = Δ/V -> Δ = SV
    // if(H<=0) -> a=V-Δ; b=a-HΔ
    // if(H>=0) -> b=V-Δ; a=b+HΔ
    tmp[1] = hsv[1] * hsv[2]; // Δ
    tmp[0] *= tmp[1]; // HΔ
    tmp[1] = hsv[2] - tmp[1]; // V-Δ
    if(tmp[0] <= 0) {
	rgb[cola] = tmp[1];
	rgb[colb] = tmp[1] - tmp[0];
    } else {
	rgb[colb] = tmp[1];
	rgb[cola] = tmp[1] + tmp[0];
    }
    return rgb;
}
}

int main(int argc, const char **argv)
{
    if(argc > 1)
	sample_path = argv[1];

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
