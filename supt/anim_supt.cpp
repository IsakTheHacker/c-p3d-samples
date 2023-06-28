#include "anim_supt.h"
#include <cIntervalManager.h>
#include <genericAsyncTask.h>
#include <asyncTaskManager.h>
#include <character.h>
#include <partBundle.h>
#include <loader.h>

// FIXME: sleep until there's something to do
void init_interval(void)
{
    AsyncTaskManager::get_global_ptr()->
	add(new GenericAsyncTask("cInterval", [](GenericAsyncTask *, void *){
	    CIntervalManager::get_global_ptr()->step();
	    return AsyncTask::DS_cont;
    }, 0));
}

// Since there a lot of animations to load, this convenience function
// saves a lot of typing.
PT(AnimControl) load_anim(NodePath &model, const std::string &file)
{
    // The first step is to locate the Character node to bind with.
    // In the samples, it's always the first child
    auto char_node = dynamic_cast<Character *>(model.node()->get_child(0));
    // But, to be safe, you can search by type.  The name isn't passed in here,
    // so that's not possible.
#if 0
    auto char_node = dynamic_cast<Character *>
	(model.find("<name>").node()); // you have to know this.
    auto char_node = dynamic_cast<Character *>
	(model.find("-Character").node());
#endif
    // These flags (with obnoxiously long names) are needed to bind the
    // sample animations.  This bypasses hierarchy match integrity checks.
    // The first fails because it stupidly expects the animation name to
    // match the model name.
    // The second fails because there are some joints in the animation
    // with no corresponding joint in the model.
    int match_flags = PartGroup::HMF_ok_wrong_root_name | // the first killer
                      PartGroup::HMF_ok_anim_extra; // the silent killer

#if 1
    // There is a convenient combination function, load_bind_anim(), to
    // do all the work:
    static Loader loader; // any loader will do; window's supports vfs
    return char_node->get_bundle(0)->
	load_bind_anim(&loader, file, match_flags, PartSubset(), false);     
#else
    // Since the window's load_model() has additional functionality, it's
    // probably better to always use it, though.
    auto mod = def_load_model(file);
    auto anim = AnimBundleNode::find_anim_bundle(mod);
    auto ret = char_node->get_bundle(0)->bind_anim(anim, match_flags);
    ret->set_anim_model(mod);
    return ret;
#endif
}

// This method requires partBundle.h, which anim_supt.h otherwise does not
// require, so I moved it here.
void CharAnimate::init(AnimControl *ctrl, double rate, double start, double end)
{
    _ctrl = ctrl;
    auto bundle = ctrl->get_anim();
    if(end > bundle->get_num_frames() || end < 0)
	end = bundle->get_num_frames() - 1;
    if(start < 0)
	start = 0;
    _end_t = _duration = (end - start + 1) / rate / bundle->get_base_frame_rate();
    set_play_rate(rate);
    _start = start;
    _end = end;
}

void CharAnimate::priv_initialize(double t)
{
    _ctrl->get_part()->set_control_effect(_ctrl, 1);
    CInterval::priv_initialize(t);
}

void CharAnimate::priv_finalize(void)
{
    _ctrl->get_part()->set_control_effect(_ctrl, 0);
    CInterval::priv_finalize();
}


void FuncAsyncI::priv_instant()
{
    AsyncTaskManager::get_global_ptr()->
	add(new GenericAsyncTask("AsyncInterval", [](GenericAsyncTask *, void *v) {
	    reinterpret_cast<decltype(this)>(v)->_f();
	    return AsyncTask::DS_done;
	}, this));
}
