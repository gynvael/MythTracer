#pragma once
// Wavefront .obj 3D scene reader.
#include "octtree.h"

namespace raytracer {

using raytracer::OctTree;

bool ReadObjFile(OctTree *tree, const char *fname);

};  // namespace raytracer

