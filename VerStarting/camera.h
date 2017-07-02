#pragma once
#include "math3d.h"
#include "ray.h"

namespace raytracer {

using math3d::V3D;
using math3d::M4D;

class Camera {
 public:
  class Sensor {
   public:
    Ray GetRay(int x, int y) const;

   private:
    void Reset();
    V3D delta_scanline;
    V3D delta_pixel;
    V3D start_point;

    // Set by parent.
    int width, height;
    const Camera *cam;   

    friend Camera;
  };

  V3D origin;
  V3D::basetype pitch, yaw, roll;  // X Y and Z accis.
  V3D::basetype aov;  // Angle of view, in degrees.

  V3D GetDirection() const;
  Sensor GetSensor(int width, int height) const;  
};

}  // namespace raytracer

