/*
 *  Translation of Python media-player sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/media-player
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, if present.
 */
#include <pandaFramework.h>
#include <movieTexture.h>
#include <cardMaker.h>
#include <audioManager.h>

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
PT(AudioManager) sfx_manager;
PT(AudioSound) sound;

// Function to put instructions on the screen.
void add_instructions(PN_stdfloat pos, const char *msg)
{
    TextNode *text_node = new TextNode(msg);
    auto path = NodePath(text_node);
    text_node->set_text(msg);
    // style=1 -> default
    text_node->set_text_color(0, 0, 0, 1);
    text_node->set_shadow_color(1, 1, 1, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    path.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_left);
    path.set_pos(-1.0 + 0.08, 0, 1.0 - pos - 0.04); // TopLeft == (-1,0,1)
    path.set_scale(0.06);
}

// Function to put title on the screen.
void add_title(const char *msg)
{
    TextNode *text_node = new TextNode(msg);
    auto path = NodePath(text_node);
    text_node->set_text(msg);
    // style=1 -> default
    path.set_pos(1.0 - 0.1, 0, -1.0 + 0.09); // BottomRight == (1,0,-1)
    path.set_scale(0.08);
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0, 0, 0, 1);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    path.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
}

void playpause(void), stopsound(void), fastforward(void);

void init(const char *media_file)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();

    //Set the window title and open new window
    framework.set_window_title("Media Player - C++ Panda3D Samples");
    window = framework.open_window();

    window->enable_keyboard();
    add_title("Panda3D: Tutorial - Media Player");
    add_instructions(0.06, "P: Play/Pause");
    add_instructions(0.12, "S: Stop and Rewind");
    add_instructions(0.18, "M: Slow Motion / Normal Motion toggle");

    // Load the texture. We could use loader.load_texture for this,
    // but we want to make sure we get a MovieTexture, since it
    // implements synchronize_to.
    PT(MovieTexture) tex = new MovieTexture("name");
    if(!tex->read(sample_path + media_file)) {
	std::cerr << "Failed to load video!";
	exit(1);
    }

    // Set up a fullscreen card to set the video texture on.
    CardMaker cm("My Fullscreen Card");
    cm.set_frame_fullscreen_quad();

    // Tell the CardMaker to create texture coordinates that take into
    // account the padding region of the texture.
    cm.set_uv_range(tex);

    // Now place the card in the scene graph and apply the texture to it.
    auto card = NodePath(cm.generate());
    card.reparent_to(window->get_render_2d());
    card.set_texture(tex);

    sfx_manager = AudioManager::create_AudioManager();
    sound = sfx_manager->get_sound(sample_path + media_file);
    // Synchronize the video to the sound.
    tex->synchronize_to(sound);

    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("p", "", EV_FN() { playpause(); }, 0);
    framework.define_key("P", "", EV_FN() { playpause(); }, 0);
    framework.define_key("s", "", EV_FN() { stopsound(); }, 0);
    framework.define_key("S", "", EV_FN() { stopsound(); }, 0);
    framework.define_key("m", "", EV_FN() { fastforward(); }, 0);
    framework.define_key("M", "", EV_FN() { fastforward(); }, 0);

    framework.get_task_mgr().
	add(new GenericAsyncTask("audio", [](GenericAsyncTask *, void *){
	    sfx_manager->update();
	    return AsyncTask::DS_cont;
	}, 0));
}

void stopsound()
{
    sound->stop();
    sound->set_play_rate(1.0);
}

void fastforward()
{
    if(sound->status() == AudioSound::PLAYING) {
	auto t = sound->get_time();
	sound->stop();
	if(sound->get_play_rate() == 1.0)
	    sound->set_play_rate(0.5);
	else
	    sound->set_play_rate(1.0);
	sound->set_time(t);
	sound->play();
    }
}

void playpause()
{
    if(sound->status() == AudioSound::PLAYING) {
	auto t = sound->get_time();
	sound->stop();
	sound->set_time(t);
    } else
	sound->play();
}
}

int main(int argc, char **argv)
{
    // Tell Panda3D to use OpenAL, not FMOD
    audio_library_name = "p3openal_audio";
    if(argc > 1)
	sample_path = argv[1];
    init("PandaSneezes.ogv");
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
