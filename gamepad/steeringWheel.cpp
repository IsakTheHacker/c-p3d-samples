/*
 *  Translation of Python gamepad sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/gamepad
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Demonstrate usage of steering wheels
 *
 * In this sample you can use a wheel type device to control the camera and
 * show some messages on screen.  You can accelerate forward using the
 * acceleration pedal and slow down using the brake pedal.
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
NodePath lbl_warning, lbl_action;
TextNode *lbl_action_text;
InputDevice *wheel;
PN_stdfloat
    current_move_speed = 0.0,
    max_acceleration = 28.0,
    deceleration = 10.0,
    deceleration_brake = 37.0,
    max_speed = 80.0,
    wheel_center = 0;
AsyncTask::DoneStatus move_task(GenericAsyncTask *task, void *data);

void connect(InputDevice *device), disconnect(InputDevice *device);
void connect(const Event *ev, void *), disconnect(const Event *ev, void *);
void center_wheel(void), reset(void), action(const char *), action_up(void);

void init(void)
{
    default_fov = 60;
    Notify::ptr()->get_category(":device")->set_severity(NS_debug);

    framework.open_framework();
    framework.set_window_title("Steering Wheel - C++ Panda3D Samples");
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

    // Is there a steering wheel connected?
    wheel = nullptr;
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::steering_wheel);
    if(devices.size())
	connect(devices[0]);

    // Accept device dis-/connection events
    auto &evhand = framework.get_event_handler();
    evhand.add_hook("connect-device", connect, 0);
    evhand.add_hook("disconnect-device", disconnect, 0);

    window->enable_keyboard();
    evhand.add_hook("escape", framework.event_esc, &framework);

    // Accept button events of the first connected steering wheel
    // note: framework.event_esc expects WindowFramework event parameter
    evhand.add_hook("steering_wheel0-face_a", EV_FN() { action("Action"); }, 0);
    evhand.add_hook("steering_wheel0-face_a-up", EV_FN() { action_up(); }, 0);
    evhand.add_hook("steering_wheel0-hat_up", EV_FN() { center_wheel(); }, 0);

    auto environment = window->load_model(framework.get_models(), "environment");
    environment.reparent_to(window->get_render());

    // save the center position of the wheel
    // NOTE: here we assume, that the wheel is centered when the application get started.
    //       In real world applications, you should notice the user and give him enough time
    //       to center the wheel until you store the center position of the controler!
    wheel_center = 0;
    if(wheel)
	wheel_center = wheel->find_axis(InputDevice::Axis::wheel).value;

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

    // We're only interested if this is a steering wheel and we don't have a
    // wheel yet.
    if(device->get_device_class() == InputDevice::DeviceClass::steering_wheel && !wheel) {
	std::cout << "Found " << *device << '\n';
	wheel = device;

	// There appears to be no facility in the framework for managing
	// input devices.  Instead, everything is done manually.
	// First, attach it to the data graph root (get_data_root()).
	auto idn = new InputDeviceNode(device, device->get_name());
	auto dn = framework.get_data_root().attach_new_node(idn);
	// Then, attach a ButtonThrower to generate events.
	auto bt = new ButtonThrower(device->get_name());
	dn.attach_new_node(bt);
	// We set up the events with a prefix of "steering_wheel0-".
	bt->set_prefix("steering_wheel0-");

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

    if(wheel != device)
	// We don't care since it's not our wheel.
	return;

    // Remove the data graph nodes
    std::cout << "Disconnected " << *device << '\n';
    framework.get_data_root().find(device->get_name()).remove_node();
    wheel = nullptr;

    // Do we have any other steering wheels?  Attach the first other steering wheel.
    auto devmgr = InputDeviceManager::get_global_ptr();
    auto devices = devmgr->get_devices(InputDevice::DeviceClass::steering_wheel);
    if(devices.size())
	connect(devices[0]);
    else
	// No devices.  Show the warning.
	lbl_warning.show();
}

void reset()
{
    /* Reset the camera to the initial position. */

    window->get_camera_group().set_pos_hpr(0, -200, 2, 0, 0, 0);
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

void center_wheel()
{
    /* Reset the wheels center rotation to the current rotation of the wheel */
    wheel_center = wheel->find_axis(InputDevice::Axis::wheel).value;
}

AsyncTask::DoneStatus move_task(GenericAsyncTask *task, void *data)
{
    // framework does not appear to update devices, so do it here
    InputDeviceManager::get_global_ptr()->update();

    auto dt = ClockObject::get_global_clock()->get_dt();
    LVector3 movement_vec;

    if(!wheel)
	return AsyncTask::DS_cont;

    if(current_move_speed > 0) {
	current_move_speed -= dt * deceleration;
	if(current_move_speed < 0)
	    current_move_speed = 0;
    }

    // we will use the first found wheel
    // accelerate
    auto accelerator_pedal = wheel->find_axis(InputDevice::Axis::accelerator).value;
    auto acceleration = accelerator_pedal * max_acceleration;
    if(current_move_speed > accelerator_pedal * max_speed)
	current_move_speed -= dt * deceleration;
    current_move_speed += dt * acceleration;

    // Brake
    auto brake_pedal = wheel->find_axis(InputDevice::Axis::brake).value;
    auto deceleration = brake_pedal * deceleration_brake;
    current_move_speed -= dt * deceleration;
    if(current_move_speed < 0)
	current_move_speed = 0;

    // Steering
    auto camera = window->get_camera_group();
    auto rotation = wheel_center - wheel->find_axis(InputDevice::Axis::wheel).value;
    camera.set_h(camera, 100 * dt * rotation);

    // calculate movement
    camera.set_y(camera, dt * current_move_speed);

    return AsyncTask::DS_cont;
}
}

int main(int argc, const char **argv)
{
    init();

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
