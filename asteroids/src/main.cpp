
#include <textNode.h>
#include <transparencyAttrib.h>
#include <samplerState.h>

// "globals"
#include <pandaFramework.h>
namespace Globals {
// Constants that will control the behavior of the game. It is good to
// group constants like this so that they can be changed once without
// having to find everywhere they are used in code
const PN_stdfloat
    SPRITE_POS = 55,     // At default field of view and a depth of 55, the screen
                         // dimensions is 40x30 units
    SCREEN_X = 20,       // Screen goes from -20 to 20 on X
    SCREEN_Y = 15,       // Screen goes from -15 to 15 on Y
    TURN_RATE = 360,     // Degrees ship can turn in 1 second
    ACCELERATION = 10,   // Ship acceleration in units/sec/sec
    MAX_VEL = 6,         // Maximum ship velocity in units/sec
    MAX_VEL_SQ = (MAX_VEL * MAX_VEL),  // Square of the ship velocity
    DEG_TO_RAD = MathNumbers::deg_2_rad,  // translates degrees to radians for sin and cos
    BULLET_LIFE = 2,     // How long bullets stay on screen before removed
    BULLET_REPEAT = .2,  // How often bullets can be fired
    BULLET_SPEED = 10,   // Speed bullets move
    AST_INIT_VEL = 1,    // Velocity of the largest asteroids
    AST_INIT_SCALE = 3,  // Initial asteroid scale
    AST_VEL_SCALE = 2.2,  // How much asteroid speed multiplies when broken up
    AST_SIZE_SCALE = .6,  // How much asteroid scale changes when broken up
    AST_MIN_SCALE = 1.1;  // If and asteroid is smaller than this and is hit,
                           // it disapears instead of splitting up

    // Global variables
    PandaFramework framework;
    WindowFramework *window;
    Randomizer rands;
    std::string sample_path;
    NodePath title;
    NodePath escape_text, leftkey_text, rightkey_text, upkey_text, spacekey_text;
    NodePath bg, ship;
    PN_stdfloat next_bullet;
    std::vector<NodePath> bullets, asteroids;
    bool alive;

    enum k_t {
	k_turn_left, k_turn_right, k_accel, k_fire, k_num, k_pressed = 32
    };
    bool keys[k_num];
};
using namespace Globals;

// This helps reduce the amount of code used by loading objects, since all of
// the objects are pretty much the same.
NodePath load_object(std::string tex, PN_stdfloat scale = 1, LPoint2 pos={0,0},
		     PN_stdfloat depth = SPRITE_POS, bool transparency = true)
{
    // Every object uses the plane model and is parented to the camera
    // so that it faces the screen.
    auto obj = window->load_model(framework.get_models(), sample_path + "models/plane.egg");
    obj.reparent_to(window->get_camera_group());

    // Set the initial position and scale.
    obj.set_pos(pos[0], depth, pos[1]);
    obj.set_scale(scale);

    // This tells Panda not to worry about the order that things are drawn in
    // (ie. disable Z-testing).  This prevents an effect known as Z-fighting.
    obj.set_bin("unsorted", 0);
    obj.set_depth_test(false);

    if(transparency)
        // Enable transparency blending.
        obj.set_transparency(TransparencyAttrib::M_alpha);

    if(tex.size()) {
        // Load and set the requested texture.
	PT(Texture) texture = new Texture(tex);
	texture->read(sample_path + "textures/" + tex);
        texture->set_wrap_u(SamplerState::WM_clamp);
	texture->set_wrap_v(SamplerState::WM_clamp);
        obj.set_texture(texture, 1);
    }

    return obj;
}

// Macro-like function used to reduce the amount to code needed to create the
// on screen instructions
NodePath gen_label_text(const char *text, int i)
{
    TextNode *text_node = new TextNode(text);
    auto ret = NodePath(text_node);
    text_node->set_text(text);
    ret.reparent_to(window->get_aspect_2d()); // a2d
    ret.set_pos(-1.0 + 0.07, 0, 1.0 - 0.06 * i - 0.1); // TopLeft == (-1,0,1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_left);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    ret.set_scale(0.05);
    return ret;
}

