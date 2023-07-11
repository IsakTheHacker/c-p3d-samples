#include <panda3d/character.h>
#include <panda3d/partBundle.h>
#include <panda3d/loader.h>
#include "sample_supt.h"

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
#if 1
    // There is a convenient combination function, load_bind_anim(), to
    // do all the work:
    static Loader loader; // any loader will do; window's supports vfs
    return char_node->get_bundle(0)->
	load_bind_anim(&loader, file, ANIM_BIND_FLAGS, PartSubset(), false);     
#else
    // Since the window's load_model() has additional functionality, it's
    // probably better to always use it, though.
    auto mod = def_load_model(file);
    auto anim = AnimBundleNode::find_anim_bundle(mod);
    auto ret = char_node->get_bundle(0)->bind_anim(anim, ANIM_BIND_FLAGS);
    ret->set_anim_model(mod);
    return ret;
#endif
}
