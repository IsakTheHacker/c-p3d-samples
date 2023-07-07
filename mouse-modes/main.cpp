/*
 *  Translation of Python mouse-modes sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/mouse-modes
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Demonstrate different mouse modes
 */

#include <panda3d/pandaFramework.h>
#include <panda3d/mouseWatcher.h>
#include <panda3d/load_prc_file.h>
#include <sstream>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
NodePath model;
TextNode *mouse_text, *delta_text, *position_text;
int last_mouse_x, last_mouse_y;
bool hide_mouse, manual_recenter_mouse;
PN_stdfloat mouse_magnitude, rotate_x, rotate_y;
WindowProperties::MouseMode mouse_mode;

TextNode *gen_label_text(const char *text, int i)
{
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode(text);
    auto path = a2d.attach_new_node(text_node);
    text_node->set_text(text);
    path.set_pos(-1.3, 0, .5-.05*i); // default center (0, 0, 0)
    text_node->set_text_color(0, 1, 0, 1);
    text_node->set_align(TextNode::A_left);
    path.set_scale(0.05);
    return text_node;
}

void set_mouse_mode(WindowProperties::MouseMode mode);
void recenter_mouse(void), toggle_recenter(void), toggle_mouse(void);
AsyncTask::DoneStatus mouse_task(GenericAsyncTask *task, void *data);

void init(void)
{
    // Notify::ptr()->get_category(":x11display")->set_severity(NS_debug);
    // Notify::ptr()->get_category(":windisplay")->set_severity(NS_debug);
    //
    // load_prc_file_data("", "load-display p3tinydisplay");

    framework.open_framework();
    framework.set_window_title("Mouse Modes - C++ Panda3D Samples");
    window = framework.open_window();

    // control mapping of mouse movement to box movement
    mouse_magnitude = 1;

    rotate_x = rotate_y = 0;

    gen_label_text("[0] Absolute mode, [1] Relative mode, [2] Confined mode", 0);

    framework.define_key("0", "", EV_FN() { set_mouse_mode(WindowProperties::M_absolute); }, 0);
    framework.define_key("1", "", EV_FN() { set_mouse_mode(WindowProperties::M_relative); }, 0);
    framework.define_key("2", "", EV_FN() { set_mouse_mode(WindowProperties::M_confined); }, 0);

    gen_label_text("[C] Manually re-center mouse on each tick", 1);
    framework.define_key("C", "", EV_FN() { toggle_recenter(); }, 0);
    framework.define_key("c", "", EV_FN() { toggle_recenter(); }, 0);

    gen_label_text("[S] Show mouse", 2);
    framework.define_key("S", "", EV_FN() { toggle_mouse(); }, 0);
    framework.define_key("s", "", EV_FN() { toggle_mouse(); }, 0);

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);

    mouse_text = gen_label_text("", 5);
    delta_text = gen_label_text("", 6);
    position_text = gen_label_text("", 8);

    last_mouse_x = last_mouse_y = INT_MAX;

    hide_mouse = false;

    set_mouse_mode(WindowProperties::M_absolute);
    manual_recenter_mouse = true;

    // make a box to move with the mouse
    model = window->load_model(window->get_render(), "box");

    auto cam = window->get_camera_group();
    cam.set_pos(0, -5, 0);
    cam.look_at(0, 0, 0);

    framework.get_task_mgr().add(new GenericAsyncTask("Mouse Task", mouse_task, 0));
}

AsyncTask::DoneStatus resolve_mouse(GenericAsyncTask *task, void *data);

void set_mouse_mode(WindowProperties::MouseMode mode)
{
    std::cout << "Changing mode to " << mode << '\n';

    mouse_mode = mode;

    WindowProperties wp;
    wp.set_mouse_mode(mode);
    window->get_graphics_window()->request_properties(wp);

    // these changes may require a tick to apply
    auto task = new GenericAsyncTask("Resolve mouse setting", resolve_mouse, 0);
    task->set_delay(0.0); // not sure if this forces a delay
    framework.get_task_mgr().add(task);
}

AsyncTask::DoneStatus resolve_mouse(GenericAsyncTask *task, void *data)
{
    auto wp = window->get_graphics_window()->get_properties();

    auto actual_mode = wp.get_mouse_mode();
    if(mouse_mode != actual_mode)
	std::cout << "ACTUAL MOUSE MODE: " << actual_mode << '\n';

    mouse_mode = actual_mode;

    rotate_x = rotate_y = -.5;
    last_mouse_x = last_mouse_y = INT_MAX;
    recenter_mouse();
    return AsyncTask::DS_done;
}

void recenter_mouse()
{
    auto win = window->get_graphics_window();
    auto wp = win->get_properties();


    win->move_pointer(0, wp.get_x_size() / 2, wp.get_y_size() / 2);
}

void toggle_recenter()
{
    std::cout << "Toggling re-center behavior\n";
    manual_recenter_mouse = !manual_recenter_mouse;
}

void toggle_mouse()
{
    std::cout << "Toggling mouse visibility\n";

    hide_mouse = !hide_mouse;

    WindowProperties wp;
    wp.set_cursor_hidden(hide_mouse);
    window->get_graphics_window()->request_properties(wp);
}

AsyncTask::DoneStatus mouse_task(GenericAsyncTask *task, void *data)
{
    auto win = window->get_graphics_window();
    if(!win) // exiting
	return AsyncTask::DS_done;
    auto mw = DCAST(MouseWatcher, window->get_mouse().node());

    bool has_mouse = mw->has_mouse();
    PN_stdfloat x, y, dx, dy;
    if(has_mouse) {
	// get the window manager's idea of the mouse position
	x = mw->get_mouse_x(); y = mw->get_mouse_y();

	if(last_mouse_x != INT_MAX) {
	    // get the delta
	    if(manual_recenter_mouse) {
		// when recentering, the position IS the delta
		// since the center is reported as 0, 0
		dx = x; dy = y;
	    } else {
		dx = x - last_mouse_x; dy =  y - last_mouse_y;
	    }
	} else {
	    // no data to compare with yet
	    dx = dy = 0;
	}

	last_mouse_x = x; last_mouse_y = y;

    } else
	x = y = dx = dy = 0;

    if(manual_recenter_mouse) {
	// move mouse back to center
	recenter_mouse();
	last_mouse_x = last_mouse_y = 0;
    }

    // scale position and delta to pixels for user
    auto sz = win->get_size();
    int w = sz[0], h = sz[1];

    std::ostringstream msg; // streams are easier to format than to_string()
    msg << std::boolalpha;  // case in point: boolean & enum names
    msg << "Mode: " << mouse_mode << ", Recenter: " << manual_recenter_mouse <<
	   " | Mouse: " << (int)(x*w) << ", " << (int)(y*h) << " | has_mouse: " << has_mouse;
    mouse_text->set_text(msg.str());
    delta_text->set_text("Delta: " + std::to_string((int)(dx*w)) + ", " + std::to_string((int)(dy*h)));

    // rotate box by delta
    rotate_x += dx * 10 * mouse_magnitude;
    rotate_y += dy * 10 * mouse_magnitude;

    std::ostringstream rot;
    rot << std::setprecision(3); // another case: %.3f (sprintf requires buf)
    rot << "Model rotation: " << rotate_x << ", " << rotate_y;
    position_text->set_text(rot.str());
    model.set_h(rotate_x);
    model.set_p(rotate_y);
    return AsyncTask::DS_cont;
}
}

int main(int argc, char **argv)
{
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
