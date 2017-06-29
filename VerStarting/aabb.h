#include "math3d.h"

namespace raytracer {

using math3d::V3D;

class AABB {
 public:
  bool FullyContains(const AABB& aabb) const;
  bool Contains(const AABB& aabb) const;
  bool Contains(const V3D& point) const;
  void Extend(const AABB& aabb);
  void Extend(const V3D& point);
  std::pair<V3D, V3D> GetCenterWHD() const;

  V3D min, max;
};

}  // namespace raytracer