// forward decls
void set_velocity(NodePath &, LVector3);
void set_key(const Event *, void *);
void spawn_asteroids(void);
AsyncTask::DoneStatus game_loop(GenericAsyncTask *, void *);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();

    //Set the window title and open new window
    framework.set_window_title("Asteroids - C++ Panda3D Samples");
    window = framework.open_window();

    // This code puts the standard title and instruction text on screen
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    TextNode *text_node = new TextNode("title");
    title = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Tasks");
    title.reparent_to(window->get_aspect_2d()); // a2d
    title.set_scale(0.07);
    text_node->set_align(TextNode::A_right);
    title.set_pos(1.0-0.1, 0, -1+0.1); // BottomRight == (1,0,-1)
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    escape_text = gen_label_text("ESC: Quit", 0);
    leftkey_text = gen_label_text("[Left Arrow]: Turn Left (CCW)", 1);
    rightkey_text = gen_label_text("[Right Arrow]: Turn Right (CW)", 2);
    upkey_text = gen_label_text("[Up Arrow]: Accelerate", 3);
    spacekey_text = gen_label_text("[Space Bar]: Fire", 4);

    // Mouse-based camera control requires manual enable.  Don't do that.

    // Load the background starfield.
    // FIXME:  I am doing something wrong here.  It doesn't do anything
    window->get_graphics_output()->set_clear_color(LColor(0, 0, 0, 1));
    window->get_graphics_output()->set_clear_color_active(true);

    bg = load_object("stars.jpg", 146, {0,0}, 200, false);

    // Load the ship and set its initial velocity.
    ship = load_object("ship.png");
    set_velocity(ship, LVector3::zero());

    // An array of what keys are currently being pressed
    // The key events update this list, and our task will query it as input
    memset(keys, 0, sizeof(keys)); // all false

    window->enable_keyboard();
    // Instead of definint ESC
    //framework.define_key("escape", "Quit", exit, 0);
    // Use the default keybindings (partial conflict, but OK)
    framework.enable_default_keys();

    // Other keys events set the appropriate value in our key array
    framework.define_key("arrow_left",     "Turn Left",  set_key, (void *)(k_turn_left | k_pressed));
    framework.define_key("arrow_left-up",  "",           set_key, (void *)(k_turn_left));
    framework.define_key("arrow_right",    "Turn Right", set_key, (void *)(k_turn_right | k_pressed));
    framework.define_key("arrow_right-up", "",           set_key, (void *)(k_turn_right));
    framework.define_key("arrow_up",       "Accelerate", set_key, (void *)(k_accel | k_pressed));
    framework.define_key("arrow_up-up",    "",           set_key, (void *)k_accel);
    framework.define_key("space",          "Fire",       set_key, (void *)(k_fire | k_pressed));

    // Now we create the task.  A task is derived from AsyncTask, which
    // has a do_task() method to perform the task.  There are a few special
    // purpose types derived from this; the one we're interested in is
    // GenericAsyncTask, which executes an arbitrary function.  The
    // constructor takes a name (for lookup and serialization),
    // a function pointer, and the user data (void *).  The function receives
    // both the user data and a pointer to the task structure itself, so you
    // can derive from GenericAsyncTask to add user data there instead.  If
    // you're going to do that, though, you may as well derive from AsyncTask
    // instead, and override do_task with your function while you're at it.
    auto game_task = new GenericAsyncTask("game_loop", game_loop, 0);
    // The AsyncTaskManager is the task manager that actually executes the
    // AsyncTask each frame.  There is one global that most users should
    // always use, found by get_global_ptr().  The add method queues
    // and effectively takes ownership of the task.
    AsyncTaskManager::get_global_ptr()->add(game_task);

    // Stores the time at which the next bullet may be fired.
    next_bullet = 0.0;

    // This list will stored fired bullets.
    bullets.clear();

    // Complete initialization by spawning the asteroids.
    spawn_asteroids();
}

// Since the C++ API for tags is raw strings, and the serialization is hidden
// inside other facilities, here are some helpers for space-separated floats
std::string f_to_str(const PN_stdfloat *f, size_t num)
{
    std::string res;
    while(num-- > 0) {
	if(res.size())
	    res += ' ';
	res += std::to_string(*f++);
    }
    return res;
}

