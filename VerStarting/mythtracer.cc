#include <stdint.h>
#include <time.h>
#include <memory>
#include <vector>
#ifdef __unix__
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include <direct.h>
#endif
#include "camera.h"
#include "objreader.h"
#include "octtree.h"

using namespace raytracer;
using math3d::V3D;
using math3d::M4D;

const int W = 1920*2;  // 960 480
const int H = 1080*2;  // 540 270

const int MAX_RECURSION_LEVEL = 2;

V3D TraceRayWorker(Scene *scene, const Ray& ray, int level, int *line_no) {
  V3D intersection_point;
  V3D::basetype intersection_distance;
  auto primitive = scene->tree.IntersectRay(
      ray, &intersection_point, &intersection_distance);

  if (primitive == nullptr) {
    if (line_no != nullptr) {
      *line_no = -1;
    }

    // Background color.
    return { 0.75, 0.75, 0.75 };
  }

  if (line_no != nullptr) {
    *line_no = primitive->debug_line_no;
  }

  V3D normal = primitive->GetNormal(intersection_point);

  V3D::basetype normal_ray_dot = normal.Dot(-ray.direction);
  if (normal_ray_dot < 0.0) {
    normal = -normal;
    normal_ray_dot = normal.Dot(-ray.direction);
  }

  normal_ray_dot = (normal_ray_dot + 1.0) * 0.5;

  // If no other material information is available, use only the normal-ray dot
  // product.
  if (primitive->mtl == nullptr) {
    return { normal_ray_dot, normal_ray_dot, normal_ray_dot };
  }

  // Calculate the actual color.
  auto mtl = primitive->mtl;

  V3D ambient = mtl->ambient;
  if (mtl->tex) {
    V3D uvw = primitive->GetUVW(intersection_point);
    V3D tex_color = mtl->tex->GetColorAt(
        uvw.v[0], uvw.v[1], intersection_distance);
    ambient *= tex_color;   
  }

  V3D color = ambient * normal_ray_dot;  

  // Ray reflection.
  // http://paulbourke.net/geometry/reflected/
  if (level < MAX_RECURSION_LEVEL && mtl->reflectance > 0.0) {
    V3D reflected_direction =
        ray.direction - normal * (2 * ray.direction.Dot(normal));
    Ray reflected_ray{
        // TODO(gynvael): Pick a better epsilon.
        intersection_point + (reflected_direction * 0.0001),
        reflected_direction
    };

    color += TraceRayWorker(scene, reflected_ray, level + 1, nullptr) *
             mtl->reflectance;
  }

  return color;
}


V3D TraceRay(Scene *scene, const Ray& ray, int *line_no) {
  return TraceRayWorker(scene, ray, 0, line_no);
}

void V3DtoRGB(const V3D& v, uint8_t rgb[3]) {
  for (int i = 0; i < 3; i++) {
    rgb[i] = v.v[i] > 1.0 ? 255 :
             v.v[i] < 0.0 ? 0 :
             (uint8_t)(v.v[i] * 255);
  }
}

int main(void) {
  puts("Creating anim/ directory");
#ifdef __unix__
  mkdir("anim", 0700);
#else
  _mkdir("anim");
#endif

  printf("Resolution: %u %u\n", W, H);

  Scene scene;

  puts("Reading .OBJ file.");
  ObjFileReader objreader;
  if (!objreader.ReadObjFile(&scene, "../Models/Living Room USSU Design.obj")) {
    return -1;
  }
  
  puts("Finalizing tree.");
  scene.tree.Finalize();

  AABB aabb = scene.tree.GetAABB();
  printf("%f %f %f x %f %f %f\n",
      aabb.min.v[0],
      aabb.min.v[1],
      aabb.min.v[2],
      aabb.max.v[0],
      aabb.max.v[1],
      aabb.max.v[2]);


  std::vector<int> debug_line_numbers(W * H);
  std::vector<uint8_t> bitmap(W * H * 3);
  int frame = 0;
  for (double angle = 0.0; angle <= 360.0; angle += 1.0, frame++) {

  Camera cam{
    { 300.0, 57.0, 120.0 },
    10.0, 180.0, 0.0,
    120.0
  };

  puts("Rendering.");
  clock_t tm_start = clock();  
  Camera::Sensor sensor = cam.GetSensor(W, H);

  #pragma omp parallel  
  {
  #pragma omp for
  for (int j = 0; j < H; j++) {
    for (int i = 0; i < W; i++) {
      int debug_line_no = 0;
      V3D color = TraceRay(&scene, sensor.GetRay(i, j), &debug_line_no);
      V3DtoRGB(color, &bitmap[(j * W + i) * 3]);
      debug_line_numbers[j * W + i] = debug_line_no;
    }
    putchar('.'); fflush(stdout);
  }
  }

  clock_t tm_end = clock();
  float tm = (float)(tm_end - tm_start) / (float)CLOCKS_PER_SEC;
  printf("%.3fs\n", tm);

  puts("Writing");

  char fname[256];
  sprintf(fname, "anim/dump_%.5i.raw", frame);

  FILE *f = fopen(fname, "wb");
  fwrite(&bitmap[0], bitmap.size(), 1, f);
  fclose(f);

  sprintf(fname, "anim/dump_%.5i.txt", frame);
  f = fopen(fname, "w");
  int idx = 0;
  for (int j = 0; j < H; j++) {
    for (int i = 0; i < W; i++, idx++) {
      fprintf(f, "%i, %i: line %i\n", i, j, debug_line_numbers[idx]);
    }
  }
  fclose(f);

  }

  puts("Done");


  return 0;
}

