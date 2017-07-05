#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "math3d.h"
#include "texture.h"

namespace raytracer {

using math3d::V3D;

class Material {
 public:
  // Ambient lightning for the object, as well as diffuse and specular 
  // reflectivity strength/color.
  // In MTL files: Ka, Kd and Ks respectively.
  V3D ambient{}, diffuse{}, specular{};

  // Pointer to a texture object that can be used together with ambient
  // to get the color.
  Texture *tex = nullptr;

  // The exponent for the specular reflectiveness. The higher the exponenta,
  // the more condense the blink is.
  // In MTL files: Ns.
  V3D::basetype specular_exp = 0.0;

  // The mirror-like effect of the surface. Zero means no reflection at all.
  // In MTL files: Refl (non-standard).
  V3D::basetype reflectance = 0.0;

  // The transparency of the the surface. Zero means full opaque.
  // In MTL files: Tr (or d as d=1-Tr).
  V3D::basetype transparency = 0.0;

  // TODO(gynvael): Add translucancy coeficient and diffused reflection.

  // The color filter of the material (only if it's translucent) using RGB
  // model. E.g. 0 1 1 means "green and blue go through, but red is removed
  // from the light color".
  // In MTL files: Tf.
  V3D transmission_filter{};

  // The index of refraction, i.e. how much does the light bend when entering
  // the surface.
  // In MTL files: Ni.
  V3D::basetype refraction_index = 0.0;
};

typedef std::unordered_map<std::string, std::unique_ptr<Material>> MaterialMap;

}  // namespace raytracer

