/* Conversion of https://oldmanprogrammer.net/demos/square/map.js to Panda3D */
/* Original by Steve Baker.  Conversion by Thomas J. Moore */
/* Some of this code comes from my C++/OpenSceneGraph port done earlier */
/* This is meant to be stand-alone, not dependent on any of my previously
 * developed support utilities for converting the official samples from
 * Python to C++.  Some of that support code is duplicated here.  In
 * particular, most of CMakeLists.txt is copied from samples.cmake. */
/* Since this isn't a port of an official tutorial sample, I use some differnt
 * coding conventions as well. */
/*
 * Command-line arguments:
 *     r  roughness (default: 5)
 *     s  size      (default: 64)
 *     z  zscale    (default: 2.0)  (scale == 1/val)
 *     c  config    set config (e.g. c "win-size 1280 720")
 *     you can combine the above letters; args will be conusmed in order.
 *     e.g. rsz 2 1024 1.0
 *
 * Key bindings: use "?" to get a list.
 *   also, up/down/left-right/pgup/pgdown = move camera
 *   LMB+mouse = change view angle (I don't use trackball as it conflicts w/ above)
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/load_prc_file.h>
#include <panda3d/shadeModelAttrib.h>
#include <panda3d/ambientLight.h>
#include <panda3d/directionalLight.h>
#include "map.h"

/* This is global to allow use in basic lambda functions */
static Map m;
NodePath render;
PT(GenericAsyncTask) rotater;
PandaFramework framework;
WindowFramework *window;

void redraw_map(void)
{
  // Lacking easy graphical notification, I'll just print this to stdout:
  m.makemap();
  printf("%dx%d r%d z%g\n", m.mapsize, m.mapsize, m.roughness, 1/render.get_sz());
  if(render.get_num_children()) {
    auto n = render.get_child(0);
    n.detach_node(); // not doing this first often caused crashes/slowness
    n.remove_node(); // really, this should be enough.
  }
  PT(GeomNode) geom = new GeomNode("map");
  // The companion flag to flat_last_vertex; doesn't seem to work
  // quite right, though.  I still get a "curvy" surface.
  // setting this via geom->set_attrib doesn't quite work, either.
  // at least setting it via 
//  geom->set_attrib(ShadeModelAttrib::make(ShadeModelAttrib::M_flat));
  CPT(RenderState) state;
  state = RenderState::make(ShadeModelAttrib::make(ShadeModelAttrib::M_flat),
                            ColorAttrib::make_vertex());
  geom->add_geom(m.drawmap(), state);
  auto n = render.attach_new_node(geom);
  n.force_recompute_bounds();
#if 0
  cerr << n << '\n';
  n.ls();
  cerr << *geom << '\n';
  cerr << *geom->get_geom(0) << '\n';
  cerr << *geom->get_geom(0)->get_primitive(0) << '\n';
#endif
  auto camera = window->get_camera_group();
  camera.set_pos(0, -(PN_stdfloat)m.mapsize/8,  256 * render.get_sz());
  camera.look_at(n);
}

