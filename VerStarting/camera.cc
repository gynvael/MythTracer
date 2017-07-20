#include "camera.h"
#include <cstring>

namespace raytracer {

using math3d::V3D;
using math3d::M4D;

V3D Camera::GetDirection() const {
  // Note: Roll is skipped as it wouldn't change the vector anyway.
  V3D dir{0.0, 0.0, 1.0};
  return
    M4D::RotationYDeg(yaw) *
    M4D::RotationXDeg(pitch) * dir;
}

Camera::Sensor Camera::GetSensor(int width, int height) const {
  Sensor s;
  s.width = width;
  s.height = height;
  s.cam = this;
  s.Reset();

  return s;
}

void Camera::Sensor::Reset() {
  // Calculate the vertival AOV angle.
  auto aov_vertical = (V3D::basetype(height) / V3D::basetype(width)) * cam->aov;

  // Calculate the frustum based on AOE.
  M4D rot_left = M4D::RotationYDeg(cam->aov / 2.0);
  M4D rot_right = M4D::RotationYDeg(-cam->aov / 2.0);
  M4D rot_top = M4D::RotationZDeg(aov_vertical / 2.0);
  M4D rot_bottom = M4D::RotationZDeg(-aov_vertical / 2.0);

  M4D rot_left_top = rot_top * rot_left;
  M4D rot_right_top = rot_bottom * rot_right;
  M4D rot_left_bottom = rot_bottom * rot_left;  

  V3D dir{0.0, 0.0, 1.0};

  V3D frustum_top_left = rot_left_top * dir;
  V3D frustum_top_right = rot_right_top * dir;
  V3D frustum_bottom_left = rot_left_bottom * dir;

  // Rotate the frustum in the direction of the camera.
  M4D frustum_rotation = 
      M4D::RotationYDeg(cam->yaw) *
      M4D::RotationXDeg(cam->pitch) * 
      M4D::RotationZDeg(cam->roll);

  frustum_top_left = frustum_rotation * frustum_top_left;
  frustum_top_right = frustum_rotation * frustum_top_right;
  frustum_bottom_left = frustum_rotation * frustum_bottom_left;  

  // Calculate horizontal and vertical deltas.
  delta_scanline =
    (frustum_bottom_left - frustum_top_left) / V3D::basetype(height);
  delta_pixel =
    (frustum_top_right - frustum_top_left) / V3D::basetype(width);
  start_point = frustum_top_left;
}

Ray Camera::Sensor::GetRay(int x, int y) const {
  V3D direction = start_point + (delta_scanline * y) + (delta_pixel * x);
  direction.Norm();
  return { cam->origin, direction };
}

void Camera::Serialize(std::vector<uint8_t> *bytes) {
  bytes->resize(kSerializedSize);

  // TODO(gynvael): Make this sane, plz.
  uint8_t *ptr = &(*bytes)[0];
  memcpy(ptr, &origin, sizeof(V3D)); ptr += sizeof(V3D);
  memcpy(ptr, &pitch, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(ptr, &yaw, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(ptr, &roll, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(ptr, &aov, sizeof(V3D::basetype));
}

bool Camera::Deserialize(const std::vector<uint8_t>& bytes) {
  if (bytes.size() != kSerializedSize) {
    return false;
  }

  // TODO(gynvael): Make this sane, plz.
  const uint8_t *ptr = &bytes[0];
  memcpy(&origin, ptr, sizeof(V3D)); ptr += sizeof(V3D);
  memcpy(&pitch, ptr, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(&yaw, ptr, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(&roll, ptr, sizeof(V3D::basetype)); ptr += sizeof(V3D::basetype);
  memcpy(&aov, ptr, sizeof(V3D::basetype));
  return true;
}

}  // namespace raytracer

