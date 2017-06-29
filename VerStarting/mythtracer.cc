#include <memory>
#include "objreader.h"
#include "octtree.h"

using namespace raytracer;
using math3d::V3D;

const int W = 1920;  // 960 480
const int H = 1080;  // 540 270

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
  printf("Resolution: %u %u\n", W, H);

  OctTree tree;

  puts("Reading .OBJ file.");

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


  puts("Rendering.");

  V3D st(-9.60/8, 5.40/8, 0.0);
  V3D en( 9.60/8,-5.40/8, 0.0);
  V3D d = en - st;
  V3D::basetype dx = d.x() / W;
  V3D::basetype dy = d.y() / H;

  V3D cam_origin(300.0, 57.0, 120.0);

  V3D::basetype y = st.y();
  for (int j = 0; j < H; j++, y += dy) {
    V3D::basetype x = st.x();
    for (int i = 0; i < W; i++, x += dx) {
      V3D cam_dir(x, y, -1.0);
      cam_dir.Norm();

      Ray ray{cam_origin, cam_dir};

      bitmap[j * W + i] = TraceRay(&tree, ray);
    }
    putchar('.'); fflush(stdout);    
  }

  puts("Writing");

  FILE *f = fopen("dump.raw", "wb");
  fwrite(bitmap, sizeof(bitmap), 1, f);
  fclose(f);

  puts("Done");

  return 0;
}