void str_to_f(PN_stdfloat *f, size_t num, const std::string &str)
{
    std::string::size_type off = 0, noff;
    while(num-- > 0) {
	*f++ = std::stod(str.substr(off), &noff);
	off += noff;
    }
    // assert(str.size() == off)
}

// As described earlier, this simply sets a key in the keys array
// to whether or not the key is pressed.
void set_key(const Event *, void *_k)
{
    k_t k = (k_t)(unsigned long)_k;
    keys[k & (k_pressed - 1)] = k >= k_pressed;
};

void set_velocity(NodePath &obj, LVector3 val)
{
    obj.set_tag("velocity", f_to_str(val.begin(), 3));
}

LVector3 get_velocity(const NodePath &obj)
{
    PN_stdfloat ret[3];
    str_to_f(ret, 3, obj.get_tag("velocity"));
    return LVector3(ret[0], ret[1], ret[2]);
}

void set_expires(NodePath &obj, PN_stdfloat val)
{
    obj.set_tag("expires", f_to_str(&val, 1));
}

PN_stdfloat get_expires(const NodePath &obj)
{
    PN_stdfloat ret;
    str_to_f(&ret, 1, obj.get_tag("expires"));
    return ret;
}

void spawn_asteroids()
{
    // Control variable for if the ship is alive
    alive = true;
    asteroids.clear();  // List that will contain our asteroids

    for(unsigned i = 0; i < 10; i++) {
	// This loads an asteroid. The texture chosen is random
	// from "asteroid1.png" to "asteroid3.png".
	std::string texname("asteroid");
	texname += std::to_string(rands.random_int(3) + 1) + ".png";
	auto asteroid = load_object(texname, AST_INIT_SCALE);
	asteroids.push_back(asteroid);

	// This is kind of a hack, but it keeps the asteroids from spawning
	// near the player.  It avoids placing at -4..4.  The Python version
	// used a random element of a table/range for this, but I have
	// changed it to just compute that.  Since the player starts at 0
	// and this list doesn't contain anything from -4 to 4, it won't be
	// close to the player.
	int x_choice = rands.random_int(((int)SCREEN_X + 1) * 2 - 9) - (int)SCREEN_X;
	if(x_choice >= -4)
	    x_choice += 9;
	asteroid.set_x(x_choice);
	// Same thing for Y
	int y_choice = rands.random_int(((int)SCREEN_Y + 1) * 2 - 9) - (int)SCREEN_Y;
	if(y_choice >= -4)
	    y_choice += 9;
	asteroid.set_z(y_choice);

	// Heading is a random angle in radians
	auto heading = rands.random_real(2 * MathNumbers::pi);

	// Converts the heading to a vector and multiplies it by speed to
	// get a velocity vector
	auto v = LVector3(sin(heading), 0, cos(heading)) * AST_INIT_VEL;
	set_velocity(asteroids[i], v);
    }
}

void update_ship(double);
void fire(double);
void update_pos(NodePath &, double);
void asteroid_hit(unsigned i);

