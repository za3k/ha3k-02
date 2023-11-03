#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
typedef struct { color co; sc absorbtion; sc reflectivity; } material;
typedef struct { vec cp; material ma; sc r; } sphere;
typedef struct { sphere *spheres; int nn; } world;
typedef struct { vec start; vec dir; } ray; // dir is normalized!

/* Random sampling */

static sc random_double() { return rand() / (RAND_MAX + 1.0); } // [0, 1)
static vec random_vec() {
    vec v = { random_double(), random_double(), random_double() };
    return v;
}
static vec random_in_unit_sphere() {
    while (1) {
        vec v = random_vec();
        if (magsq(v) < 1) return v;
    }
}
static vec random_unit_vector() { return normalize(random_in_unit_sphere()); }

/* Ray-tracing */

static color BLACK = {0, 0, 0};
static color WHITE = {1.0, 1.0, 1.0};
static color BLUE = {0.5, 0.7, 1.0};

static color sky_color(ray rr) {
  sc a = 0.5 * (rr.dir.y + 1);
  return add(scale(WHITE, 1.0-a), scale(BLUE, a));
}

static color ray_color(world here, ray rr, int depth);

static color surface_color(world here, sphere *obj, ray rr, vec point, int depth) {
  vec normal = normalize(sub(point, obj->cp));
  ray bounce = { point, normalize(add(normal, random_unit_vector())) };
  if (depth <= 0) return BLACK;
  return scale(ray_color(here, bounce, depth-1), (1.0-obj->ma.absorbtion));
}

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

static color ray_color(world here, ray rr, int depth)
{
  sc intersection;
  sc nearest_t = 1/.0;
  sphere *nearest_object = 0;

  for (int i = 0; i < here.nn; i++) {
    if (find_nearest_intersection(rr, here.spheres[i], &intersection)) {
      if (intersection < 0.000001 || intersection >= nearest_t) continue;
      nearest_t = intersection;
      nearest_object = &here.spheres[i];
    }
  }

  if (nearest_object) {
    return surface_color(here, nearest_object, rr, add(rr.start, scale(rr.dir, nearest_t)), depth);
  }

  return sky_color(rr);
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


static ray get_ray(int w, int h, int x, int y) {
  // Camera is always at 0,0
  sc aspect = ((sc)w)/h; // Assume aspect >= 1
  sc viewport_height = 2.0;
  sc focal_length = 1.0; // Z distance of viewport
  sc viewport_width = viewport_height * aspect;

  sc pixel_width = (viewport_width / w);
  sc pixel_height = (viewport_height / h);
  sc left = viewport_width / -2.0;
  sc top = viewport_height / 2.0;

  sc px = left + (pixel_width * (x + random_double()));
  sc py = top - (pixel_height * (y + random_double()));

  vec pv = { px, py, focal_length };
  ray rr = { {0}, normalize(pv) };

  return rr;
}

static void render(world here, int w, int h)
{
  int samples_per_pixel = 10;
  int max_bounces = 10;

  output_header(w, h);
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++) {
      color pixel_color = {0, 0, 0};
      for (int sample = 0; sample < samples_per_pixel; ++sample) {
        ray rr = get_ray(w, h, j, i);
        pixel_color = add(pixel_color, ray_color(here, rr, max_bounces));
      }
      encode_color(scale(pixel_color, 1.0/samples_per_pixel));
    }
}

int main(int argc, char **argv) {
  sphere ss[4] = { 
    { .ma = { .co = {0, .5, 0}, .absorbtion = 0.5 }, .r = 100, .cp = {0, -100, 5} }, // Ground
    { .ma = { .co = {.5, 0, 0}, .absorbtion = 0.1 }, .r = 1, .cp = {-2, 1, 5} }, // Sphere 1
    { .ma = { .co = {0, .5, 0}, .absorbtion = 0.3 }, .r = 1, .cp = {0, 1, 5} }, // Sphere 2
    { .ma = { .co = {0, 0, .5}, .absorbtion = 0.5 }, .r = 1, .cp = {2, 1, 5} }, // Sphere 3
  };
  world here = { ss, 4 };
  render(here, 800, 600);
  return 0;
}
