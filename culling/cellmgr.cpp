#include <map>
#include <panda3d/portalNode.h>
#include <panda3d/collisionRay.h>
#include <panda3d/collisionTraverser.h>
#include <panda3d/collisionHandlerQueue.h>

// Support classes for portalculling.cpp.
// This is in a separate file because it needs to be at the top, but to
// match the Python code, it needs to be lexically at the bottom.  So,
// just don't mention the contents at all, and store in a separate file.

// The Cell class is a handy way to keep an association between
// all the related nodes and information of a cell.
class CellManager;
struct Cell {
    Cell(const CellManager *cellmanager, const std::string &name, NodePath collider);
    void add_portal(NodePath portal, Cell *cell_out);

    // Note: these 3 are a waste of space as of now.
    const CellManager *_cellmanager;
    const std::string _name;
    NodePath _collider;
    NodePath _nodepath;
    // Note: follwing is a waste of sapce as of now.
    std::vector<NodePath> _portals;
};

// Creates a collision ray and collision traverser to use for
// selecting the current cell.
class CellManager {
  public:
    CellManager(WindowFramework *window) :
	_window(window), _cell_picker_world("cell_picker_world"),
        _ray(new CollisionRay), _traverser("traverser")
    {
	_ray->set_direction(LVector3::down());
	auto cnode = new CollisionNode("cell_raycast_cnode");
        _ray_nodepath = _cell_picker_world.attach_new_node(cnode);
        cnode->add_solid(_ray);
        cnode->set_into_collide_mask(0); // not for colliding into
        cnode->set_from_collide_mask(1<<0);
    }

    // Add a new cell.
    void add_cell(NodePath &collider, const std::string &name) {
	auto cell = new Cell(this, name, collider);
	_cells[name] = cell;
	_cells_by_collider[collider.node()] = cell;
    }

    // Given a position, return the nearest cell below that position.
    // If no cell is found, returns None.
    Cell *get_cell(const LVector3 &pos) {
        _ray->set_origin(pos);
        PT(CollisionHandlerQueue) queue = new CollisionHandlerQueue;
        _traverser.add_collider(_ray_nodepath, queue);
        _traverser.traverse(_cell_picker_world);
        _traverser.remove_collider(_ray_nodepath);
        queue->sort_entries();
        if(!queue->get_num_entries())
            return nullptr;
        auto entry = queue->get_entry(0);
	auto cnode = entry->get_into_node();
	auto c = _cells_by_collider.find(cnode);
	if(c != _cells_by_collider.end())
	    return c->second;
	else {
	    std::cerr << "Warning: collision ray collided with something "
	                 "other than a cell: " << cnode << '\n';
	    return nullptr;
	}
    }

    // Given a position, return the distance to the nearest cell
    // below that position. If no cell is found, returns < 0.
    PN_stdfloat get_dist_to_cell(LPoint3 pos) {
        _ray->set_origin(pos);
        PT(CollisionHandlerQueue) queue = new CollisionHandlerQueue();
        _traverser.add_collider(_ray_nodepath, queue);
        _traverser.traverse(_cell_picker_world);
        _traverser.remove_collider(_ray_nodepath);
        queue->sort_entries();
        if(!queue->get_num_entries())
            return -1;
        auto entry = queue->get_entry(0);
        return (entry->get_surface_point(_cell_picker_world) - pos).length();
    }

    // Loads cells from an EGG file. Cells must be named in the
    // format "cell#" to be loaded by this function.
    void load_cells_from_model(std::string modelpath) {
        auto cell_model = _window->load_model(NodePath(), modelpath);
	auto colliders = cell_model.find_all_matches("**/+GeomNode");
	for(unsigned i = 0; i < colliders.size(); i++) {
	    auto collider = colliders[i];
            auto name = collider.get_name();
	    if(name.substr(0, 4) == "cell")
                add_cell(collider, name.substr(4));
	    cell_model.remove_node();
	}
    }

    // Loads portals from an EGG file. Portals must be named in the
    // format "portal_#to#_*" to be loaded by this function, whereby the
    // first # is the from cell, the second # is the into cell, and * can
    // be anything.
    void load_portals_from_model(std::string modelpath) {
        auto portal_model = _window->load_model(NodePath(), modelpath);
        auto portal_nodepaths = portal_model.find_all_matches("**/+PortalNode");
	for(unsigned i = 0; i < portal_nodepaths.size(); i++) {
	    auto portal_nodepath = portal_nodepaths[i];
            const auto &name = portal_nodepath.get_name();
            if(name.substr(0, 7) == "portal_") {
		auto n = name.find("to");
		auto e = name.find("_", n);
		if(n == std::string::npos)
		    continue;
		auto from_cell_id = name.substr(7, n - 7);
		auto into_cell_id = name.substr(n + 2, e - n - 2);
		auto from_cell = _cells.find(from_cell_id);
		if(from_cell == _cells.end()) {
		    std::cerr << "could not load portal \"" << name <<
			         "\" because cell \"" << from_cell_id <<
			         "\" does not exist";
		    continue;
		}
		auto into_cell = _cells.find(into_cell_id);
		if(into_cell == _cells.end()) {
		    std::cerr << "could not load portal \"" << name <<
			         "\" because cell \"" << into_cell_id <<
			         "\" does not exist";
		    continue;
		}
                from_cell->second->add_portal(portal_nodepath, into_cell->second);
	    }
	}
        portal_model.remove_node();
    }

    // Show the cell the camera is currently in and hides the rest.
    // If the camera is not in a cell, use the last known cell that the
    // camera was in. If the camera has not yet been in a cell, then all
    // cells will be hidden.
    void update() {
	auto camera = _window->get_camera_group();
        auto camera_pos = camera.get_pos(_window->get_render());
	for(auto cell : _cells)
            cell.second->_nodepath.hide();
        auto current_cell = get_cell(camera_pos);
        if(!current_cell) {
            if(!_last_known_cell)
                return;
            _last_known_cell->_nodepath.show();
	} else {
            _last_known_cell = current_cell;
            current_cell->_nodepath.show();
	}
    }

    WindowFramework *_window;
    std::map<std::string, Cell *> _cells;
    std::map<PandaNode *, Cell *> _cells_by_collider;
    NodePath _cell_picker_world, _ray_nodepath;
    PT(CollisionRay) _ray;
    CollisionTraverser _traverser;
    Cell *_last_known_cell;
};

Cell::Cell(const CellManager *cellmanager, const std::string &name,
	   NodePath collider) :
      _cellmanager(cellmanager), _name(name), _collider(collider)
{
    collider.reparent_to(cellmanager->_cell_picker_world);
    collider.set_collide_mask(1<<0);
    collider.hide();
    _nodepath = NodePath("cell_" + name + "_root");
    _nodepath.reparent_to(cellmanager->_window->get_render());
}

// Add a portal from this cell going into another one.
void Cell::add_portal(NodePath portal, Cell *cell_out)
{
    portal.reparent_to(_nodepath);
    auto pn = DCAST(PortalNode, portal.node());
    pn->set_cell_in(_nodepath);
    pn->set_cell_out(cell_out->_nodepath);
    _portals.push_back(portal);
}
