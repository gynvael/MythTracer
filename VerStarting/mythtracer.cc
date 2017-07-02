#include <memory>
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

const int W = 1920/4;  // 960 480
const int H = 1080/4;  // 540 270

unsigned char bitmap[W * H];

unsigned char TraceRay(OctTree *tree, const Ray& ray) {
  V3D intersection_point;
  V3D::basetype intersection_distance;
  auto primitive = tree->IntersectRay(
      ray, &intersection_point, &intersection_distance);

  if (primitive == nullptr) {
    return 192;
  }

  V3D normal = primitive->GetNormal(intersection_point);
  
  V3D::basetype color = (normal.Dot(-ray.direction) + 1.0) * (255.0 / 2.0);
  if (color < 0) {
    color = 0;
  }

  if (color > 255.0) {
    color = 255.0;
  }

  return color;
}

int main(void) {
  puts("Creating anim/ directory");
#ifdef __unix__
  mkdir("anim", 0700);
#else
  _mkdir("anim");
#endif

  printf("Resolution: %u %u\n", W, H);

  puts("Reading .OBJ file.");

  OctTree tree;
  if (!ReadObjFile(&tree, "../Models/Living Room USSU Design.obj")) {
    return -1;
  }
  
  puts("Finalizing tree.");
  tree.Finalize();

  AABB aabb = tree.GetAABB();
  printf("%f %f %f x %f %f %f\n",
      aabb.min.v[0],
      aabb.min.v[1],
      aabb.min.v[2],
      aabb.max.v[0],
      aabb.max.v[1],
      aabb.max.v[2]);

  int frame = 0;
  for (double angle = 0.0; angle <= 360.0; angle += 1.0, frame++) {

  Camera cam{
    { 300.0, 57.0, 0.0 },
    10.0, angle, 0.0,
    100.0
  };

  Camera::Sensor sensor = cam.GetSensor(W, H);


  puts("Rendering.");
  #pragma omp parallel  
  {
  #pragma omp for
  for (int j = 0; j < H; j++) {
    for (int i = 0; i < W; i++) {
      bitmap[j * W + i] = TraceRay(&tree, sensor.GetRay(i, j));
    }
    putchar('.'); fflush(stdout);
  }
  }

  puts("Writing");

  char fname[256];
  sprintf(fname, "anim/dump_%.5i.raw", frame);

  FILE *f = fopen(fname, "wb");
  fwrite(bitmap, sizeof(bitmap), 1, f);
  fclose(f);

  }

  puts("Done");


  return 0;
}

