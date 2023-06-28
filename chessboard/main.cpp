/*
 * Translation of Python chessboard sample from Panda3D.
 * https://github.com/panda3d/panda3d/tree/v1.10.13/samples/chessboard 
 *
 * Original C++ conversion by Thomas J. Moore June 2023
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang and Phil Saltzman
 * Models: Eddie Canaan
 * Last Updated: 2015-03-13
 *
 * This tutorial shows how to determine what objects the mouse is pointing to
 * We do this using a collision ray that extends from the mouse position
 * and points straight into the scene, and see what it collides with. We pick
 * the object with the closest collision
 */
#include <pandaFramework.h>
#include <collisionHandlerQueue.h>
#include <collisionRay.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <mouseWatcher.h>

#include "anim_supt.h"

namespace { // don't export/pollute the global namespace
// First we define some constants for the colors
const LColor BLACK(0, 0, 0, 1);
const LColor WHITE(1, 1, 1, 1);
const LColor HIGHLIGHT(0, 1, 1, 1);
const LColor PIECEBLACK(.15, .15, .15, 1);

// Global variables
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;
CollisionTraverser picker;
PT(CollisionNode) picker_node;
NodePath pickerNP;
PT(CollisionRay) picker_ray;
PT(CollisionHandlerQueue) pq;
NodePath square_root;
NodePath squares[64];
class Piece *pieces[64];
int hi_sq, dragging;

// Now we define some helper functions that we will need later

// This function, given a line (vector plus origin point) and a desired z value,
// will give us the point on the line where the desired z value is what we want.
// This is how we know where to position an object in 3D space based on a 2D mouse
// position. It also assumes that we are dragging in the XY plane.
//
// This is derived from the mathematical of a plane, solved for a given point
LPoint3 point_at_z(PN_stdfloat z, const LPoint3 &point, const LVector3 &vec)
{
    return point + vec * ((z - point.get_z()) / vec.get_z());
}

// A handy little function for getting the proper position for a given square1
LPoint3 square_pos(int i)
{
    return LPoint3((i % 8) - 3.5, i/8 - 3.5, 0);
}

// Helper function for determining whether a square should be white or black
// The modulo operations (%) generate the every-other pattern of a chess-board
LColor square_color(int i)
{
    return (i + (i/8)%2)%2 ? BLACK : WHITE;
}

// This needs to be known at this time, so it's declared here.  The Python
// code declared it at the bottom.
class Piece {
  public:
    Piece() {}
    virtual Piece *create(int square, const LColor &color) const = 0; // factory
    NodePath obj; // the model to render
    int square; // its current home
  protected:
    Piece *init(Piece *what, int square, const LColor &color) const;
    virtual const char * model() const { return nullptr; }
};
extern const Piece *pawn, *king, *queen, *bishop, *knight, *rook;

AsyncTask::DoneStatus process_mouse(GenericAsyncTask *, void *);
void setup_lights(void);
void grab_piece(const Event *, void *);
void release_piece(const Event *, void *);

void init(void)
{
    // Open framework (replaces ShowBase).  This will not set up
    // everything like ShowBase, but it does provide convenient functions
    // to do so.
    framework.open_framework();

    //Set the window title and open new window
    framework.set_window_title("Chessboard - C++ Panda3D Samples");
    window = framework.open_window();
    // This code puts the standard title and instruction text on screen
    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    // This code was modified slightly to match the other demos
    auto text_node = new TextNode("title");
    auto text = NodePath(text_node);
    text_node->set_text("Panda3D: Tutorial - Mouse Picking");
    // style=1: default
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_shadow_color(0.0f, 0.0f, 0.0f, 0.5f);
    text_node->set_shadow(0.04, 0.04); // baked into OnscreenText
    text.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_right);
    text.set_pos(1.0-0.2, 0, -1+0.05); // BottomRight == (1,0,-1)
    text.set_scale(0.07);

    text_node = new TextNode("instructions");
    text = NodePath(text_node);
    text_node->set_text("ESC: Quit");
    text.reparent_to(window->get_aspect_2d()); // a2d
    // style=1: default
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(-1+0.06, 0, 1-0.1); // TopLeft == (-1,0,1)
    text_node->set_align(TextNode::A_left);
    text.set_scale(0.05);

    text_node = new TextNode("instructions2");
    text = NodePath(text_node);
    text_node->set_text("Left-click and drag: Pick up and drag piece");
    text.reparent_to(window->get_aspect_2d()); // a2d
    text_node->set_align(TextNode::A_left);
    // style=1: default
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(-1+0.06, 0, 1-0.16); // TopLeft == (-1,0,1)
    text.set_scale(0.05);

