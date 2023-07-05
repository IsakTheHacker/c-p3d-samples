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

#include "anim_supt.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_supt.h"
using namespace ImGui;

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
const char *button_text;

void toggle_music_box(void);

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
    framework.define_key("escape", "Quit", framework.event_esc, &framework);

    // Fix the camera position
    // disable_mouse(); // unnecessary; requires manual enable

    // Sounds require an audio manager.  As per the docs, there should be
    // one AudioManager for each type of sound you use.
    music_manager = AudioManager::create_AudioManager();
    music_manager->local_object();

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
    sfx_manager->local_object();

    auto lid_sfx = sfx_manager->get_sound(sample_path + "music/openclose.ogg");
    // The open/close file has both effects in it. Fortunatly we can use intervals
    // to easily define parts of a sound file to play
    lid_open_sfx = new SoundInterval(lid_sfx, 2, 0);
    lid_close_sfx = new SoundInterval(lid_sfx, -1, 5);

    // For this tutorial, it seemed appropriate to have on screen controls.
    // The following code creates them.
    // Since the "direct" GUI is Python-only, this uses Dear ImGui.
    setup_imgui(window);
    GetIO().Fonts->Clear(); // replace the default font
    static PN_stdfloat orig_ys;
    orig_ys = window->get_pixel_2d().get_sz();
    // GUI was probably designed for height=600
    auto load_fscale = 600 / (2 / orig_ys);
    static float fheight;
    fheight = load_font("Sans-20", load_fscale)->FontSize / load_fscale;
    GetIO().IniFilename = NULL;
    imgui_draw({
	// pixel2d is a child of render2d, so the scale factors are the
	// actual # of pixels / 2 (due to -1..1)
	auto scale = window->get_pixel_2d().get_scale();
	ImVec2 sz(1 / scale.get_x(), 1 / scale.get_z());
	auto fscale = orig_ys / scale.get_z() * 300 / fheight;
	SetNextWindowPos(ImVec2(0, 0));
	SetNextWindowSize(sz * 2);
	sz.y = -sz.y; // for coordinates, invert y
	auto d2i = [&](float x, float y) { return (ImVec2(x, y) + ImVec2(1, -1)) * sz; };
	if(Begin("mainwin", NULL, ImGuiWindowFlags_NoDecoration |
		                  ImGuiWindowFlags_AlwaysAutoResize |
		                  ImGuiWindowFlags_NoBackground)) {
	    // Original coordinates: -0.1, .75.   Too far to the right.
	    // Presumably it's being centered, but centering is hard
	    SetCursorScreenPos(d2i(-0.7, 0.75));
	    // scale=.07
	    GetIO().FontGlobalScale = 0.07 * fscale;
	    float vol = music_box_sound->get_volume();
	    if(SliderFloat(" ", &vol, 0, 1, "Volume %.2f",
			   ImGuiSliderFlags_AlwaysClamp))
		music_box_sound->set_volume(vol);
	    // Again, original (0.9,0.75) is too far to the right.
	    SetCursorScreenPos(d2i(0.65, 0.75));
	    GetIO().FontGlobalScale = 
	    // scale=.1, pad=(.2, .2)
	    GetIO().FontGlobalScale = 0.1 * fscale;
	    if(Button(button_text))
		toggle_music_box();
	}
	End();
    });

    // A variable to represent the state of the simulation. It starts closed
    box_open = false;
    button_text = "Open";

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

void toggle_music_box()
{
    //if(lid_open->is_playing() || lid_close->is_playing())
    //    // It's currently already opening or closing
    //    return;

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

    // Set our state to opposite what it was
    box_open = !box_open;
    //(closed to open or open to closed)
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
