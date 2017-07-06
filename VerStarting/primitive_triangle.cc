#include <algorithm>
#include <cmath>
#include <string>

#include "primitive_triangle.h"

namespace raytracer {

using math3d::V3D;

Triangle::~Triangle() {
}

AABB Triangle::GetAABB() const {
  return cached_aabb;
}

void Triangle::CacheAABB() {
  AABB aabb{vertex[0], vertex[0]};
  aabb.Extend(vertex[1]);
  aabb.Extend(vertex[2]);
  cached_aabb.min = aabb.min;
  cached_aabb.max = aabb.max;  
}

// http://www.mathopenref.com/heronsformula.html
static V3D::basetype AreaOfTriangle(
    V3D::basetype a, V3D::basetype b, V3D::basetype c) {
  V3D::basetype p = (a + b + c) / 2.0;
  V3D::basetype area_sqr = p * (p - a) * (p - b) * (p - c);

  // It seems that due to floating point inaccuracies it's possible to get a
  // negative result here when we are dealing with a triangle having all points
  // on the same line (i.e. with a zero size area).
  if (area_sqr < 0.0) {
    return 0.0;
  }

  return sqrt(area_sqr);
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

V3D Triangle::GetUVW(const V3D& point) const {
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

  return (uvw[0] * n0 + uvw[1] * n1 + uvw[2] * n2) / n;
}

bool Triangle::IntersectRay(const Ray& ray, V3D *point,
                            V3D::basetype *distance) const {
  // A quick ray-AABB(triangle) test that is faster than ray-triangle test 
  // itself, so it acts as a quick negative test.
  AABB aabb = GetAABB();    
  const V3D& dirfrac = ray.inv_direction;

  V3D::basetype t1 = (aabb.min.x() - ray.origin.x()) * dirfrac.x();
  V3D::basetype t2 = (aabb.max.x() - ray.origin.x()) * dirfrac.x();
  V3D::basetype t3 = (aabb.min.y() - ray.origin.y()) * dirfrac.y();
  V3D::basetype t4 = (aabb.max.y() - ray.origin.y()) * dirfrac.y();
  V3D::basetype t5 = (aabb.min.z() - ray.origin.z()) * dirfrac.z();
  V3D::basetype t6 = (aabb.max.z() - ray.origin.z()) * dirfrac.z();

  // If tmax is less than zero, ray (line) is intersecting AABB, but the whole
  // AABB is behind the ray.
  V3D::basetype tmax = std::min({
      std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});
  if (tmax < 0.0) {
    return false;
  }

  // If tmin is greater than tmax, ray doesn't intersect AABB.
  V3D::basetype tmin = std::max({
      std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
  if (tmin > tmax) {
    return false;
  }

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

  V3D::basetype final_distance = e2.Dot(qvec) * inv_det;
  if (final_distance < 0.0) {
    // Intersection is behind the camera.
    return false;
  }
  *distance = final_distance;  
  *point = ray.origin + ray.direction * *distance;
  return true;
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
