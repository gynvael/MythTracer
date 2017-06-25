#pragma once
#include <memory>
#include <string>
#include <utility>

#include "primitive.h"

namespace raytracer {

class Triangle : public Primitive {
 public:
  ~Triangle() override;
  std::pair<V3D, V3D> GetAABB() const override;
  bool IntersectRay(const Ray& ray, V3D *point,
                    V3D::basetype *distance) const override;
  Ray ReflectionRay(const Ray& ray) const override;
  Ray RefractionRay(const Ray& ray) const override;
  std::string Serialize() const override;
  static bool Deserialize(
      std::unique_ptr<Triangle> *primitive,
      const std::string& data);

  V3D vertex[3];
};

}  // namespace raytracer
