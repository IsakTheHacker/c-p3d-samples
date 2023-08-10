/*
 *  Translation of Python gamepad sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/gamepad
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Demonstrate usage of gamepads and other input devices
 *
 * In this sample you can use a gamepad type device to control the camera and
 * show some messages on screen.  Using the left stick on the controler will
 * move the camera where the right stick will rotate the camera.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/inputDeviceManager.h>
#include <panda3d/inputDeviceNode.h>
#include <panda3d/buttonThrower.h>
#include <panda3d/gamepadButton.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables.
PandaFramework framework;
WindowFramework* window;
NodePath lbl_warning, lbl_action;
TextNode *lbl_action_text;
InputDevice *gamepad;
AsyncTask::DoneStatus move_task(GenericAsyncTask *, void *);

// Informational text in the bottom-left corner.  We use the special \5
// character to embed an image representing the gamepad buttons.
const char INFO_TEXT[] =
    "Move \5lstick\5 to strafe, \5rstick\5 to turn\n"
    "Press \5ltrigger\5 and \5rtrigger\5 to go down/up\n"
    "Press \5face_x\5 to reset camera";

void connect(InputDevice *device), disconnect(InputDevice *device);
void connect(const Event *ev, void *), disconnect(const Event *ev, void *);
void reset(void), action(const char *), action_up(void);

void init(void)
{
    default_fov = 60;
    Notify::ptr()->get_category(":device")->set_severity(NS_debug);

    framework.open_framework();
    framework.set_window_title("Gamepad - C++ Panda3D Samples");
    window = framework.open_window();

    // Print all events sent through the messenger
    //Notify::ptr()->get_category(":event")->set_severity(NS_spam);

    // Load the graphics for the gamepad buttons and register them, so that
    // we can embed them in our information text.
    auto graphics = def_load_model("models/xbone-icons.egg");
    auto mgr = TextPropertiesManager::get_global_ptr();
    const char * const names[] = {
	"face_a", "face_b", "face_x", "face_y", "ltrigger", "rtrigger",
	"lstick", "rstick"
    };
    for(unsigned i = 0; i < sizeof(names)/sizeof(names[0]); i++) {
	auto graphic = graphics.find(std::string("**/") + names[i]);
	graphic.set_scale(1.5);
	mgr->set_graphic(names[i], graphic);
	graphic.set_z(-0.5);
    }

    // Show the informational text in the corner.
    auto a2d = window->get_aspect_2d();
    auto text_node = new TextNode("instructions");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(INFO_TEXT);
    text.set_pos(-1.0/a2d.get_sx()+0.1, 0, -1+0.3); // BottomLeft == (-1,0,-1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_card_color(0.2, 0.2, 0.2, 0.9); // bg
    text_node->set_align(TextNode::A_left);
    text.set_scale(0.07); // default scale
    text_node->set_card_as_margin(0.5, 0.5, 0.5, 0.2);

    text_node = new TextNode("warning");
    lbl_warning = a2d.attach_new_node(text_node);
    //  (@center = 0,0,0)
    text_node->set_text("No devices found");
    text_node->set_align(TextNode::A_center);
    text_node->set_text_color(1,0,0,1);
    lbl_warning.set_scale(.25);

    lbl_action_text = new TextNode("action");
    lbl_action = a2d.attach_new_node(lbl_action_text);
    // (@center = 0,0,0)
    lbl_action_text->set_text_color(1,1,1,1);
    lbl_action.set_scale(.15);
    lbl_action.hide();

    // Is there a gamepad connected?
    gamepad = nullptr;
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::gamepad);
    if(devices.size())
	connect(devices[0]);

    // Accept device dis-/connection events
    auto &evhand = framework.get_event_handler();
    evhand.add_hook("connect-device", connect, 0);
    evhand.add_hook("disconnect-device", disconnect, 0);

    window->enable_keyboard();
    evhand.add_hook("escape", framework.event_esc, &framework);

    // Accept button events of the first connected gamepad
    // note: framework.event_esc expects WindowFramework event parameter
    evhand.add_hook("gamepad-back", EV_FN() { framework.set_exit_flag(); }, 0);
    evhand.add_hook("gamepad-start", EV_FN() { framework.set_exit_flag(); }, 0);
    evhand.add_hook("gamepad-face_x", EV_FN() { reset(); }, 0);
    evhand.add_hook("gamepad-face_a", EV_FN() { action("face_a"); }, 0);
    evhand.add_hook("gamepad-face_a-up", EV_FN() { action_up(); }, 0);
    evhand.add_hook("gamepad-face_b", EV_FN() { action("face_b"); }, 0);
    evhand.add_hook("gamepad-face_b-up", EV_FN() { action_up(); }, 0);
    evhand.add_hook("gamepad-face_y", EV_FN() { action("face_y"); }, 0);
    evhand.add_hook("gamepad-face_y-up", EV_FN() { action_up(); }, 0);

    auto environment = window->load_model(framework.get_models(), "environment");
    environment.reparent_to(window->get_render());

    reset();

    framework.get_task_mgr().add(new GenericAsyncTask("movement update task", move_task, 0));
}