int main(int argc, char **argv)
{
  srand48(time(NULL));
  // Initialize Panda3D
  framework.open_framework();
  window = framework.open_window();
  //window->set_background_type(WindowFramework::BT_black);

  // find the map size for further computation
  // I don't scale things to 2048x2048 like the js demo did
  // Map m;  // done globally now.

  // Use separate root node to scale, rotate
  render = window->get_render().attach_new_node(new PandaNode("maprender"));
  render.set_sz(1/2.0);

  // cmd line args (panda3d has no standard args, so I don't parse any)
  // to set panda3d config, I added a 'c' flag.
  while(--argc > 0) {
    auto arg = *++argv;
    while(*arg == '-') // there's really no need for -
      arg++;
    while(*arg) {
      switch(*arg++) {
       case 'r':
	if(--argc > 0)
	  m.roughness = atof(*++argv);
	break;
       case 's':
	if(--argc > 0)
	  m.mapsize = atol(*++argv);
	break;
       case 'z':
	if(--argc > 0)
	  render.set_sz(1/atof(*++argv));
	break;
       case 'c':
	if(--argc > 0)
	  load_prc_file_data("", *++argv);
      }
    }
  }

  // add standard lights
//  window->set_lighting(true);
  auto grender = window->get_render();
  auto alight = new AmbientLight("ambient");
  alight->set_color(LColor(1, 1, 1, 1));
  grender.set_light(grender.attach_new_node(alight));
#if 0
  auto dlight = new DirectionalLight("direct");
  dlight->set_color(LColor(.6, .6, .6, 1));
  auto dln = grender.attach_new_node(dlight);
  grender.set_light(dln);
  dln.set_pos_hpr(0, 0, 16000, 0, 90, 0);
#endif

  // add the standard keybindings, including esc and ? mainly
  // most don't do anything useful, so I should probably trim it.
  window->enable_keyboard();
  framework.enable_default_keys();
  // and a few more
#define EVH(p) [](const Event *, void *p)
  framework.define_key("[", "Halve map size", EVH() {
    if(m.mapsize > 4) {
      m.mapsize /= 2;
      redraw_map();
    }
  }, 0);
  framework.define_key("]", "Double map size", EVH() {
    if(m.mapsize < 8192) {
      m.mapsize *= 2;
      redraw_map();
    }
  }, 0);
  framework.define_key("r", "Reduce roughness", EVH() {
    if(m.roughness > 1) {
      --m.roughness;
      redraw_map();
    }
  }, 0);
  framework.define_key("R", "Increase roughness", EVH() {
    if(m.roughness < 32) {
      ++m.roughness;
      redraw_map();
    }
  }, 0);
  framework.define_key("{", "Decrease Z scale factor", EVH() {
    auto sz = 1/render.get_sz();
    if(sz > 0.15) {
      sz -= 0.1;
      render.set_sz(1/sz);
    }
  }, 0);
  framework.define_key("}", "Increase Z scale factor", EVH() {
    auto sz = 1/render.get_sz() + 0.1;
    render.set_sz(1/sz);
  }, 0);
  framework.define_key("g", "Generate new map", EVH() {
    redraw_map();
  }, 0);
  // set up the camera manipulator (well, maybe not: interferes w/ kb ctrls
  // below; the only way this would work is if I added it manually, and used a
  // node hierarchy to apply 2 transforms at once.  Not going to happen.
  // Instead, I just added click+view support to the keyboard updater.
//  window->setup_trackball();
  bool key_pressed[9] = {};
  // note: these key bindings could be done differently instead of adding
  // an individual handler for every key.  I'm not doing that now, though.
  auto &evhand = framework.get_event_handler(); // undocumented keys
#define key_setter(k, v) EVH(kpa) { ((bool *)kpa)[k] = v; }, key_pressed
  evhand.add_hook("arrow_up", key_setter(0, true));
  evhand.add_hook("arrow_down", key_setter(1, true));
  evhand.add_hook("arrow_left", key_setter(2, true));
  evhand.add_hook("arrow_right", key_setter(3, true));
  evhand.add_hook("page_up", key_setter(4, true));
  evhand.add_hook("page_down", key_setter(5, true));
  evhand.add_hook("arrow_up-up", key_setter(0, false));
  evhand.add_hook("arrow_down-up", key_setter(1, false));
  evhand.add_hook("arrow_left-up", key_setter(2, false));
  evhand.add_hook("arrow_right-up", key_setter(3, false));
  evhand.add_hook("page_up-up", key_setter(4, false));
  evhand.add_hook("page_down-up", key_setter(5, false));
#if 1 // manual mouse controls rather than window's "default" trackball
  evhand.add_hook("mouse1", key_setter(6, true));
  evhand.add_hook("mouse2", key_setter(7, true));
  evhand.add_hook("mouse3", key_setter(8, true));
  evhand.add_hook("mouse1-up", key_setter(6, false));
  evhand.add_hook("mouse2-up", key_setter(7, false));
  evhand.add_hook("mouse3-up", key_setter(8, false));
#endif

  // A task to move the camera based on the keys
  auto task = new GenericAsyncTask("keymove", [](GenericAsyncTask *, void *data) {
    auto win = window->get_graphics_window();
    if(!win)
      return AsyncTask::DS_done;
    auto camera = window->get_camera_group();
//    cerr << camera.get_pos() << ' ' << camera.get_hpr() << "    \r";
    bool *kpa = (bool *)data;
    // figure out how much the mouse has moved (in pixels)
    static int last_x = -1, last_y = -1;
    if(!kpa[6])
      last_x = last_y = -1;
    else {
      auto md = win->get_pointer(0);
      auto x = md.get_x();
      auto y = md.get_y();
      if(last_x < 0) {
	last_x = x;
	last_y = y;
      }
      auto heading = camera.get_h(), pitch = camera.get_p(),
	   roll = camera.get_r();
//      if(win->move_pointer(0, 100, 100)) {
	heading = heading - (x - last_x) * 0.2;
	pitch = pitch - (y - last_y) * 0.2;
#if 0
	if(pitch < -90)
	pitch = -90;
	if(pitch > 90)
	  pitch = 90;
#endif
//      }
      camera.set_hpr(heading, pitch, roll);
      last_x = x;
      last_y = y;
    }
    LVector3 adj(0, 0, 0);
    if(!kpa[0] && !kpa[1] && !kpa[2] && !kpa[3] && !kpa[4] && !kpa[5])
      return AsyncTask::DS_cont;
    auto dt = ClockObject::get_global_clock()->get_dt();
    if(kpa[0])
      adj[1] = 10;
    if(kpa[1])
      adj[1] = -10;
    if(kpa[2])
      adj[0] = -10;
    if(kpa[3])
      adj[0] = 10;
    if(kpa[4])
      adj[2] = 2;
    if(kpa[5])
      adj[2] = -2;
    camera.set_pos(camera, adj * dt);
    return AsyncTask::DS_cont;
  }, key_pressed);
  framework.get_task_mgr().add(task);

  // Finally, space toggles map rotation
  // by 1000th of a circle per frame
  rotater = new GenericAsyncTask("rotater", [](GenericAsyncTask *, void *) {
    render.set_h(render.get_h() + 360.0/1000);
    return AsyncTask::DS_cont;
  }, 0);
  framework.define_key("space", "Rotate scene", EVH() {
    if(rotater->get_state() == AsyncTask::S_inactive)
      framework.get_task_mgr().add(rotater);
    else
      rotater->remove();
  }, 0);

  // Generate geometry node
  redraw_map();

  // run the framework's main loop
  framework.main_loop();
  framework.close_framework();
  return 0;
}
