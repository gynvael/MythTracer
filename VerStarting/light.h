#pragma once
#include "math3d.h"

namespace raytracer {

using math3d::V3D;

class Light {
 public:
  V3D position;
  V3D ambient;
  V3D diffuse;  
  V3D specular;
};


};  // namespace raytracer

