#pragma once
#include <memory>
#include <list>
#include <utility>
#include <vector>

#include "math3d.h"
#include "primitive.h"

namespace raytracer {

using math3d::V3D;

class OctTree {
 public:
  OctTree();

  // Adds a primitive to the temporary list. This method must not be callled
  // after the tree is finalized or otherwise the behaviour is undefined.
  // The OctTree becomes the new owner of the object and will call delete on
  // it when destructing.
  void AddPrimitive(Primitive *p);

  // Finalize the tree. It won't be possibel to add any new primitives, but it
  // will be possible to use the intersection methods.
  void Finalize();

  // Finds the closest ray-primitive intersection point and returns a pointer
  // to the primitive (the OctTree remains the owner of this pointer), the
  // intersection point and the distance between the ray origin and the
  // intersection point.
  // Returns nullptr in case the ray didn't intersect any primitives.
  const Primitive* IntersectRay(
      const Ray& ray,                  // Ray to check against.      
      V3D *point,                      // Intersection point.
      V3D::basetype *distance          // Distance to intersection.
  ) const;

  AABB GetAABB() const;

 private:
  // The minimum primitives required to make a split.
  static const int SPLIT_BOUNDARY = 16;

  // A node might either have both primitives or Nodes.
  // A node is not the owner of any of the objects that is contains pointers to.
  struct Node {
    std::vector<Primitive*> primitives;
    std::vector<Node> nodes;  // Always either 0 or 8 nodes.    

    V3D center;
    AABB aabb;

    void CalcCenter();
    void AttemptSplit();

    // Check is this node colides with the ray.
    bool NodeIntersectRay(const Ray& ray, V3D::basetype *dist) const;

    // Returns a primitive (if any) that intersects with the ray with the
    // lowest distance.
    const Primitive* PrimitiveIntersectRay(
        const Ray& ray, V3D *point, V3D::basetype *distance) const;
  };

  Node root;
  std::list<std::unique_ptr<Primitive>> primitives;
};

}  // namespace raytracer

