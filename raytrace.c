#include <math.h>
#include <stdio.h>

/* Types */
typedef float sc; // scalar
typedef struct { sc x, y, z; } vec;
typedef int bool;

/* Vectors */
static sc dot(vec aa, vec bb)   { return aa.x*bb.x + aa.y*bb.y + aa.z*bb.z; }
static sc magsq(vec vv)         { return dot(vv, vv); }
static vec scale(vec vv, sc c)  { vec rv = { vv.x*c, vv.y*c, vv.z*c }; return rv; }
static vec normalize(vec vv)    { return scale(vv, 1/sqrt(dot(vv, vv))); }
static vec add(vec aa, vec bb)  { vec rv = { aa.x+bb.x, aa.y+bb.y, aa.z+bb.z }; return rv; }
static vec sub(vec aa, vec bb)  { return add(aa, scale(bb, -1)); }

/* Ray-tracing types */
typedef vec color;              // So as to reuse dot(vv,vv) and scale
typedef struct { color co; sc reflectivity; char texture; } material;
typedef struct { vec cp; material ma; sc r; } sphere;
typedef struct { sphere *spheres; int nn; } world;
typedef struct { vec start; vec dir; } ray; // dir is normalized!

static color RED = { 255, 0, 0 };

/* Ray-tracing */

static bool find_nearest_intersection(ray rr, sphere ss, sc *intersection) {
  vec center_rel = sub(rr.start, ss.cp);
  // Quadratic coefficients of parametric intersection equation.  a == 1.
  sc b = 2*dot(center_rel, rr.dir), c = magsq(center_rel) - ss.r*ss.r;
  sc discrim = b*b - 4*c;
  if (discrim < 0) return 0;
  sc sqdiscrim = sqrt(discrim);
  *intersection = (-b - sqdiscrim > 0 ? (-b - sqdiscrim)/2
                                      : (-b + sqdiscrim)/2);
  return 1;
}

static color trace(world here, ray rr, sc importance)
{
  int ii;
  vec crap = { 0, 0, -0.5 };
  sc intersection;
  sc nearest_t = 1/.0;
  sphere *nearest_object = 0;

  for (ii = 0; ii < here.nn; ii++) {
    if (find_nearest_intersection(rr, here.spheres[ii], &intersection)) {
      if (intersection < 0 || intersection >= nearest_t) continue;
      nearest_t = intersection;
      nearest_object = &here.spheres[ii];
    }
  }

  //if (debug_distance && nearest_object) return grey(nearest_t-floor(nearest_t));

  if (nearest_object) {
    return RED;
  }

  return normalize(add(crap, rr.dir));
}

/* PPM6 */
/* PPM P6 file format; see <http://netpbm.sourceforge.net/doc/ppm.html> */

static void
output_header(int w, int h)
{ printf("P6\n%d %d\n255\n", w, h); }

static unsigned char
byte(double dd) { return dd > 1 ? 255 : dd < 0 ? 0 : dd * 255 + 0.5; }

static void
encode_color(color co)
{ putchar(byte(co.x)); putchar(byte(co.y)); putchar(byte(co.z)); }

/* Rendering */

static color pixel_color(world here, int w, int h, int x, int y) {
  // Camera is always at 0,0
  // Viewport is -.5 
  sc aspect = ((sc)w)/h; // Assume aspect >= 1
  sc viewport_height = 1.0;
  sc viewport_width = viewport_height * aspect;
  vec pv = { ((double)x/w - 0.5)*(viewport_width / 2.0), ((double)y/h - 0.5)*(viewport_height / 2.0), 1 };
  ray rr = { {0}, normalize(pv) };
  return trace(here, rr, 1.0);
}

static void render_pixel(world here, int w, int h, int x, int y) {
  encode_color(pixel_color(here, w, h, x, y));
}

static void render(world here, int w, int h)
{
  output_header(w, h);
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      render_pixel(here, w, h, j, i);
}

int main(int argc, char **argv)
{
  sphere ss[1] = { 
    { .ma = { .co = {0, .5, 0} }, .r = 1, .cp = {0, 0, 5} }
  };
  world here = { ss, 1 };
  render(here, 800, 600);
  return 0;
}
