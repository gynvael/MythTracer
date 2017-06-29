#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include "math3d.h"
#include "objreader.h"
#include "primitive_triangle.h"

namespace raytracer {

using math3d::V3D;

// Shamelessly taken from dev.krzaq.cc ;)
struct FileDeleter {
  void operator()(FILE *f) const {
    fclose(f);
  }
};

static bool ReadVertex(std::vector<V3D> *vertices, char *line) {
  double x, y, z;
  if (sscanf(line, "v %lf %lf %lf", &x, &y, &z) != 3) {
    fprintf(stderr, "warning: unsupported vertex format \"%s\"\n", line);
    return false;
  }
  //printf("%f %f %f\n", x, y, z);  

  vertices->push_back({x, y, z});
  return true;
}

static bool ReadNormals(std::vector<V3D> *normals, char *line) {
  double x, y, z;
  if (sscanf(line, "vn %lf %lf %lf", &x, &y, &z) != 3) {
    fprintf(stderr, "warning: unsupported normal format \"%s\"\n", line);
    return false;
  }

  normals->push_back({x, y, z});
  return true;
}

static bool ReadFace(
    OctTree *tree,
    const std::vector<V3D>& vertices,
    const std::vector<V3D>& normals,
    char *line) {
  // Can be one of four formats:
  // f v  ...
  // f v/vt ...
  // f v//vn ...
  // f v/vt/vn ...

  std::stringstream s(line);
  std::string token;
  s >> token;  // Ignore the first one.

  std::vector<int> vertex_indexes;
  std::vector<int> normal_indexes;
  std::vector<int> texcoord_indexes;

  while (s.good()) {
    s >> token;
    if (s.eof()) {
      break;
    }

    int v = 0, vt = 0, vn = 0;
    if (sscanf(token.c_str(), "%i/%i/%i", &v, &vt, &vn) != 3 &&
        sscanf(token.c_str(), "%i//%i", &v, &vn) != 2 &&
        sscanf(token.c_str(), "%i/%i", &v, &vt) != 2 &&
        sscanf(token.c_str(), "%i", &v) != 1) {
      fprintf(stderr, "warning: unsupported face format \"%s\"\n",
              token.c_str());
      return false;
    }

    // The indexes in the file are 1-based. Correct them.
    v -= 1; vt -= 1; vn -= 1;

    vertex_indexes.push_back(v);
    normal_indexes.push_back(vn);
    texcoord_indexes.push_back(vt);
  }

  if (vertex_indexes.size() != 3 && vertex_indexes.size() != 4) {
    fprintf(stderr, "warning: unsupported face count (%i)\n  %s\n",
            (int)vertex_indexes.size(), line);
    return false;
  }

  // If it's a quad, add duplicate the first vertex as an additional one.
  if (vertex_indexes.size() == 4) {
    vertex_indexes.push_back(vertex_indexes[0]);
    normal_indexes.push_back(normal_indexes[0]);
    texcoord_indexes.push_back(texcoord_indexes[0]);    
  }

  // Add triangle(s) to the scene.
  // First: 0 1 2
  // Second (if any): 2 3 0
  for (size_t i = 3; i <= vertex_indexes.size(); i += 2) {
    Triangle *tr = new Triangle();

    for (size_t j = 0; j < 3; j++) {
      tr->vertex[j] = vertices[vertex_indexes[i - 3 + j]];      
    }

    if (normal_indexes[i - 3 + 0] != 0 &&
        normal_indexes[i - 3 + 1] != 0 &&
        normal_indexes[i - 3 + 2] != 0) {
      for (size_t j = 0; j < 3; j++) {
        tr->normal[j] = normals[normal_indexes[i - 3 + j]];
      }    
    }
    
    // TODO(gynvael): Add texture coord and material support.
    tree->AddPrimitive(tr);  // Takes ownership of the Triangle object.
  }

  return true;
}

bool ReadObjFile(OctTree *tree, const char *fname) {
  std::unique_ptr<FILE, FileDeleter> f(fopen(fname, "r"));
  if (f == nullptr) {
    return true;
  }

  // Faces (triangles) can be directly added to the OctTree, but we need to
  // store vertices somewhere during processing.
  std::vector<V3D> vertices;
  std::vector<V3D> normals;

  while (true) {
    char line[128];
    if (fgets(line, sizeof(line), f.get()) == nullptr) {
      break;
    }

    char token[16]{};
    if (sscanf(line, "%15s", token) != 1) {
      // Skip empty lines.
      continue;
    }

    std::string token_str(token);

    // Ignore comments.
    if (token_str[0] == '#') {
      continue;
    }

    if (token_str == "v") {
      if (!ReadVertex(&vertices, line)) {
        return false;
      }
    } else if (token_str == "vn") {
      if (!ReadNormals(&normals, line)) {
        return false;
      }
    } else if (token_str == "vt") {
      // TODO(gynvael): Add material support.
    } else if (token_str == "f") {
      if (!ReadFace(tree, vertices, normals, line)) {
        return false;
      }
    } else if (token_str == "s") {
      // Ignore smoothness.
    } else if (token_str == "mtllib") {
      // TODO(gynvael): Add material support.
    } else if (token_str == "g") {
      // Ignore group.
    } else if (token_str == "usemtl") {
      // TODO(gynvael): Add material support.
    } else if (token_str == "o") {
      // Ignore object name.
    } else if (token_str == "g") {
      // Ignore group.
    } else {
      fprintf(stderr, "warning: unknown feature \"%s\"\n", token);
    }
  }

  return true;
}

};  // namespace raytracer


