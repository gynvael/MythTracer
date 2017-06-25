#pragma once
#include <cmath>
#include <cstdio>
#include <iostream>

#include "math3d.h"

using math3d::V3D;
using math3d::ToStr;

namespace test {

template <typename T>
inline void TestErrorMsg(T a, T b, const char *str_a, int line) {
  std::cerr << "Test at line " << line << " failed: " << str_a << "\n"
            << "  was      : " << a << "\n"
            << "  should be: " << b << std::endl;  
}

template <typename T>
inline void TestEq(T a, T b, const char *str_a, int line) {
  if (a != b) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
inline void TestEq(double a, double b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
inline void TestEq(float a, float b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001f) {
    TestErrorMsg(a, b, str_a, line);
  }
}

template <>
inline void TestEq(long double a, long double b, const char *str_a, int line) {
  if (fabs(a - b) >= 0.0000001L) {
    TestErrorMsg(a, b, str_a, line);
  }
}

bool EqVectors(const V3D& a,
               const V3D& b,
               V3D::basetype epsilon);

template <>
inline void TestEq(V3D a, V3D b, const char *str_a, int line) {
  if (!EqVectors(a, b, 0.0000001)) {
    TestErrorMsg(a, b, str_a, line);
  }
}

#define TESTEQ(a, b) TestEq((a), (b), #a, __LINE__)

}  // namespace test
