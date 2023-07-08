/*
 *  Translation of Python infinite-tunnel sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/infinite-tunnel
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang and Phil Saltzman
 * Last Updated: 2015-03-13
 *
 * This tutorial will cover fog and how it can be used to make a finite length
 * tunnel seem endless by hiding its endpoint in darkness. Fog in panda works by
 * coloring objects based on their distance from the camera. Fog is not a 3D
 * volume object like real world fog.
 * With the right settings, Fog in panda can mimic the appearence of real world
 * fog.
 *
 * The tunnel and texture was originally created by Vamsi Bandaru and Victoria
 * Webb for the Entertainment Technology Center class Building Virtual Worlds
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/texturePool.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables for the tunnel dimensions and speed of travel
#define TUNNEL_SEGMENT_LENGTH 50
#define TUNNEL_TIME 2  /* Amount of time for one segment to travel the
                          distance of TUNNEL_SEGMENT_LENGTH */

// Global variables
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;
PT(Fog) fog;
NodePath tunnel[4];
PT(CInterval) tunnel_move;

// Macro-like function used to reduce the amount to code needed to create the
// on screen instructions
void gen_label_text(int i, const char *text)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode(text);
    auto path = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    path.set_scale(0.05);
    path.set_pos(-1.0/a2d.get_sx() + 0.06, 0, 1.0 - 0.065 * i); // TopLeft == (-1,0,1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_left);
}

void set_background_color(PN_stdfloat r, PN_stdfloat g, PN_stdfloat b);
void add_fog_density(PN_stdfloat), toggle_interval(CInterval *);
void toggle_fog(NodePath node, Fog *fog);
void init_tunnel(void), cont_tunnel(void);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    update_intervals();

    framework.set_window_title("Infinite Tunnel - C++ Panda3D Samples");
    window = framework.open_window();

    // Standard initialization stuff
    // Standard title that's on screen in every tutorial
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Fog");
    // style = 1 -> plain (default)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 0.5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text_node->set_align(TextNode::A_right);
    text.set_pos(1.0/a2d.get_sx()-0.1, 0, -1+0.1); // BottomRight == (1,0,-1)
    text.set_scale(0.08);

    // Code to generate the on screen instructions
    gen_label_text(1, "ESC: Quit");
    gen_label_text(2, "[P]: Pause");
    gen_label_text(3, "[T]: Toggle Fog");
    gen_label_text(4, "[D]: Make fog color black");
    gen_label_text(5, "[SHIFT+D]: Make background color black");
    gen_label_text(6, "[R]: Make fog color red");
    gen_label_text(7, "[SHIFT+R]: Make background color red");
    gen_label_text(8, "[B]: Make fog color blue");
    gen_label_text(9, "[SHIFT+B]: Make background color blue");
    gen_label_text(10, "[G]: Make fog color green");
    gen_label_text(11, "[SHIFT+G]: Make background color green");
    gen_label_text(12, "[L]: Make fog color light grey");
    gen_label_text(13, "[SHIFT+L]: Make background color light grey");
    gen_label_text(14, "[+]: Increase fog density");
    gen_label_text(15, "[-]: Decrease fog density");

    // place the camera
    window->get_camera_group().set_pos_hpr(0, 0, 10, 0, -90, 0);
    // set the background color to black
    window->set_background_type(WindowFramework::BT_black);

    // World specific-code

    // Create an instance of fog called 'distance_fog'.
    //'distance_fog' is just a name for our fog, not a specific type of fog.
    fog = new Fog("distance_fog");
    // Set the initial color of our fog to black.
    fog->set_color(0, 0, 0);
    // Set the density/falloff of the fog.  The range is 0-1.
    // The higher the numer, the "bigger" the fog effect.
    fog->set_exp_density(.08);
    // We will set fog on render which means that everything in our scene will
    // be affected by fog. Alternatively, you could only set fog on a specific
    // object/node and only it and the nodes below it would be affected by
    // the fog.
    window->get_render().set_fog(fog);

    // Define the keyboard input
    window->enable_keyboard();
    // Escape closes the demo
    framework.define_key("escape", "", &framework.event_esc, &framework);
    // Handle pausing the tunnel
    framework.define_key("p", "", EV_FN() { toggle_interval(tunnel_move); }, 0);
    // Handle turning the fog on and off
    framework.define_key("t", "", EV_FN() { toggle_fog(window->get_render(), fog); }, 0);
    // Sets keys to set the fog to various colors
    framework.define_key("r", "", EV_FN() { fog->set_color(1, 0, 0); }, 0);
    framework.define_key("g", "", EV_FN() { fog->set_color(0, 1, 0); }, 0);
    framework.define_key("b", "", EV_FN() { fog->set_color(0, 0, 1); }, 0);
    framework.define_key("l", "", EV_FN() { fog->set_color(.7, .7, .7); }, 0);
    framework.define_key("d", "", EV_FN() { fog->set_color(0, 0, 0); }, 0);
    // Sets keys to change the background colors
    framework.define_key("shift-r", "", EV_FN() { set_background_color(1, 0, 0); }, 0);
    framework.define_key("shift-g", "", EV_FN() { set_background_color(0, 1, 0); }, 0);
    framework.define_key("shift-b", "", EV_FN() { set_background_color(0, 0, 1); }, 0);
    framework.define_key("shift-l", "", EV_FN() { set_background_color(.7, .7, .7); }, 0);
    framework.define_key("shift-d", "", EV_FN() { set_background_color(0, 0, 0); }, 0);
    // Increases the fog density when "+" key is pressed
    framework.define_key("+", "", EV_FN() { add_fog_density(.01); }, 0);
    // This is to handle the other "+" key (it's over = on the keyboard)
    framework.define_key("=", "", EV_FN() { add_fog_density(.01); }, 0);
    framework.define_key("shift-=", "", EV_FN() {  add_fog_density(.01); }, 0);
    // Decreases the fog density when the "-" key is pressed
    framework.define_key("-", "", EV_FN() { add_fog_density(-.01); }, 0);

    // Load the tunel and start the tunnel
    init_tunnel();
    cont_tunnel();
}

