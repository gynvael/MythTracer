#include <cmath>
#include <cstdio>
#include <iostream>

#include "math3d.h"
#include "test_helper.h"

using namespace test;
using math3d::V3D;
using math3d::ToStr;

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

