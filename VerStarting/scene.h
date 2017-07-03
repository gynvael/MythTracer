#pragma once
#include "octtree.h"
#include "material.h"

namespace raytracer {

class Scene {
 public:
  OctTree tree;
  MaterialMap materials;
  TextureMap textures;
};

};
