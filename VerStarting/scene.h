#pragma once
#include <vector>
#include "octtree.h"
#include "material.h"
#include "light.h"

namespace raytracer {

class Scene {
 public:
  OctTree tree;
  MaterialMap materials;
  TextureMap textures;
  std::vector<Light> lights;
};

};
