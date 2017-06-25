#pragma once

#include "math3d.h"

namespace raytracer {

using math3d::V3D;

class Ray {
 public:
  Ray(V3D org, V3D dir) : origin(org), direction(dir) { }
  V3D origin;
  V3D direction;  // Assume and always make sure the direction vector is
                  // normalized.
};

}  // namespace raytracer

