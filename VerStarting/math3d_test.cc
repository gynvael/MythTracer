#include <cmath>
#include <cstdio>
#include <iostream>

#include "math3d.h"

using math3d::V3D;
using math3d::ToStr;

template <typename T>
void TestErrorMsg(T a, T b, const char *str_a, int line) {
  std::cerr << "Test at line " << line << " failed: " << str_a << "\n"
            << "  was      : " << a << "\n"
            << "  should be: " << b << std::endl;  
}

template <typename T>
void TestEq(T a, T b, const char *str_a, int line) {
  if (a != b) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
void TestEq(double a, double b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
void TestEq(float a, float b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001f) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
void TestEq(long double a, long double b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001L) {
    TestErrorMsg(a, b, str_a, line);
  }
}

bool EqVectors(const V3D& a,
               const V3D& b,
               V3D::basetype epsilon) {
  const V3D::basetype dx = fabs(a.x() - b.x());
  const V3D::basetype dy = fabs(a.y() - b.y());
  const V3D::basetype dz = fabs(a.z() - b.z());  
  return dx < epsilon && dy < epsilon && dz < epsilon;
}

template <>
void TestEq(V3D a, V3D b, const char *str_a, int line) {
  if (!EqVectors(a, b, 0.0000001)) {
    TestErrorMsg(a, b, str_a, line);
  }
}

#define TESTEQ(a, b) TestEq((a), (b), #a, __LINE__)

int main(void) {
  V3D a(1.0, 2.0, 3.0);
  TESTEQ(a, V3D(1.0, 2.0, 3.0));

  V3D b;
  TESTEQ(b, V3D(0.0, 0.0, 0.0));  

  b.x() = 4.0;
  b.y() = 5.0;
  b.z() = 6.0;
  TESTEQ(b, V3D(4.0, 5.0, 6.0));

  V3D c(a);
  TESTEQ(c, V3D(1.0, 2.0, 3.0));

  c = b;
  TESTEQ(c, V3D(4.0, 5.0, 6.0));  

  c = a;
  TESTEQ(c += a, V3D(2.0, 4.0, 6.0));

  c = a;
  TESTEQ(c -= a, V3D(0.0, 0.0, 0.0));

  c = a;
  TESTEQ(c *= a, V3D(1.0, 4.0, 9.0));

  c = a;
  TESTEQ(c /= a, V3D(1.0, 1.0, 1.0));

  c = a;
  TESTEQ(c *= 3.0, V3D(3.0, 6.0, 9.0));

  c = a;
  TESTEQ(c *= 3.0, V3D(3.0, 6.0, 9.0));

  c = a;
  TESTEQ(c + a, V3D(2.0, 4.0, 6.0));
  TESTEQ(c - a, V3D(0.0, 0.0, 0.0));
  TESTEQ(c * a, V3D(1.0, 4.0, 9.0));
  TESTEQ(c / a, V3D(1.0, 1.0, 1.0));
  TESTEQ(-c, V3D(-1.0, -2.0, -3.0));  
  TESTEQ(+c, V3D(1.0, 2.0, 3.0));    

  c = V3D(1.0, 0.0, 0.0);
  TESTEQ(c.Length(), 1.0);
  TESTEQ(c.SqrLength(), 1.0);

  c = V3D(0.0, 1.0, 0.0);
  TESTEQ(c.Length(), 1.0);
  TESTEQ(c.SqrLength(), 1.0);

  c = V3D(0.0, 0.0, 1.0);
  TESTEQ(c.Length(), 1.0);
  TESTEQ(c.SqrLength(), 1.0);  

  c = V3D(1.0, 2.0, 3.0);
  TESTEQ(c.Length(), 3.7416573867739413);  
  TESTEQ(c.SqrLength(), 14.0);  

  a = V3D(1.0, 1.0, 1.0);  
  b = V3D(2.0, 2.0, 2.0);
  TESTEQ(a.Distance(b), b.Distance(a));
  TESTEQ(a.Distance(b), 1.7320508075688772);

  a = V3D(1.0, 2.0, 3.0);  
  b = V3D(5.0, 4.0, 3.0);
  TESTEQ(a.Dot(b), b.Dot(a));
  TESTEQ(a.Dot(b), 22.0);

  TESTEQ(a.Cross(b), V3D(-6.0, 12.0, -6.0));
  TESTEQ(b.Cross(a), V3D(6.0, -12.0, 6.0));  

  a = V3D(1.0, 2.0, 3.0);  
  b = a;
  a.Norm();
  TESTEQ(a, V3D(0.2672612419124, 0.5345224838248, 0.8017837257372));
  TESTEQ(b.DupNorm(), V3D(0.2672612419124, 0.5345224838248, 0.8017837257372));

  return 0;
}