// replacement for showbase set_background_color
void set_background_color(PN_stdfloat r, PN_stdfloat g, PN_stdfloat b)
{
    auto bgctrl = window->get_display_region_3d();
    bgctrl->set_clear_color(LColor(r, g, b, 1));
    bgctrl->set_clear_color_active(true);
}

// This function will change the fog density by the amount passed into it
// Unlike Python, it is not necessary, but it is convenient/less verbose above.
void add_fog_density(PN_stdfloat change)
{
    // The min() statement makes sure the density is never over 1
    // The max() statement makes sure the density is never below 0
    fog->set_exp_density(
	std::min((PN_stdfloat)1, std::max((PN_stdfloat)0, fog->get_exp_density() + change)));
}

// Code to initialize the tunnel
void init_tunnel()
{
    for(unsigned x = 0; x < 4; x++) {
	// Load a copy of the tunnel
	tunnel[x] = def_load_model("models/tunnel");
	// The front segment needs to be attached to render
	if(x == 0)
	    tunnel[x].reparent_to(window->get_render());
	else
            // The rest of the segments parent to the previous one, so that by moving
            // the front segement, the entire tunnel is moved
	    tunnel[x].reparent_to(tunnel[x - 1]);
	// We have to offset each segment by its length so that they stack onto
	// each other. Otherwise, they would all occupy the same space.
	tunnel[x].set_pos(0, 0, -TUNNEL_SEGMENT_LENGTH);
    }
    // Now we have a tunnel consisting of 4 repeating segments with a
    // hierarchy like this:
    // render<-tunnel[0]<-tunnel[1]<-tunnel[2]<-tunnel[3]
}

// This function is called to snap the front of the tunnel to the back
// to simulate traveling through it
void cont_tunnel()
{
    // This takes the front of the list and put it on the back.
    for(unsigned i = 0; i < 3; i++) {
	NodePath t(tunnel[i]);
	tunnel[i] = tunnel[i+1];
	tunnel[i+1] = t;
    }

    // Set the front segment (which was at TUNNEL_SEGMENT_LENGTH) to 0, which
    // is where the previous segment started
    tunnel[0].set_z(0);
    // Reparent the front to render to preserve the hierarchy outlined above
    tunnel[0].reparent_to(window->get_render());
    // Set the scale to be apropriate (since attributes like scale are
    // inherited, the rest of the segments have a scale of 1)
    tunnel[0].set_scale(.155, .155, .305);
    // Set the new back to the values that the rest of teh segments have
    tunnel[3].reparent_to(tunnel[2]);
    tunnel[3].set_z(-TUNNEL_SEGMENT_LENGTH);
    tunnel[3].set_scale(1);

    // Set up the tunnel to move one segment and then call cont_tunnel again
    // to make the tunnel move infinitely
    tunnel_move = new Sequence({
	new LerpFunc(&tunnel[0], &NodePath::set_z,
		     (PN_stdfloat)0, (PN_stdfloat)(TUNNEL_SEGMENT_LENGTH * .305),
                     TUNNEL_TIME),
	new FuncAsync(cont_tunnel())
    });
    tunnel_move->start();
}

// This function will toggle any interval passed to it between playing and
// paused
void toggle_interval(CInterval *interval)
{
    if(interval->is_playing())
        interval->pause();
    else
        interval->resume();
}

// This function will toggle fog on a node
void toggle_fog(NodePath node, Fog *fog)
{
    // If the fog attached to the node is equal to the one we passed in, then
    // fog is on and we should clear it
    if(node.get_fog() == fog)
        node.clear_fog();
    // Otherwise fog is not set so we should set it
    else
        node.set_fog(fog);
}
}

int main(int argc, char **argv)
{
    if(argc > 1)
	sample_path = argv[1];
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    kill_intervals();
    framework.close_framework();
    return 0;
}

