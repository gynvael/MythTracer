#include "aabb.h"

namespace raytracer {

bool AABB::FullyContains(const AABB& aabb) const {
  return Contains(aabb.min) && Contains(aabb.max);
}

bool AABB::Contains(const AABB& aabb) const {
  // Calculate the distance between the centers and compare vs width, height and
  // depth.
  auto this_pair = GetCenterWHD();
  auto aabb_pair = aabb.GetCenterWHD();

  auto& this_center = this_pair.first;
  auto& this_whd = this_pair.second;  

  auto& aabb_center = aabb_pair.first;
  auto& aabb_whd = aabb_pair.second;  

  return ((fabs(this_center.v[0] - aabb_center.v[0]) * 2.0 <=
          this_whd.v[0] + aabb_whd.v[0])) &&
         ((fabs(this_center.v[1] - aabb_center.v[1]) * 2.0 <=
          this_whd.v[1] + aabb_whd.v[1])) &&
         ((fabs(this_center.v[2] - aabb_center.v[2]) * 2.0 <=
          this_whd.v[2] + aabb_whd.v[2]));
}

bool AABB::Contains(const V3D& point) const {
  return point.v[0] >= min.v[0] && point.v[0] <= max.v[0] &&
         point.v[1] >= min.v[1] && point.v[1] <= max.v[1] &&
         point.v[2] >= min.v[2] && point.v[2] <= max.v[2];
}

void AABB::Extend(const AABB& aabb) {
  for (int i = 0; i < 3; i++) {
    min.v[i] = std::min(min.v[i], aabb.min.v[i]);
    max.v[i] = std::max(max.v[i], aabb.max.v[i]);
  }
}

void AABB::Extend(const V3D& point) {
  for (int i = 0; i < 3; i++) {
    min.v[i] = std::min(min.v[i], point.v[i]);
    max.v[i] = std::max(max.v[i], point.v[i]);
  }
}

std::pair<V3D, V3D> AABB::GetCenterWHD() const {
  return {
    min + (max - min) / 2,
    max - min
  };
}

}  // namespace raytracer
