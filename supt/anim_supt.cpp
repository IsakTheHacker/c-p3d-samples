#include <panda3d/character.h>
#include <panda3d/loader.h>
#include "sample_supt.h"

// Since there a lot of animations to load, this convenience function
// saves a lot of typing.
PT(AnimControl) load_anim(NodePath &model, const std::string &file)
{
    // The first step is to locate the Character node to bind with.
    // In the samples, it's always the first child
    auto char_node = DCAST(Character, model.node()->get_child(0));
    // But, to be safe, you can search by type.  The name isn't passed in here,
    // so that's not possible.
#if 0
    auto char_node = DCAST(Character, model.find("<name>").node()); // you have to know this.
    auto char_node = DCAST(Character, model.find("-Character").node());
#endif
    // There is a convenient combination function, load_bind_anim(), to
    // do all the work:
    return char_node->get_bundle(0)->
	load_bind_anim(Loader::get_global_ptr(), file, ANIM_BIND_FLAGS, PartSubset(), false);
}
