#include <algorithm>
#include <string>

#include "primitive_triangle.h"

namespace raytracer {

using math3d::V3D;

Triangle::~Triangle() {
}

std::pair<V3D, V3D> Triangle::GetAABB() const {
  return std::make_pair(
      V3D(
        std::min({vertex[0].v[0], vertex[1].v[0], vertex[2].v[0]}),
        std::min({vertex[0].v[1], vertex[1].v[1], vertex[2].v[1]}),
        std::min({vertex[0].v[2], vertex[1].v[2], vertex[2].v[2]})
      ),
      V3D(
        std::min({vertex[0].v[0], vertex[1].v[0], vertex[2].v[0]}),
        std::min({vertex[0].v[1], vertex[1].v[1], vertex[2].v[1]}),
        std::min({vertex[0].v[2], vertex[1].v[2], vertex[2].v[2]})
      )
  );
}

bool Triangle::IntersectRay(const Ray& ray, V3D *point,
                            V3D::basetype *distance) const {
  // Moller-Trumbore intersection algorithm, as presented on Wikipedia.
  V3D e1 = vertex[1] - vertex[0];
  V3D e2 = vertex[2] - vertex[0];
  V3D pvec = ray.direction.Cross(e2);
  V3D::basetype det = e1.Dot(pvec);

  // Check if ray is parallel to the plane.
  if (det >= -0.00000001 && det < 0.00000001) {
    return false;
  }

  V3D::basetype inv_det = 1.0 / det;
  V3D tvec = ray.origin - vertex[0];
  V3D::basetype u = tvec.Dot(pvec) * inv_det;
  if (u < 0.0 || u > 1.0) {
    return false;
  }

  V3D qvec = tvec.Cross(e1);
  V3D::basetype v = ray.direction.Dot(qvec) * inv_det;
  if (v < 0.0 || u + v > 1.0) {
    return false;
  }

  *distance = e2.Dot(qvec) * inv_det;
  *point = ray.origin + ray.direction * *distance;
  return true;
}

Ray Triangle::ReflectionRay(const Ray& ray) const {
  (void)ray;
  return Ray(V3D(), V3D()); // TODO
}

Ray Triangle::RefractionRay(const Ray& ray) const {
  (void)ray;
  return Ray(V3D(), V3D());  // TODO
}

std::string Triangle::Serialize() const{
  return "nope"; // TODO
}

bool Triangle::Deserialize(
    std::unique_ptr<Triangle> *primitive,
    const std::string& data) {
  (void)primitive;
  (void)data;
  return false; // TODO
}

}  // namespace raytracer
