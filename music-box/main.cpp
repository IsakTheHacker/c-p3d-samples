/*
 *  Translation of Python music-box sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/music-box
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang, Phil Saltzman, and Elan Ruskin
 * Last Updated: 2015-03-13
 *
 * This tutorial shows how to load, play, and manipulate sounds
 * and sound intervals in a panda project.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/audio.h>
#include <panda3d/pointLight.h>
#include <panda3d/ambientLight.h>
#include <panda3d/mouseButton.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;
PT(AudioManager) music_manager, sfx_manager;
PT(AudioSound) music_box_sound;
PT(CInterval) lid_open_sfx, lid_close_sfx;
double music_time;
bool box_open;
NodePath Lid, Panda, HingeNode;
PT(CInterval) lid_open, lid_close, panda_turn;

void set_music_box_volume(const Event *, void *);
void toggle_music_box(const Event *, void *);
void set_pad(PGItem *item, PN_stdfloat x, PN_stdfloat y);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();
    // The C++ framework doesn't have a global, automaticaly-run interval
    // pool.  The following line uses a support function I wrote to spawn
    // an asynchronous task to do the updates.
    init_interval();

    //Set the window title and open new window
    framework.set_window_title("Music Box - C++ Panda3D Samples");
    window = framework.open_window();

    // Our standard title and instructions text
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    TextNode *text_node = new TextNode("title");
    auto title = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Music Box");
    title.reparent_to(window->get_aspect_2d()); // a2d
    title.set_pos(0, 0, -1+0.08); // BottomCenter == (0,0,-1)
    title.set_scale(0.08);
    text_node->set_align(TextNode::A_center);
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, .5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText

    text_node = new TextNode("instructions");
    title = NodePath(text_node);
    text_node->set_text("ESC: Quit");
    title.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_text_color(1, 1, 1, 1);
    title.set_pos(-1.0 + 0.06, 0, 1.0 - 0.1); // TopLeft == (-1,0,1)
    text_node->set_align(TextNode::A_left);
    title.set_scale(0.05);

    // Set up the key input
    window->enable_keyboard();
    framework.define_key("escape", "Quit", framework.event_esc, &framework);

    // Fix the camera position
    // disable_mouse(); // unnecessary; requires manual enable

    // Sounds require an audio manager.  As per the docs, there should be
    // one AudioManager for each type of sound you use.
    music_manager = AudioManager::create_AudioManager();

    // Loading the main music box song
    // Synchronous loads can ge done with get_sound().  Asynchronous
    // loads are dune via an AudioLoadRequest object.  The latter is not
    // explained here.
    music_box_sound = music_manager->get_sound(sample_path + "music/musicbox.ogg");
    music_box_sound->set_volume(.5);  // Volume is a percentage from 0 to 1
    // 0 means loop forever, 1 (default) means
    // play once. 2 or higher means play that many times
    music_box_sound->set_loop_count(0);

    // Set up a simple light.
    auto plight = new PointLight("light");
    plight->set_color(LColor(0.7, 0.7, 0.5, 1));
    auto light_path = window->get_render().attach_new_node(plight);
    light_path.set_pos(0, 0, 20);
    window->get_render().set_light(light_path);

    auto alight = new AmbientLight("ambient");
    alight->set_color(LColor(0.3, 0.3, 0.4, 1));
    window->get_render().set_light(window->get_render().attach_new_node(alight));

    // Enable per-pixel lighting
    window->get_render().set_shader_auto();

    // Sound objects do not have a pause function, just play and stop. So we will
    // Use this variable to keep track of where the sound is at when it was stoped
    // to impliment pausing
    music_time = 0;

    // Loading the open/close effect
    // Sound effects are in a different audio manager.
    sfx_manager = AudioManager::create_AudioManager();

    auto lid_sfx = sfx_manager->get_sound(sample_path + "music/openclose.ogg");
    // The open/close file has both effects in it. Fortunatly we can use intervals
    // to easily define parts of a sound file to play
    lid_open_sfx = new SoundInterval(lid_sfx, 2, 0);
    lid_close_sfx = new SoundInterval(lid_sfx, -1, 5);

    // For this tutorial, it seemed appropriate to have on screen controls.
    // The following code creates them.
    // Note that instead of the Python-only DirectGUI, this uses PGUI, upon
    // which DirectGUI is partially based.
    // All PGUI elements must be descendents of PGTop.  The WindowFramework
    // conveniently roots aspect_2d and pixel_2d with PGTop.
    // We'll be using aspect_2d, just like with the on-screen text in the
    // other samples.  The coordinates match the Python code, in any case.
    // This is a label for a slider
    text_node = new TextNode("label");
    title = NodePath(text_node);
    text_node->set_text("Volume");
    title.reparent_to(window->get_aspect_2d());
    title.set_pos(-0.1, 0, 0.87); // def. center (0, 0, 0)
    text_node->set_align(TextNode::A_center);
    title.set_scale(0.07);
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, .5);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText

    // The slider itself.
    auto slider = new PGSliderBar;
//    slider->setup_slider(false, ....);
    auto slider_node = window->get_aspect_2d().attach_new_node(slider);
    slider_node.set_pos(-0.1, 0, 0.75); // def. center == (0,0,0)
    slider_node.set_scale(0.8);
    slider->set_value(0.50); // default range is 0..1
    slider->setup_slider(false, 2, 0.16, 0.8); // DirectGUI default size
    // In order to respond to changes, a callback needs to be added.
    // One way would be to subclass a PGSliderBarNotifier object, and
    // attach it using set_notify().  However, that's way too much
    // effort, so instead, I'll use the standard event management system.
    // The event name has a unique embedded ID, so you obtain it using
    // get_adjust_event().  This then calls set_music_box_volume when
    // adjusted.
    auto &evhnd = framework.get_event_handler();
    evhnd.add_hook(slider->get_adjust_event(), set_music_box_volume, slider);
    // A button that calls toggle_music_box when pressed
    PT(PGButton) button = new PGButton("open/close");
    auto button_node = window->get_aspect_2d().attach_new_node(button);
    button_node.set_pos(.9, 0, .75);
    button->setup("Open");
    set_pad(button, 0.2, 0.2);
    button_node.set_scale(0.1);
    evhnd.add_hook(button->get_click_event(MouseButton::one()), toggle_music_box, button);
    // A variable to represent the state of the simulation. It starts closed
    box_open = false;

    // Here we load and set up the music box. It was modeled in a complex way, so
    // setting it up will be complicated
    auto music_box = def_load_model("models/MusicBox");
    music_box.set_pos(0, 60, -9);
    music_box.reparent_to(window->get_render());
    // Just like the scene graph contains hierarchies of nodes, so can
    // models. You can get the NodePath for the node using the find
    // function, and then you can animate the model by moving its parts
    // To see the hierarchy of a model, use, the ls function
    //music_box.ls(); // prints out the entire hierarchy of the model

    // Finding pieces of the model
    Lid = music_box.find("**/lid");
    Panda = music_box.find("**/turningthing");

    // This model was made with the hinge in the wrong place
    // this is here so we have something to turn
    HingeNode = music_box.find(
        "**/box").attach_new_node("nHingeNode");
    HingeNode.set_pos(.8659, 6.5, 5.4);
    // WRT - ie with respect to. Reparents the object without changing
    // its position, size, or orientation
    Lid.wrt_reparent_to(HingeNode);
    HingeNode.set_hpr(0, 90, 0);

    // This sets up an interval to play the close sound and actually close the box
    // at the same time.
    // Note that my shortned constructor macro doesn't work with changed
    // blend type and baked-in-start flags.  Only the class name is shorter.
    auto anim = new NPAnimEx("hinge_close", 2.0, CLerpInterval::BT_ease_in_out,
			     false, false, HingeNode, NodePath());
    anim->set_end_hpr(LVector3(0, 90, 0));
    lid_close = new Parallel({ lid_close_sfx, anim });

    // Same thing for opening the box
    anim = new NPAnimEx("hinge_open", 2.0, CLerpInterval::BT_ease_in_out,
			false, false, HingeNode, NodePath());
    anim->set_end_hpr(LVector3(0, 0, 0));
    lid_open = new Parallel({ lid_open_sfx, anim });

    // The interval for turning the panda
    panda_turn = anim = new NPAnim(Panda, "panda_turn", 7);
    anim->set_end_hpr(LVector3(360, 0, 0));
    // Do a quick loop and pause to set it as a looping interval so it can be
    // started with resume and loop properly
    panda_turn->loop();
    panda_turn->pause();

    // Finally, one other thing done by the Python API that isn't done here
    // is to actually run the AudioMagager()s.  A task is added for this.
    framework.get_task_mgr().
	add(new GenericAsyncTask("audio", [](GenericAsyncTask *, void *){
	    music_manager->update();
	    sfx_manager->update();
	    return AsyncTask::DS_cont;
	}, 0));
}