// This is our main task function, which does all of the per-frame
// processing.  It takes in self like all functions in a class, and task,
// the task object returned by taskMgr.
AsyncTask::DoneStatus game_loop(GenericAsyncTask *task, void *)
{
    // Get the time elapsed since the next frame.  We need this for our
    // distance and velocity calculations.
    double dt = ClockObject::get_global_clock()->get_dt();

    // If the ship is not alive, do nothing.  Tasks return AsyncTask::DS_cont
    // to signify that the task should continue running. If AsyncTask::DS_done
    // were returned instead, the task would be removed and would no longer be
    // called every frame.
    if(!alive)
	return AsyncTask::DS_cont;

    // update ship position
    update_ship(dt);

    // check to see if the ship can fire
    auto time = task->get_elapsed_time();
    if(keys[k_fire] && time > next_bullet) {
	fire(time);  // If so, call the fire function
	// And disable firing for a bit
	next_bullet = time + BULLET_REPEAT;
    }
    // Remove the fire flag until the next spacebar press
    keys[k_fire] = false;

    // update asteroids
    for(auto &obj : asteroids)
	update_pos(obj, dt);

    // update bullets
    std::vector<NodePath> new_bullet_array;
    for(auto &obj : bullets) {
	update_pos(obj, dt);  // Update the bullet
	// Bullets have an expiration time (see fire() function)
	// If a bullet has not expired, add it to the new bullet list so
	// that it will continue to exist.
	if(get_expires(obj) > time)
	    new_bullet_array.push_back(obj);
	else
	    obj.remove_node();  // Otherwise, remove it from the scene.
    }
    // Set the bullet array to be the newly updated array
    bullets = new_bullet_array;

    // Check bullet collision with asteroids
    // In short, it checks every bullet against every asteroid. This is
    // quite slow.  A big optimization would be to sort the objects left to
    // right and check only if they overlap.  Framerate can go way down if
    // there are many bullets on screen, but for the most part it's okay.
    for(auto &bullet: bullets) {
	// This range statement makes it step though the asteroid list
	// backwards.  This is because if an asteroid is removed, the
	// elements after it will change position in the list.  If you go
	// backwards, the length stays constant.
	auto bullet_size = bullet.get_scale().get_x();
	for(unsigned i = asteroids.size(); i--; ) {
	    auto &asteroid = asteroids[i];
	    // Panda's collision detection is more complicated than we need
	    // here.  This is the basic sphere collision check. If the
	    // distance between the object centers is less than sum of the
	    // radii of the two objects, then we have a collision. We use
	    // lengthSquared() since it is faster than length().
	    auto rad_sum = (bullet_size + asteroid.get_scale().get_x()) * .5;
	    if((bullet.get_pos() - asteroid.get_pos()).length_squared() <
	       rad_sum * rad_sum) {
		// Schedule the bullet for removal
		set_expires(bullet, 0);
		asteroid_hit(i);      // Handle the hit
	    }
	}
    }

    // Now we do the same collision pass for the ship
    auto ship_size = ship.get_scale().get_x();
    for(auto &ast : asteroids) {
	// Same sphere collision check for the ship vs. the asteroid
	auto rad_sum = (ship_size + ast.get_scale().get_x()) * .5;
	if((ship.get_pos() - ast.get_pos()).length_squared() <
	   rad_sum * rad_sum) {
	    // If there is a hit, clear the screen and schedule a restart
	    alive = false;         // Ship is no longer alive
	    // Remove every object in asteroids and bullets from the scene
	    for(auto &i: asteroids)
		i.remove_node();
	    for(auto &i: bullets)
		i.remove_node();
	    bullets.clear();          // Clear the bullet list
            ship.hide();           // Hide the ship
	    // Reset the velocity
	    set_velocity(ship, LVector3(0, 0, 0));
	    auto task = new GenericAsyncTask("restart", [](GenericAsyncTask *,void *) {
		ship.set_r(0);  // Reset heading
		ship.set_x(0);  // Reset position X
		ship.set_z(0);  // Reset position Y (Z for Panda)
		ship.show();    // Show the ship
		spawn_asteroids();  // Remake asteroids
		return AsyncTask::DS_done;
	    }, 0);
	    task->set_delay(2);          // Wait 2 seconds first
	    AsyncTaskManager::get_global_ptr()->add(task);
	    return AsyncTask::DS_cont;
	}
    }

    // If the player has successfully destroyed all asteroids, respawn them
    if(asteroids.size() == 0)
	spawn_asteroids();

    return AsyncTask::DS_cont;    // Since every return is Task.cont, the task will
                                  // continue indefinitely
}

// Updates the positions of objects
void update_pos(NodePath &obj, double dt)
{
    auto vel = get_velocity(obj);
    auto new_pos = obj.get_pos() + (vel * dt);

    // Check if the object is out of bounds. If so, wrap it
    auto radius = .5 * obj.get_scale().get_x();
    if(new_pos.get_x() - radius > SCREEN_X)
	new_pos.set_x(-SCREEN_X);
    else if(new_pos.get_x() + radius < -SCREEN_X)
	new_pos.set_x(SCREEN_X);
    if(new_pos.get_z() - radius > SCREEN_Y)
	new_pos.set_z(-SCREEN_Y);
    else if(new_pos.get_z() + radius < -SCREEN_Y)
	new_pos.set_z(SCREEN_Y);

    obj.set_pos(new_pos);
}