void connect(const Event *ev, void *)
{
    connect(DCAST(InputDevice, ev->get_parameter(0).get_typed_ref_count_value()));
}

void connect(InputDevice *device)
{
    /* Event handler that is called when a device is discovered. */

    // We're only interested if this is a gamepad and we don't have a
    // gamepad yet.
    if(device->get_device_class() == InputDevice::DeviceClass::gamepad && !gamepad) {
	std::cout << "Found " << *device << '\n';
	gamepad = device;

	// There appears to be no facility in the framework for managing
	// input devices.  Instead, everything is done manually.
	// First, attach it to the data graph root (get_data_root()).
	auto idn = new InputDeviceNode(device, device->get_name());
	auto dn = framework.get_data_root().attach_new_node(idn);
	// Then, attach a ButtonThrower to generate events.
	auto bt = new ButtonThrower(device->get_name());
	dn.attach_new_node(bt);
	// We set up the events with a prefix of "gamepad-".
	bt->set_prefix("gamepad-");

	// Hide the warning that we have no devices.
	lbl_warning.hide();
    }
}

void disconnect(const Event *ev, void *)
{
    disconnect(DCAST(InputDevice, ev->get_parameter(0).get_typed_ref_count_value()));
}

void disconnect(InputDevice *device)
{
    /* Event handler that is called when a device is removed. */

    if(gamepad != device)
	// We don't care since it's not our gamepad.
	return;

    // Remove the data graph nodes
    std::cout << "Disconnected " << *device << '\n';
    framework.get_data_root().find(device->get_name()).remove_node();
    gamepad = nullptr;

    // Do we have any other gamepads?  Attach the first other gamepad.
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::gamepad);
    if(devices.size())
	connect(devices[0]);
    else
	// No devices.  Show the warning.
	lbl_warning.show();
}

void reset()
{
    /* Reset the camera to the initial position. */

    window->get_camera_group().set_pos_hpr(0, -200, 10, 0, 0, 0);
}

void action(const char *button)
{
    // Just show which button has been pressed.
    lbl_action_text->set_text(std::string("Pressed \5") + button + "\5");
    lbl_action.show();
}

void action_up()
{
    // Hide the label showing which button is pressed.
    lbl_action.hide();
}

AsyncTask::DoneStatus move_task(GenericAsyncTask *, void *)
{
    // framework does not appear to update devices, so do it here
    InputDeviceManager::get_global_ptr()->update();

    auto dt = ClockObject::get_global_clock()->get_dt();

    if(!gamepad)
	return AsyncTask::DS_cont;

    PN_stdfloat
	strafe_speed = 85,
	vert_speed = 50,
	turn_speed = 100;

    // If the left stick is pressed, we will go faster.
    auto lstick = gamepad->find_button(GamepadButton::lstick().get_index());
    if(lstick.is_pressed())
	strafe_speed *= 2.0;

    // we will use the first found gamepad
    // Move the camera left/right
    auto strafe = LVector3(0);
    auto left_x = gamepad->find_axis(InputDevice::Axis::left_x);
    auto left_y = gamepad->find_axis(InputDevice::Axis::left_y);
    strafe.set_x(left_x.value);
    strafe.set_y(left_y.value);

    // Apply some deadzone, since the sticks don't center exactly at 0
    auto camera = window->get_camera_group();
    if(strafe.length_squared() >= 0.01)
	camera.set_pos(camera, strafe * strafe_speed * dt);

    // Use the triggers for the vertical position.
    auto trigger_l = gamepad->find_axis(InputDevice::Axis::left_trigger);
    auto trigger_r = gamepad->find_axis(InputDevice::Axis::right_trigger);
    auto lift = trigger_r.value - trigger_l.value;
    camera.set_z(camera.get_z() + (lift * vert_speed * dt));

    // Move the camera forward/backward
    auto right_x = gamepad->find_axis(InputDevice::Axis::right_x);
    auto right_y = gamepad->find_axis(InputDevice::Axis::right_y);

    // Again, some deadzone
    if(std::abs(right_x.value) >= 0.1 ||
       std::abs(right_y.value) >= 0.1) {
	camera.set_h(camera, turn_speed * dt * -right_x.value);
	camera.set_p(camera, turn_speed * dt * right_y.value);

	// Reset the roll so that the camera remains upright.
	camera.set_r(0);
    }

    return AsyncTask::DS_cont;
}
}

int main(int argc, const char **argv)
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1)
	get_model_path().prepend_directory(argv[1]);

    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
