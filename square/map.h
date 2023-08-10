/* Conversion of https://oldmanprogrammer.net/demos/square/map.js to Panda3D */
/* Original by Steve Baker.  Conversion by Thomas J. Moore */
/* Some of this code comes from my C++/OpenSceneGraph port done earlier */
#pragma once
#include <panda3d/geom.h> // also includes tons of other stuff.
// but not:
#include <panda3d/geomTriangles.h>
#include <panda3d/geomVertexWriter.h>

// Warning: rant ahead.  Skip this comment block if you don't care.
// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rs-using-directive
// says:  don't do this.  I suppose that's why C++ makes it as hard as
// pssible.  I mean, whould it have killed them to allow multiple ,-separated
// namespaces in the same directive, at least?  C++ likes to make me type
// more than necessary.  I am a fast typist, in spite of getting slower
// with age, but this also clutters up the code making it harder to read.
// If it weren't for syntax highlighting, it would be nearly impossible to
// read C++ the way they want it written.  Screw that whole document.  I
// don't subscribe to it.  std:: is ugly.  Ada, considered to be one of the
// most anal languages ever, makes it easier than this.  Of course having
// return type overloading and genuine type differentiation (typedef is only
// an alias, and creating a class just to have a new type is stupid)  pretty
// much eliminates namespace collisions.  I don't even need to qualify enum
// literals in Ada.
using namespace std;

/********************** from map.js ****************/
//http://en.wikipedia.org/wiki/Diamond-square_algorithm
// tjm - note: I have decided to use PN_stdfloat, and its assocated
// c*() versions of the standard C math functions.  There is really no
// advantage to doing it this way, but I felt like it.
// tjm - note: I prefer 4 space indents, but the original used 2 space, so
// I'm keeping that, at least.
int R(PN_stdfloat range)
{
  return cfloor(drand48() * range);
}

struct Map {
 private:
  vector<vector<PN_stdfloat>> grid; // mapsize * mapsize
 public:
  // Setters/getters are an artifact of poor language design, as far as I'm
  // concerned (Ada-like return type overloading and paren-less function calls
  // would make them obsolete, given C++ has refs for lvalue returns).
  // I guess Panda3d also likes them for its Python API interface.
  // This isn't public, so I'll just expose the vars.
  int mapsize = 64;
  int roughness = 5;

 private:
  const struct col { int r, g, b; }
    waterStart = {10,  20,  40},
    waterEnd   = {64,  110, 142},
    grassStart = {67,  100, 18},
    grassEnd   = {180, 200, 38},
    mtnStart   = {191, 180, 38},
    mtnEnd     = {60,  56,  31},
    rockStart  = {100, 100, 100},
    rockEnd    = {190, 190, 190}/*,
    snowEnd    = {200, 200, 200},
    snowStart  = {255, 255, 255}*/;

  PN_stdfloat displace(PN_stdfloat dim) {
    return (drand48()*(dim * 128 / mapsize)-(dim * 64 / mapsize))*roughness;
  }

  void mpd(int dim) {
    if (dim == 1) return;
    auto newdim = dim / 2;

    for(auto i = dim; i <= mapsize; i += dim) {
      for(auto j = dim; j <= mapsize; j += dim) {
	auto x = i - newdim;
	auto y = j - newdim;
	auto tl = grid[i-dim][j-dim];
	auto tr = grid[i%mapsize][j-dim];
	auto bl = grid[i-dim][j%mapsize];
	auto br = grid[i%mapsize][j%mapsize];
	grid[x][y] = normalize(((PN_stdfloat)(tl+tr+bl+br)/4)+displace(dim));
      }
    }
    for(auto i = dim; i <= mapsize; i += dim) {
      for(auto j = dim; j <= mapsize; j += dim) {
	auto x = i - newdim;
	auto y = j - newdim;
	auto cp = grid[x][y];
	auto tl = grid[i-dim][j-dim];
	auto tr = grid[i%mapsize][j-dim];
	auto bl = grid[i-dim][j%mapsize];
	auto br = grid[i%mapsize][j%mapsize];
	// top
	if ((j-dim)-newdim < 0)
	  grid[x][j-dim] = normalize((cp+tl+tr+grid[x][mapsize+((j-dim)-newdim)])/4 + displace(dim));
	else
	  grid[x][j-dim] = normalize((cp+tl+tr+grid[x][(j-dim)-newdim])/4 + displace(dim));

	//left
	if ((i-dim)-newdim < 0)
	  grid[i-dim][y] = normalize((cp+tl+bl+grid[mapsize+((i-dim)-newdim)][y])/4 + displace(dim));
	else
	  grid[i-dim][y] = normalize((cp+tl+bl+grid[(i-dim)-newdim][y])/4 + displace(dim));

	//bot
	grid[x][j] = normalize((cp+bl+br+grid[x][(j+newdim)%mapsize])/4 + displace(dim));

	//right
	grid[i][y] = normalize((cp+tr+br+grid[(i+newdim)%mapsize][y])/4 + displace(dim));
      }
    }
    mpd(newdim);
  }