// The handler when an asteroid is hit by a bullet
void asteroid_hit(unsigned index)
{
    // If the asteroid is small it is simply removed
    if(asteroids[index].get_scale().get_x() <= AST_MIN_SCALE) {
	asteroids[index].remove_node();
	// Remove the asteroid from the list of asteroids.
	asteroids.erase(asteroids.begin() + index);
    } else {
	// If it is big enough, divide it up into little asteroids.
	// First we update the current asteroid.
	auto &asteroid = asteroids[index];
	auto new_scale = asteroid.get_scale().get_x() * AST_SIZE_SCALE;
	asteroid.set_scale(new_scale);  // Rescale it

	// The new direction is chosen as perpendicular to the old direction
	// This is determined using the cross product, which returns a
	// vector perpendicular to the two input vectors.  By crossing
	// velocity with a vector that goes into the screen, we get a vector
	// that is orthagonal to the original velocity in the screen plane.
	auto vel = get_velocity(asteroid);
	auto speed = vel.length() * AST_VEL_SCALE;
	vel.normalize();
	vel = LVector3(0, 1, 0).cross(vel);
	vel *= speed;
	set_velocity(asteroid, vel);

	// Now we create a new asteroid identical to the current one
	auto new_ast = load_object(std::string(), new_scale);
	set_velocity(new_ast, vel * -1);
	new_ast.set_pos(asteroid.get_pos());
	new_ast.set_texture(asteroid.get_texture(), 1);
	asteroids.push_back(new_ast);
    }
}

// This updates the ship's position. This is similar to the general update
// but takes into account turn and thrust
void update_ship(double dt)
{
    auto heading = ship.get_r();  // Heading is the roll value for this model
    // Change heading if left or right is being pressed
    if(keys[k_turn_right]) {
	heading += dt * TURN_RATE;
	ship.set_r((int)heading % 360);
    } else if(keys[k_turn_left]) {
	heading -= dt * TURN_RATE;
	ship.set_r((int)heading % 360);
    }

    // Thrust causes acceleration in the direction the ship is currently
    // facing
    if(keys[k_accel]) {
	auto heading_rad = DEG_TO_RAD * heading;
	// This builds a new velocity vector and adds it to the current one
	// relative to the camera, the screen in Panda is the XZ plane.
	// Therefore all of our Y values in our velocities are 0 to signify
	// no change in that direction.
	auto new_vel =
	    LVector3(sin(heading_rad), 0, cos(heading_rad)) * ACCELERATION * dt;
	new_vel += get_velocity(ship);
	// Clamps the new velocity to the maximum speed. lengthSquared() is
	// used again since it is faster than length()
	if(new_vel.length_squared() > MAX_VEL_SQ) {
	    new_vel.normalize();
	    new_vel *= MAX_VEL;
	}
	set_velocity(ship, new_vel);
    }

    // Finally, update the position as with any other object
    update_pos(ship, dt);
}

// Creates a bullet and adds it to the bullet list
void fire(double time)
{
    auto direction = DEG_TO_RAD * ship.get_r();
    auto pos = ship.get_pos();
    auto bullet = load_object("bullet.png", .2);  // Create the object
    bullet.set_pos(pos);
    // Velocity is in relation to the ship
    auto vel = get_velocity(ship) +
	(LVector3(sin(direction), 0, cos(direction)) *
	    BULLET_SPEED);
    set_velocity(bullet, vel);
    // Set the bullet expiration time to be a certain amount past the
    // current time
    set_expires(bullet, time + BULLET_LIFE);

    // Finally, add the new bullet to the list
    bullets.push_back(bullet);
}

int main(int argc, char **argv)
{
    if(argc > 1 && !strcmp(argv[1], "-vs"))
	sample_path = "../../../";
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
