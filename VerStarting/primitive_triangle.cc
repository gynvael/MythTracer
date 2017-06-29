#include <algorithm>
#include <cmath>
#include <string>

#include "primitive_triangle.h"

namespace raytracer {

using math3d::V3D;

Triangle::~Triangle() {
}

AABB Triangle::GetAABB() const {
  AABB aabb{vertex[0], vertex[0]};
  aabb.Extend(vertex[1]);
  aabb.Extend(vertex[2]);  
  return aabb;
}

// http://www.mathopenref.com/heronsformula.html
static V3D::basetype AreaOfTriangle(
    V3D::basetype a, V3D::basetype b, V3D::basetype c) {
  V3D::basetype p = (a + b + c) / 2.0;
  return sqrt(p * (p - a) * (p - b) * (p - c));
}

// https://classes.soe.ucsc.edu/cmps160/Fall10/resources/barycentricInterpolation.pdf
V3D Triangle::GetNormal(const V3D& point) const {
  // Using barycentric interpolation. There might be a better / faster way to
  // do it.
  V3D::basetype a = vertex[0].Distance(vertex[1]);
  V3D::basetype b = vertex[1].Distance(vertex[2]);
  V3D::basetype c = vertex[2].Distance(vertex[0]);

  V3D::basetype p0 = point.Distance(vertex[0]);
  V3D::basetype p1 = point.Distance(vertex[1]);
  V3D::basetype p2 = point.Distance(vertex[2]);

  V3D::basetype n0 = AreaOfTriangle(b, p2, p1);
  V3D::basetype n1 = AreaOfTriangle(c, p0, p2);
  V3D::basetype n2 = AreaOfTriangle(a, p1, p0);

  V3D::basetype n = n0 + n1 + n2;

  return (normal[0] * n0 + normal[1] * n1 + normal[2] * n2) / n;
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
  if (*distance < 0.0) {
    // Intersection is behind the camera.
    return false;
  }
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
