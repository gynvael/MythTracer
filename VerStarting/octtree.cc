#include "octtree.h"

namespace raytracer {

OctTree::OctTree() { }

void OctTree::AddPrimitive(Primitive *p) {
  primitives.push_back(std::unique_ptr<Primitive>(p));
  // TODO(gynvael): Calculate bounding box here.
}

void OctTree::Finalize() {
  // TODO(gynvael): Grow the tree.
}

const Primitive* OctTree::IntersectRay(
    const Ray& ray, V3D *point, V3D::basetype *distance) const {
  // XXX: This is a linear-search implementation. The tree will be added.
  // TODO(gynvael): Add a real tree optimization.
  // TODO(gynvael): Remove this linear implementation.
  const Primitive *closest_primitive = nullptr;
  V3D::basetype closest_distance{};
  V3D closest_point;
  for (const auto& p : primitives) {
    V3D intersection_point;
    V3D::basetype intersection_distance;
    if (!p->IntersectRay(ray, &intersection_point, &intersection_distance)) {
      continue;
    }

    // Calculate the distance and check if it's closer than the previously
    // discovered primitive (if any).
    if (closest_primitive != nullptr && 
        intersection_distance > closest_distance) {
      // Previously discovered was closer. Continue.
      continue;
    }

    // The newly discovered primitive is the closest.
    closest_primitive = p.get();
    closest_distance = intersection_distance;
    closest_point = intersection_point;
  }

  if (closest_primitive == nullptr) {
    return nullptr;
  }

  *point = closest_point;
  *distance = closest_distance;
  return closest_primitive;
}

}  // namespace raytracer

