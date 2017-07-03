#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "math3d.h"
#include "texture.h"

namespace raytracer {

using math3d::V3D;

class Texture {
 public:
  // Retrieves the interpolated color for the uv location at the given distance.
  V3D GetColorAt(double u, double v, double distance) const;

  static Texture *LoadFromFile(const char *fname);

  size_t width = 0;
  size_t height = 0;
  std::vector<V3D> colors;
};

typedef std::unordered_map<std::string, std::unique_ptr<Texture>> TextureMap;

}  // namespace raytracer

