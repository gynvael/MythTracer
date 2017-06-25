#include "test_helper.h"

namespace test {

bool EqVectors(const V3D& a,
               const V3D& b,
               V3D::basetype epsilon) {
  const V3D::basetype dx = fabs(a.x() - b.x());
  const V3D::basetype dy = fabs(a.y() - b.y());
  const V3D::basetype dz = fabs(a.z() - b.z());  
  return dx < epsilon && dy < epsilon && dz < epsilon;
}

}  // namespace test