  int normalize(PN_stdfloat n) {
    return max(0, min(255, (int)cfloor(n)));
  }

 public:
  void makemap(int roughness = -1,int mapsize = -1) {
    if (roughness > 0) this->roughness = roughness;
    if (mapsize > 0) this->mapsize = mapsize;
    grid.resize(this->mapsize + 1);
    for(auto x = 0; x <= this->mapsize; x++) {
      grid[x].resize(this->mapsize+1);
      for(auto y = 0; y <= this->mapsize; y++) {
	grid[x][y] = -1;
      }
    }
    grid[0][0] = 128+(R(50)-25);
    mpd(this->mapsize);
  }

 private:
  col fade(col startColor, col endColor, int steps, int step) {
    auto scale = (PN_stdfloat)step / steps;
    auto
      r = startColor.r + (int)cfloor(scale * (endColor.r - startColor.r)),
      b = startColor.b + (int)cfloor(scale * (endColor.b - startColor.b)),
      g = startColor.g + (int)cfloor(scale * (endColor.g - startColor.g));

    return {r, g, b};
  }

  CPT(GeomVertexFormat) format;
 public:
  // tjm: map.js:drawmap isn't used; instead, index.html does it.
  // Thus, this is mostly from index.html:
  PT(Geom) drawmap(void) {
    // partly from official fractal-plant sample, as converted to C++ by me.
    // The samples add colors in Vector4f format, which got internally converted
    // to cp, but that doesn't seem to work for me.  Instead, I add using
    // raw integers, using v3c4.  That also avoids int->float->int conversion
    // during array building.  This will probably fail on big-endian
    // machines, but for now, I don't care.  I'll have to do more research on
    // what panda3d does with the byte order (auto-convert or require user
    // intervention).  In spite of the comment in the pada3d docs, I'd have to
    // take endianness into consideration if I were to build a cp color, as
    // well, and, in the end, I don't care about DirectX compatibility so the
    // efficiency difference is irrelevant to me.
    if(format == nullptr) {
      auto fmt = new GeomVertexFormat(*GeomVertexFormat::get_v3c4());
      format = GeomVertexFormat::register_format(fmt);
    }
    PT(GeomVertexData) vdata = new GeomVertexData("body", format, Geom::UH_static);
    vdata->unclean_set_num_rows((mapsize + 1) * (mapsize + 1)); // preallocate
    GeomVertexWriter vert_writer(vdata, "vertex");
    GeomVertexWriter color_writer(vdata, "color");

    for(int y = 0; y < mapsize + 1; y++)
      for(int x = 0; x < mapsize + 1; x++) {
	auto g = grid[x%mapsize][y%mapsize];
	// tjm: seems better to replicate last
	if(x == mapsize) {
	  if(y == mapsize)
	    g = grid[x - 1][y - 1];
	  else
	    g = grid[x - 1][y];
	} else if(y == mapsize)
	    g = grid[x][y - 1];
	// center = (0,0)
	vert_writer.add_data3f(x - mapsize / 2,
			       y - mapsize / 2,
			       g < 51 ? 51 : g); // water is all at same level
	if(x == mapsize || y == mapsize) {
	  color_writer.add_data4i(1, 1, 1, 1); // don't care; just has to be there
	  continue;
	}
	struct col c;
	auto p = g / 255;
	if (p >= 0 && p <= 0.2) {
	  c = fade(waterStart, waterEnd, 30, cfloor(p* 100));
	} else if (p > 0.2 && p <= 0.7) {
	  c = fade(grassStart, grassEnd, 40, cfloor(p * 100) - 20);
	} else if (p > 0.7 && p <= 0.90) {
	  c = fade(mtnStart, mtnEnd, 20, cfloor(p * 100) - 70);
	} else if (p > 0.90 && p <= 1) {
	  c = fade(rockStart, rockEnd, 20, cfloor(p * 100) - 90);
	}
	color_writer.add_data4i(c.r, c.g, c.b, 255);
      }
    auto geom = new Geom(vdata);

    // I can't really use triangle strips due to coloring; see below.
    // There's no advantage to having a separate array for each row, so just 1.
    // If I ever change to using strips, this will change.
    PT(GeomTriangles) elements = new GeomTriangles(Geom::UH_static);
    // When flat shading, this is how GL behaves by default:
    // But setting this flag affects nothing.
//    elements->set_shade_model(Geom::SM_flat_last_vertex);

    // Convert each square into a pair of trianges
    // GL uses vertex #3 for color.
    // A B C  -> E B (A) D E (A) F C (B) E F (B)
    // D E F  -> H E (D) G H (D) I F (E) H I (E)
    // G H I
    // Tri strips use a different vertex for every color, so
    // it is not possible to share colors like above.  The colors
    // on the first two columns are ignored, and the next row can't
    // share vertices.
    // B D F -> A B (C) C B (D) C D (E) E D (F)
    // A C E -> G A (H) H A (C) H C (I) I C (E)
    // G H I                 C conflicts     E conflicts
    // Individual trangles requires no extra vertex data, but
    // increases vertex index arrays by ~3x (6 rather than 2 per square)
    // Strips would require 2x the vertex data, but 1/3 the indices.
    // For 32-bit floats and 32-bit colors, it's 4 32-bit words per vertex.
    // I assume that it's using 32-bit integers for vertex indices (although
    // it's possible that there is a hidden limit or swithc where it
    // uses 16-bit shorts instead).  For an NxN map, that's
    // tris:  (N+1)(N+1)*4 + N^2*6*1        = 10N^2+8N+4
    // strip: (N+1)(2*(N+1))*4 + N*(2N+1)*1 = 10N^2+13N+4
    // so, tris win out, but not by much.  If shorts are used,
    // tris win out by even more.
    for(int y = 0; y < mapsize; y++) {
      for(int x = 0; x < mapsize; x++) {
        // elements are indicies into vertex array
	// For OpenSceneGraph, I did the triangles ccw.  Panda3d says they
	// should be ccw as well, but when I view the results, half the
	// geometry is being face-culled.  I'll do it cw for now.
#if 0
	elements->add_vertices((y+1)*(mapsize + 1)+(x+1),  // E
			       (y+0)*(mapsize + 1)+(x+1),  // B
			       (y+0)*(mapsize + 1)+(x+0)); // A (color)
	elements->add_vertices((y+1)*(mapsize + 1)+(x+0),  // D
			       (y+1)*(mapsize + 1)+(x+1),  // E
			       (y+0)*(mapsize + 1)+(x+0)); // A (color)
#else
	elements->add_vertices((y+0)*(mapsize + 1)+(x+1),  // B
			       (y+1)*(mapsize + 1)+(x+1),  // E
			       (y+0)*(mapsize + 1)+(x+0)); // A (color)
	elements->add_vertices((y+1)*(mapsize + 1)+(x+1),  // E
			       (y+1)*(mapsize + 1)+(x+0),  // D
			       (y+0)*(mapsize + 1)+(x+0)); // A (color)
#endif
      }
    }
    elements->close_primitive();
    geom->add_primitive(elements);
    return geom;
  }
};
