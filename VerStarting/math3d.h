#pragma once
// Math types, classes and functions useful for 3D stuff.

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace math3d {

template <typename T>
class V3D_Base {
 public:
  typedef T basetype;

  V3D_Base() : v{} { };
  V3D_Base(T x, T y, T z) : v{x, y, z} { };  
  V3D_Base(const V3D_Base& a) : v{a.v[0], a.v[1], a.v[2]} { };

  V3D_Base operator+(const V3D_Base& b) const {
    return V3D_Base(v[0] + b.v[0], v[1] + b.v[1], v[2] + b.v[2]);
  }

  V3D_Base operator-(const V3D_Base& b) const {
    return V3D_Base(v[0] - b.v[0], v[1] - b.v[1], v[2] - b.v[2]);
  }

  V3D_Base operator-() const {
    return V3D_Base(-v[0], -v[1], -v[2]);
  }

  V3D_Base operator+() const {
    return *this;
  }

  V3D_Base& operator+=(const V3D_Base& b) {
    v[0] += b.v[0]; v[1] += b.v[1]; v[2] += b.v[2];
    return *this;
  }

  V3D_Base& operator-=(const V3D_Base& b) {
    v[0] -= b.v[0]; v[1] -= b.v[1]; v[2] -= b.v[2];
    return *this;
  }

  // The * and / operations between vectors do the * and / on separate scalar
  // values. See Dot and Cross methods for dot product and cross product.
  V3D_Base operator*(const V3D_Base& b) const {
    return V3D_Base(v[0] * b.v[0], v[1] * b.v[1], v[2] * b.v[2]);
  }

  V3D_Base operator/(const V3D_Base& b) const {
    return V3D_Base(v[0] / b.v[0], v[1] / b.v[1], v[2] / b.v[2]);
  }

  V3D_Base& operator*=(const V3D_Base& b) {
    v[0] *= b.v[0]; v[1] *= b.v[1]; v[2] *= b.v[2];
    return *this;
  }

  V3D_Base& operator/=(const V3D_Base& b) {
    v[0] /= b.v[0]; v[1] /= b.v[1]; v[2] /= b.v[2];
    return *this;
  }

  // Scalar operations.
  V3D_Base& operator*=(T b) {
    v[0] *= b; v[1] *= b; v[2] *= b;
    return *this;
  }

  V3D_Base& operator/=(T b) {
    v[0] /= b; v[1] /= b; v[2] /= b;
    return *this;
  }

  V3D_Base operator*(T n) const {
    return V3D_Base(v[0] * n, v[1] * n, v[2] * n);
  }

  V3D_Base operator/(T n) const {
    return V3D_Base(v[0] / n, v[1] / n, v[2] / n);
  }

  // Convenience functions.
  T SqrLength() const {
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  }

  T Length() const {
    return sqrt(SqrLength());
  }

  T SqrDistance(const V3D_Base& a) const {
    const T dx = a.v[0] - v[0];
    const T dy = a.v[1] - v[1];
    const T dz = a.v[2] - v[2];    
    return dx * dx + dy * dy + dz * dz;
  }

  T Distance(const V3D_Base& a) const {
    return sqrt(SqrDistance(a));
  }

  T Dot(const V3D_Base& a) const {
    return a.v[0] * v[0] + a.v[1] * v[1] + a.v[2] * v[2];
  }

  V3D_Base Cross(const V3D_Base& a) const {
    return V3D_Base(
        v[1] * a.v[2] - v[2] * a.v[1],
        v[2] * a.v[0] - v[0] * a.v[2],
        v[0] * a.v[1] - v[1] * a.v[0]
    );
  }

  void Norm() {
    const T l = Length();
    v[0] /= l; v[1] /= l; v[2] /= l;
  }

  V3D_Base DupNorm() const {
    const T l = Length();    
    return V3D_Base(v[0] / l, v[1] / l, v[2] / l);
  }

  template <typename U> 
  friend std::ostream& operator<<(std::ostream &os, const V3D_Base<U>& a);

  T v[3];

  // Getters acting as aliases.
  T& x() { return v[0]; }
  T& y() { return v[1]; }
  T& z() { return v[2]; }

  T& r() { return v[0]; }
  T& g() { return v[1]; }
  T& b() { return v[2]; }

  const T& x() const { return v[0]; }
  const T& y() const { return v[1]; }
  const T& z() const { return v[2]; }

  const T& r() const { return v[0]; }
  const T& g() const { return v[1]; }
  const T& b() const { return v[2]; }  
};

template <typename T>
std::ostream& operator<<(std::ostream &os, const V3D_Base<T>& a) {
  os << std::fixed << std::setprecision(5) 
     << a.v[0] << ", " << a.v[1] << ", " << a.v[2];
  return os;
}

template <typename T>
std::string ToStr(const T& a) {
  std::ostringstream s;
  s << a;
  return s.str();
}
#define V3DStr(a) math3d::ToStr(a).c_str()


typedef V3D_Base<double> V3D;

}  // namespace math3d


