#pragma once
#include <memory>
#include <list>

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

 private:
  // TODO(gynvael): Maybe a vector of unique pointers?
  std::list<std::unique_ptr<Primitive>> primitives;

};

}  // namespace raytracer

