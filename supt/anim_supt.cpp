#include "anim_supt.h"
#include <cIntervalManager.h>
#include <genericAsyncTask.h>
#include <asyncTaskManager.h>

// One way to find animations is to look for them by name.
// Another is to look for them by type, usually to enumerate the
// availabe animations.
#if 0
    const auto t = AnimBundleNode::get_class_type();
    auto cur = find_node_type(node.node(), t);
    while(cur) {
	std::cout << "Found anim " << cur->get_name() << '\n';
	cur = find_node_type(node.node(), t, cur);
    }
#endif

PandaNode *find_node_type(const PandaNode *root, const TypeHandle &t,
                          const PandaNode *from)
{
    if(from && from == root)
	return nullptr;
    while(from) {
	auto parent = from->get_parent(0);
	const auto &c = parent->get_children();
	unsigned i;
	for(i = 0; i < c.get_num_children(); i++)
	    if(c[i] == from)
		break;
	for(++i; i < c.get_num_children(); i++) {
	    auto ret = find_node_type(c[i], t);
	    if(ret)
		return ret;
	}
	if(parent == root)
	    return nullptr;
	from = parent;
    }
    if(root->get_type() == t)
	return const_cast<PandaNode *>(root);
    const auto &c = root->get_children();
    for(unsigned i = 0; i < c.get_num_children(); i++) {
	auto ret = find_node_type(c[i], t);
	if(ret)
	    return ret;
    }
    return nullptr;
}

// FIXME: sleep until there's something to do
void init_interval(void)
{
    AsyncTaskManager::get_global_ptr()->
	add(new GenericAsyncTask("cInterval", [](GenericAsyncTask *, void *){
	    CIntervalManager::get_global_ptr()->step();
	    return AsyncTask::DS_cont;
    }, 0));
}
