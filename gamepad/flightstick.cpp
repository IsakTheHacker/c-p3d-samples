/*
 *  Translation of Python gamepad sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/gamepad
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Demonstrate usage of flight stick
 *
 * In this sample you can use a flight stick to control the camera and show some
 * messages on screen.  You can accelerate using the throttle
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/inputDeviceManager.h>
#include <panda3d/inputDeviceNode.h>
#include <panda3d/buttonThrower.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables.
PandaFramework framework;
WindowFramework* window;
std::string sample_path
#ifdef SAMPLE_DIR
	= SAMPLE_DIR "/"
#endif
	;
NodePath lbl_warning, lbl_action;
TextNode *lbl_action_text;
InputDevice *flight_stick;
PN_stdfloat
    current_move_speed = 0.0,
    max_acceleration = 28.0,
    deceleration = 10.0,
    deacleration_brake = 37.0,
    max_speed = 80.0;
AsyncTask::DoneStatus move_task(GenericAsyncTask *task, void *data);

#define STICK_DEAD_ZONE 0.02
#define THROTTLE_DEAD_ZONE 0.02

void connect(InputDevice *device), disconnect(InputDevice *device);
void connect(const Event *ev, void *), disconnect(const Event *ev, void *);
void reset(void), action(const char *), action_up(void);

void init(void)
{
    default_fov = 60;
    Notify::ptr()->get_category(":device")->set_severity(NS_debug);

    framework.open_framework();
    framework.set_window_title("Flight Stick - C++ Panda3D Samples");
    window = framework.open_window();

    // Print all events sent through the messenger
    //Notify::ptr()->get_category(":event")->set_severity(NS_spam);

    auto a2d = window->get_aspect_2d();
    auto text_node = new TextNode("warning");
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

    // Is there a flight stick connected?
    flight_stick = nullptr;
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::flight_stick);
    if(devices.size())
	connect(devices[0]);

    // Accept device dis-/connection events
    auto &evhand = framework.get_event_handler();
    evhand.add_hook("connect-device", connect, 0);
    evhand.add_hook("disconnect-device", disconnect, 0);

    window->enable_keyboard();
    evhand.add_hook("escape", framework.event_esc, &framework);
    // note: framework.event_esc expects WindowFramework event parameter
    evhand.add_hook("flight_stick0-start", EV_FN() { framework.set_exit_flag(); }, 0);

    // Accept button events of the first connected flight stick
    evhand.add_hook("flight_stick0-trigger", EV_FN() { action("trigger"); }, 0);
    evhand.add_hook("flight_stick0-trigger-up", EV_FN() { action_up(); }, 0);

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

    // We're only interested if this is a flight stick and we don't have a
    // flight stick yet.
    if(device->get_device_class() == InputDevice::DeviceClass::flight_stick && !flight_stick) {
	std::cout << "Found " << *device << '\n';
	flight_stick = device;

	// There appears to be no facility in the framework for managing
	// input devices.  Instead, everything is done manually.
	// First, attach it to the data graph root (get_data_root()).
	auto idn = new InputDeviceNode(device, device->get_name());
	auto dn = framework.get_data_root().attach_new_node(idn);
	// Then, attach a ButtonThrower to generate events.
	auto bt = new ButtonThrower(device->get_name());
	dn.attach_new_node(bt);
	// We set up the events with a prefix of "flight_stick0-".
	bt->set_prefix("flight_stick0-");

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

    if(flight_stick != device)
	// We don't care since it's not our flight stick.
	return;

    // Remove the data graph nodes
    std::cout << "Disconnected " << *device << '\n';
    framework.get_data_root().find(device->get_name()).remove_node();
    flight_stick = nullptr;

    // Do we have any other flight sticks?  Attach the first other flight stick.
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::flight_stick);
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
    lbl_action_text->set_text(std::string("Pressed ") + button);
    lbl_action.show();
}

void action_up()
{
    // Hide the label showing which button is pressed.
    lbl_action.hide();
}

AsyncTask::DoneStatus move_task(GenericAsyncTask *task, void *data)
{
    // framework does not appear to update devices, so do it here
    InputDeviceManager::get_global_ptr()->update();

    auto dt = ClockObject::get_global_clock()->get_dt();

    if(!flight_stick)
	return AsyncTask::DS_cont;

    if(current_move_speed > 0) {
	current_move_speed -= dt * deceleration;
	if(current_move_speed < 0)
	    current_move_speed = 0;
    }

    // Accelerate using the throttle.  Apply deadzone of 0.01.
    auto throttle = flight_stick->find_axis(InputDevice::Axis::throttle).value;
    if(std::abs(throttle) < THROTTLE_DEAD_ZONE)
	throttle = 0;
    auto acceleration = throttle * max_acceleration;
    if(current_move_speed > throttle * max_speed)
	current_move_speed -= dt * deceleration;
    current_move_speed += dt * acceleration;

    // Steering

    // Control the cameras yaw/Headding
    auto camera = window->get_camera_group();
    auto stick_yaw = flight_stick->find_axis(InputDevice::Axis::yaw);
    if(std::abs(stick_yaw.value) > STICK_DEAD_ZONE)
	camera.set_h(camera, 100 * dt * stick_yaw.value);

    // Control the cameras pitch
    auto stick_y = flight_stick->find_axis(InputDevice::Axis::pitch);
    if(std::abs(stick_y.value) > STICK_DEAD_ZONE)
	camera.set_p(camera, 100 * dt * stick_y.value);

    // Control the cameras roll
    auto stick_x = flight_stick->find_axis(InputDevice::Axis::roll);
    if(std::abs(stick_x.value) > STICK_DEAD_ZONE)
	camera.set_r(camera, 100 * dt * stick_x.value);

    // calculate movement
    camera.set_y(camera, dt * current_move_speed);

    // Make sure camera does not go below the ground.
    if(camera.get_z() < 1)
	camera.set_z(1);

    return AsyncTask::DS_cont;
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
