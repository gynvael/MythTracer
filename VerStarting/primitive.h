#pragma once
#include <memory>
#include <string>
#include <utility>

#include "aabb.h"
#include "math3d.h"
#include "ray.h"
#include "material.h"

namespace raytracer {

class Primitive {
 public:
  virtual ~Primitive() { };

  // Returns the axis-aligned bounding box of the primitive.
  virtual AABB GetAABB() const = 0;

  // Returns true if the primitive intersected with the given ray, as well as
  // the point of intersection and distance from ray origin to the intersection
  // point.
  virtual bool IntersectRay(const Ray& ray, V3D *point,
                            V3D::basetype *distance) const = 0;

  // Returns normal in the specified point.
  virtual V3D GetNormal(const V3D& point) const = 0;

  // Returns texture coords (UVW mapping) at the specified point.
  virtual V3D GetUVW(const V3D& point) const = 0;

  // Return serialized primitive.
  // TODO(gynvael): Actually provide an implementation of this to dump all the
  // common properties. Perhaps also add a deserialize method or function.
  virtual std::string Serialize() const = 0;
  // Note: Each implementation should have a Deserialize method:
  // static bool Deserialize(
  //     std::unique_ptr<T> *primitive, const std::string& data);

  // Common primitive properties go here.
  Material *mtl = nullptr;  // The primitive is not the owner of this object.
  int debug_line_no = 0;  // Line in the input file (if any) where this
                          // primitive was defined.
};

}  // namespace raytracer
