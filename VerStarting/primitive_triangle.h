#pragma once
#include <memory>
#include <string>
#include <utility>

#include "primitive.h"

namespace raytracer {

class Triangle : public Primitive {
 public:
  ~Triangle() override;
  AABB GetAABB() const override;
  bool IntersectRay(const Ray& ray, V3D *point,
                    V3D::basetype *distance) const override;
  V3D GetNormal(const V3D& point) const override;
  V3D GetUVW(const V3D& point) const override;

  std::string Serialize() const override;
  static bool Deserialize(
      std::unique_ptr<Triangle> *primitive,
      const std::string& data);

  void CacheAABB();

  V3D vertex[3]{};
  V3D normal[3]{};
  V3D uvw[3]{};
  AABB cached_aabb;
};

}  // namespace raytracer
