#include <memory>
#include "octtree.h"
#include "primitive_triangle.h"

using namespace raytracer;
using math3d::V3D;

const int W = 1920;
const int H = 1080;

unsigned char bitmap[W * H];

unsigned char TraceRay(OctTree *tree, const Ray& ray) {
  V3D intersection_point;
  V3D::basetype intersection_distance;
  auto primitive = tree->IntersectRay(
      ray, &intersection_point, &intersection_distance);

  if (primitive == nullptr) {
    return 192;
  }

  return 64;
}

int main(void) {

  OctTree tree;

  Triangle *tr = new Triangle();
  tr->vertex[0] = { 1, 1, 0 };
  tr->vertex[1] = { 1, 0, 0 };
  tr->vertex[2] = { 0, 0, 0 };
  tree.AddPrimitive(tr);
  tree.Finalize();

  V3D st(-.960, .540, 0.0);
  V3D en( .960,-.540, 0.0);
  V3D d = en - st;
  V3D::basetype dx = d.x() / W;
  V3D::basetype dy = d.y() / H;

  V3D cam_origin(0.0, 0.0, -5.0);

  V3D::basetype y = st.y();
  for (int j = 0; j < H; j++, y += dy) {
    V3D::basetype x = st.x();
    for (int i = 0; i < W; i++, x += dx) {
      V3D cam_dir(x, y, 1.0);
      cam_dir.Norm();

      Ray ray{cam_origin, cam_dir};

      bitmap[j * W + i] = TraceRay(&tree, ray);

    }
  }

  FILE *f = fopen("dump.raw", "wb");
  fwrite(bitmap, sizeof(bitmap), 1, f);
  fclose(f);

  return 0;
}