void set_music_box_volume(const  Event *, void *data)
{
    auto slider = reinterpret_cast<PGSliderBar *>(data);
    music_box_sound->set_volume(slider->get_value());
}

void toggle_music_box(const Event *, void *data)
{
    //if(lid_open->is_playing() || lid_close->is_playing())
    //    // It's currently already opening or closing
    //    return;

    const char *button_text;
    auto button = reinterpret_cast<PGButton *>(data);

    if(box_open) {
	lid_open->pause();

	lid_close->start();  // Start the close box interval
	panda_turn->pause();  // Pause the figurine turning
	// Save the current time of the music
	music_time = music_box_sound->get_time();
	music_box_sound->stop();  // Stop the music
	button_text = "Open";  // Prepare to change button label
    } else {
	lid_close->pause();

	lid_open->start();  // Start the open box interval
	panda_turn->resume();  // Resume the figuring turning
	// Reset the time of the music so it starts where it left off
	music_box_sound->set_time(music_time);
	music_box_sound->play();  // Play the music
	button_text = "Close";  // Prepare to change button label
    }
    // Can't just set_text(), so must call setup().  This requires
    // re-adjusting the "pad" as well
    button->setup(button_text);
    set_pad(button, 0.2, 0.2);

    // Set our state to opposite what it was
    box_open = !box_open;
    //(closed to open or open to closed)
}

void set_pad(PGItem *item, PN_stdfloat x, PN_stdfloat y)
{
    auto bframe = item->get_frame();
    bframe[0] -= x;
    bframe[1] += x;
    bframe[2] -= y;
    bframe[3] += y;
    item->set_frame(bframe);
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