    window->enable_keyboard();
    // Escape quits
#if 0
    framework.define_key("escape", "Quit", framework.event_esc, &framework);
#else
    // Or, default key bindings.  In particular, ? displays bindings; ESC quits
    // This adds debugging keys.
    framework.enable_default_keys();
#endif
    // Disble mouse camera control (unnecessary as it needs manual enable)
    NodePath camera = window->get_camera_group();
    camera.set_pos_hpr(0, -12, 8, 0, -35, 0);  // Set the camera
    setup_lights();  // Setup default lighting

    // Since we are using collision detection to do picking, we set it up like
    // any other collision detection system with a traverser and a handler
    //picker = new CollisionTraverser;  // Make a traverser (already in globals)
    pq = new CollisionHandlerQueue;  // Make a handler
    // Make a collision node for our picker ray
    picker_node = new CollisionNode("mouse_ray");
    // Attach that node to the camera since the ray will need to be positioned
    // relative to it
    pickerNP = camera.attach_new_node(picker_node);
    // Everything to be picked will use bit 1. This way if we were doing other
    // collision we could separate it
    picker_node->set_from_collide_mask(1<<1);
    picker_ray = new CollisionRay;  // Make our ray
    // Add it to the collision node
    picker_node->add_solid(picker_ray);
    // Register the ray as something that can cause collisions
    picker.add_collider(pickerNP, pq);
    // picker.showCollisions(render)

    // Now we create the chess board and its pieces

    // We will attach all of the squares to their own root. This way we can do the
    // collision pass just on the squares and save the time of checking the rest
    // of the scene
    square_root = window->get_render().attach_new_node("square_root");

    // For each square
    for(int i = 0; i < 64; i++) {
	// Load, parent, color, and position the model (a single square
	// polygon)
	squares[i] = def_load_model("models/square");
	squares[i].reparent_to(square_root);
	squares[i].set_pos(square_pos(i));
	squares[i].set_color(square_color(i));
	// Set the model itself to be collideable with the ray. If this model was
	// any more complex than a single polygon, you should set up a collision
	// sphere around it instead. But for single polygons this works
	// fine.
	squares[i].find("**/polygon").node()->set_into_collide_mask(1<<1);
	// Set a tag on the square's node so we can look up what square this is
	// later during the collision pass
	squares[i].find("**/polygon").node()->set_tag("square", std::to_string(i));
    }

    // We will use this variable as a pointer to whatever piece is currently
    // in this square

    // The order of pieces on a chessboard from white's perspective. This list
    // contains the constructor functions for the piece classes defined
    // below
    const Piece *const piece_order[] = { rook, knight, bishop, queen, king, bishop, knight, rook};

    for(int i = 8; i < 16; i++)
	// Load the white pawns
	pieces[i] = pawn->create(i, WHITE);
    for(int i = 48; i < 56; i++)
	// load the black pawns
	pieces[i] = pawn->create(i, PIECEBLACK);
    for(int i = 0; i < 8; i++) {
	// Load the special pieces for the front row and color them white
	pieces[i] = piece_order[i]->create(i, WHITE);
	// Load the special pieces for the back row and color them black
	pieces[i + 56] = piece_order[i]->create(i + 56, PIECEBLACK);
    }

    // This will represent the index of the currently highlited square
    hi_sq = -1;
    // This wil represent the index of the square where currently dragged piece
    // was grabbed from
    dragging = -1;

    // Start the task that handles the picking
    auto mouse_task = new GenericAsyncTask("mouse_task", process_mouse, (void*)window->get_mouse().node());
    framework.get_task_mgr().add(mouse_task);
    framework.define_key("mouse1", "grab", grab_piece, 0);  // left-click grabs a piece
    framework.define_key("mouse1-up", "release", release_piece, 0);  // releasing places it
}

// This function swaps the positions of two pieces
void swap_pieces(int fr, int to)
{
    auto temp = pieces[fr];
    pieces[fr] = pieces[to];
    pieces[to] = temp;
    if(pieces[fr]) {
	pieces[fr]->square = fr;
	pieces[fr]->obj.set_pos(square_pos(fr));
    }
    if(pieces[to]) {
	pieces[to]->square = to;
	pieces[to]->obj.set_pos(square_pos(to));
    }
}

// This is the task that deals with making everything interactive
AsyncTask::DoneStatus process_mouse(GenericAsyncTask* task, void* mouse_watcher_node)
{
    // This task deals with the highlighting and dragging based on the mouse

    // First, clear the current highlight
    if(hi_sq >= 0) {
	squares[hi_sq].set_color(square_color(hi_sq));
	hi_sq = -1;
    }

    // Check to see if we can access the mouse. We need it to do anything
    // else
    PT(MouseWatcher) mouse_watcher = DCAST(MouseWatcher, (PandaNode*)mouse_watcher_node);
    if (mouse_watcher->has_mouse()) {
	// get the mouse position
	auto mpos = mouse_watcher->get_mouse();

	// Set the position of the ray based on the mouse position
	picker_ray->set_from_lens(window->get_camera(0), mpos.get_x(), mpos.get_y());

	// If we are dragging something, set the position of the object
	// to be at the appropriate point over the plane of the board
	if(dragging >= 0) {
	    // Gets the point described by pickerRay.getOrigin(), which is relative to
	    // camera, relative instead to render
	    auto near_point = window->get_render().get_relative_point(
                    window->get_camera_group(), picker_ray->get_origin());
	    // Same thing with the direction of the ray
	    auto near_vec = window->get_render().get_relative_vector(
                    window->get_camera_group(), picker_ray->get_direction());
	    pieces[dragging]->obj.set_pos(
                    point_at_z(.5, near_point, near_vec));
	}

	// Do the actual collision pass (Do it only on the squares for
	// efficiency purposes)
	picker.traverse(square_root);
	if(pq->get_num_entries() > 0) {
	    // if we have hit something, sort the hits so that the closest
	    // is first, and highlight that node
	    pq->sort_entries();
	    int i = std::stoi(pq->get_entry(0)->get_into_node()->get_tag("square"));
	    // Set the highlight on the picked square
	    squares[i].set_color(HIGHLIGHT);
	    hi_sq = i;
	}
    }

    return AsyncTask::DS_cont;
}

void grab_piece(const Event *, void *)
{
    // If a square is highlighted and it has a piece, set it to dragging
    // mode
    if(hi_sq >= 0 && pieces[hi_sq]) {
	dragging = hi_sq;
	hi_sq = -1;
    }
}

void release_piece(const Event *, void *)
{
    // Letting go of a piece. If we are not on a square, return it to its original
    // position. Otherwise, swap it with the piece in the new square
    // Make sure we really are dragging something
    if(dragging >= 0) {
	// We have let go of the piece, but we are not on a square
	if(hi_sq < 0)
	    pieces[dragging]->obj.set_pos(square_pos(dragging));
	else
	    // Otherwise, swap the pieces
	    swap_pieces(dragging, hi_sq);

        // We are no longer dragging anything
        dragging = -1;
    }
}

void setup_lights()  // This function sets up some default lighting
{
    auto ambient_light = new AmbientLight("ambient_light");
    ambient_light->set_color(LColor(.8, .8, .8, 1));
    auto directional_light = new DirectionalLight("directional_light");
    directional_light->set_direction(LVector3(0, 45, -45));
    directional_light->set_color(LColor(0.2, 0.2, 0.2, 1));
    window->get_render().set_light(window->get_render().attach_new_node(directional_light));
    window->get_render().set_light(window->get_render().attach_new_node(ambient_light));
}

// Class for a piece. This just handles loading the model and setting initial
// position and color
// See class definition above; this is the default init function.
Piece *Piece::init(Piece *what, int square, const LColor &color) const
{
    what->square = square;
    what->obj = def_load_model(model());
    what->obj.reparent_to(window->get_render());
    what->obj.set_color(color);
    what->obj.set_pos(square_pos(square));
    return what;
}

// Classes for each type of chess piece
// Obviously, we could have done this by just passing a string to Piece's init.
// But if you wanted to make rules for how the pieces move, a good place to start
// would be to make an isValidMove(toSquare) method for each piece type
// and then check if the destination square is acceptible during ReleasePiece
const class Pawn : public Piece {
    const char *model() const { return "models/pawn"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new Pawn, square, color); }
} pawn_factory;
const Piece *pawn = &pawn_factory;

const class King : public Piece {
    const char *model() const { return "models/king"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new King, square, color); }
} king_factory;
const Piece *king = &king_factory;

const class Queen : public Piece {
    const char *model() const { return "models/queen"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new Queen, square, color); }
} queen_factory;
const Piece *queen = &queen_factory;

const class Bishop : public Piece {
    const char *model() const { return "models/bishop"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new Bishop, square, color); }
} bishop_factory;
const Piece *bishop = &bishop_factory;

const class Knight : public Piece {
    const char *model() const { return "models/knight"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new Knight, square, color); }
} knight_factory;
const Piece *knight = &knight_factory;

const class Rook : public Piece {
    const char *model() const { return "models/rook"; }
  public:
    Piece *create(int square, const LColor &color) const { return init(new Rook, square, color); }
} rook_factory;
const Piece *rook = &rook_factory;
}

int main(int argc, char **argv)
{
    if(argc > 1)
	sample_path = argv[1];
    init();
    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
