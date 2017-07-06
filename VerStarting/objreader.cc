#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include "math3d.h"
#include "objreader.h"
#include "primitive_triangle.h"
#include "texture.h"

namespace raytracer {

using math3d::V3D;

// Deleter shamelessly taken from dev.krzaq.cc ;)
struct FileDeleter {
  void operator()(FILE *f) const {
    fclose(f);
  }
};

bool ObjFileReader::ReadNotImplemented(const char *) {
  return true;
}

bool ObjFileReader::ReadMaterialLibrary(const char *line) {
  char fname[256];
  if (sscanf(line, "mtllib %255[^\n]", fname) != 1) {
    fprintf(stderr, "warning: unsupported mtllib format \"%s\"\n", line);
    return false;
  }

  std::string path = base_directory.empty() ? 
      fname :
      base_directory + "/" + fname;

  MtlFileReader mtlreader;
  return mtlreader.ReadMtlFile(scene, path.c_str());
}

bool ObjFileReader::ReadVertex(const char *line) {
  double x, y, z;
  if (sscanf(line, "v %lf %lf %lf", &x, &y, &z) != 3) {
    fprintf(stderr, "warning: unsupported vertex format \"%s\"\n", line);
    return false;
  }

  vertices.push_back({x, y, z});
  return true;
}

bool ObjFileReader::ReadUVW(const char *line) {
  double u, v, w = 0.0;  // w is optional.
  if (sscanf(line, "vt %lf %lf %lf", &u, &v, &w) < 2) {
    fprintf(stderr, "warning: unsupported texcoord format \"%s\"\n", line);
    return false;
  }

  texcoords.push_back({u, v, w});
  return true;
}

bool ObjFileReader::ReadNormals(const char *line) {
  double x, y, z;
  if (sscanf(line, "vn %lf %lf %lf", &x, &y, &z) != 3) {
    fprintf(stderr, "warning: unsupported normal format \"%s\"\n", line);
    return false;
  }

  normals.push_back({x, y, z});
  return true;
}

bool ObjFileReader::ReadUseMaterial(const char *line) {
  char name[128];  
  // Note: There is a small chancee that some MTL/OBJ files might have these
  // names with spaces - in such case this code should be changed.
  if (sscanf(line, "usemtl %127s", name) != 1) {
    fprintf(stderr, "warning: unsupported newmtl format\n");
    return false;
  }

  auto mtl_itr = scene->materials.find(name);
  if (mtl_itr == scene->materials.end()) {
    fprintf(stderr, "warning: material \"%s\" not found\n", name);
    selected_material = nullptr;
    return true;  // Keep parsing.
  }

  selected_material = mtl_itr->second.get();
  return true;
}

bool ObjFileReader::ReadFace(const char *line) {
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

    // Add normals (if any).
    if (normal_indexes[i - 3 + 0] != -1 &&
        normal_indexes[i - 3 + 1] != -1 &&
        normal_indexes[i - 3 + 2] != -1) {
      for (size_t j = 0; j < 3; j++) {
        tr->normal[j] = normals[normal_indexes[i - 3 + j]];
      }    
    }

    // Add texture coordinates (if any).
    if (texcoord_indexes[i - 3 + 0] != -1 &&
        texcoord_indexes[i - 3 + 1] != -1 &&
        texcoord_indexes[i - 3 + 2] != -1) {
      for (size_t j = 0; j < 3; j++) {
        tr->uvw[j] = texcoords[texcoord_indexes[i - 3 + j]];
      }    
    }

    // Add material (if any).
    tr->mtl = selected_material;

    // Add debug information.
    tr->debug_line_no = line_no;

    // XXX
    tr->CacheAABB();

    // Scene takes ownership of the Triangle object.
    scene->tree.AddPrimitive(tr);
  }

  return true;
}

static std::string GetDirectoryPart(const std::string& path) {
  size_t found = path.find_last_of("/\\");
  if (found == std::string::npos) {
    return "";
  }

  return path.substr(0, found);
}

bool ObjFileReader::ReadObjFile(Scene *scene, const char *fname) {
  // Reset the temporary containers, just in case.
  vertices.resize(0);
  texcoords.resize(0);
  normals.resize(0);

  this->scene = scene;
  base_directory = GetDirectoryPart(fname);
  selected_material = nullptr;
  line_no = 0;

  // Parse the OBJ file.
  std::unique_ptr<FILE, FileDeleter> f(fopen(fname, "r"));
  if (f == nullptr) {
    fprintf(stderr, "error: file \"%s\" not found\n", fname);
    return false;
  }

  std::unordered_map<
    std::string, bool(ObjFileReader::*)(const char *)> handlers({
      { "v", &ObjFileReader::ReadVertex },
      { "vn", &ObjFileReader::ReadNormals },
      { "vt", &ObjFileReader::ReadUVW },
      { "v", &ObjFileReader::ReadVertex },
      { "f", &ObjFileReader::ReadFace },
      { "mtllib", &ObjFileReader::ReadMaterialLibrary },
      { "usemtl", &ObjFileReader::ReadUseMaterial },      
      { "s", &ObjFileReader::ReadNotImplemented },
      { "g", &ObjFileReader::ReadNotImplemented },
      { "o", &ObjFileReader::ReadNotImplemented }
  });  

  for (;;line_no++) {
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

    auto handler_itr = handlers.find(token_str);
    if (handler_itr == handlers.end()) {
      fprintf(stderr, "warning: unknown OBJ feature \"%s\"\n", token);
      continue;
    }

    if (!(this->*handler_itr->second)(line)) {
      return false;
    }
  }

  return true;
}


void MtlFileReader::CommitMaterial() {
  if (mtl != nullptr) {
    // Commit the current material.
    scene->materials[mtl_name] = std::move(mtl);
    mtl_name.clear();
  }  
}

bool MtlFileReader::ReadNotImplemented(const char *) {
  return true;
}

bool MtlFileReader::ReadNewMaterial(const char *line) {
  CommitMaterial();

  char name[128];  
  // Note: There is a small chancee that some MTL/OBJ files might have these
  // names with spaces - in such case this code should be changed.
  if (sscanf(line, "newmtl %127s", name) != 1) {
    fprintf(stderr, "warning: unsupported newmtl format\n");
    return false;
  }

  mtl.reset(new Material);
  mtl_name = name;
  return true;
}

bool MtlFileReader::ReadAmbient(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double r, g, b;
  if (sscanf(line, " Ka %lf %lf %lf", &r, &g, &b) != 3) {
    fprintf(stderr, "warning: unsupported Ka format \"%s\"\n", line);
    return false;
  }

  mtl->ambient = { r, g, b };  
  return true;
}

bool MtlFileReader::ReadDiffuse(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double r, g, b;
  if (sscanf(line, " Kd %lf %lf %lf", &r, &g, &b) != 3) {
    fprintf(stderr, "warning: unsupported Kd format \"%s\"\n", line);
    return false;
  }

  mtl->diffuse = { r, g, b };  
  return true;
}

bool MtlFileReader::ReadSpecular(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double r, g, b;
  if (sscanf(line, " Ks %lf %lf %lf", &r, &g, &b) != 3) {
    fprintf(stderr, "warning: unsupported Ks format \"%s\"\n", line);
    return false;
  }

  mtl->specular = { r, g, b };  
  return true;
}

bool MtlFileReader::ReadSpecularExp(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double value;
  if (sscanf(line, " Ns %lf", &value) != 1) {
    fprintf(stderr, "warning: unsupported Ns format \"%s\"\n", line);
    return false;
  }

  mtl->specular_exp = value;
  return true;
}

bool MtlFileReader::ReadReflectance(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double value;
  if (sscanf(line, " Refl %lf", &value) != 1) {
    fprintf(stderr, "warning: unsupported Refl format \"%s\"\n", line);
    return false;
  }

  mtl->reflectance = value;
  return true;
}

bool MtlFileReader::ReadTransparancy(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double value;
  if (sscanf(line, " Tr %lf", &value) != 1) {
    fprintf(stderr, "warning: unsupported Tr format \"%s\"\n", line);
    return false;
  }

  mtl->transparency = value;
  return true;
}

bool MtlFileReader::ReadRefractionIndex(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double value;
  if (sscanf(line, " Ni %lf", &value) != 1) {
    fprintf(stderr, "warning: unsupported Ni format \"%s\"\n", line);
    return false;
  }

  mtl->refraction_index = value;
  return true;
}

bool MtlFileReader::ReadTransmissionFilter(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  double r, g, b;
  if (sscanf(line, " Tf %lf %lf %lf", &r, &g, &b) != 3) {
    fprintf(stderr, "warning: unsupported Tf format \"%s\"\n", line);
    return false;
  }

  mtl->transmission_filter = { r, g, b };  
  return true;
}

Texture *MtlFileReader::GetTexture(const char *fname) {
  // If the texture has already been loaded before, just fetch it.
  auto tex_itr = scene->textures.find(fname);
  if (tex_itr != scene->textures.end()) {
    return tex_itr->second.get();
  }

  // Actually load the texture.
  std::string path = base_directory.empty() ? 
      fname :
      base_directory + "/" + fname;

  Texture *tex = Texture::LoadFromFile(path.c_str());
  if (tex == nullptr) {
    fprintf(stderr, "error: cannot load texture \"%s\"\n", fname);
    return nullptr;
  }
  
  scene->textures[fname].reset(tex);
  return tex;
}

bool MtlFileReader::ReadTexture(const char *line) {
  if (mtl == nullptr) {
    fprintf(stderr, "warning: material not ready; missing newmtl\n");
    return false;
  }

  char fname[256];
  if (sscanf(line, " map_Ka %255[^\n]", fname) != 1) {
    fprintf(stderr, "warning: unsupported map_ka format \"%s\"\n", line);
    return false;
  }

  mtl->tex = GetTexture(fname);
 
  return mtl->tex != nullptr;
}

bool MtlFileReader::ReadMtlFile(Scene *scene, const char *fname) {
  // Reset the temporary storage.
  mtl.reset();
  mtl_name.clear();

  this->scene = scene;
  base_directory = GetDirectoryPart(fname);  

  // Parse the MTL file.
  std::unique_ptr<FILE, FileDeleter> f(fopen(fname, "r"));
  if (f == nullptr) {
    fprintf(stderr, "error: file \"%s\" not found\n", fname);    
    return false;
  }

  std::unordered_map<
    std::string, bool(MtlFileReader::*)(const char *)> handlers({
      { "newmtl", &MtlFileReader::ReadNewMaterial },
      { "Ns", &MtlFileReader::ReadSpecularExp },
      { "Ni", &MtlFileReader::ReadRefractionIndex },
      { "d", &MtlFileReader::ReadNotImplemented },
      { "Tr", &MtlFileReader::ReadTransparancy },
      { "Refl", &MtlFileReader::ReadReflectance },      
      { "Tf", &MtlFileReader::ReadTransmissionFilter },
      { "illum", &MtlFileReader::ReadNotImplemented },
      { "Ka", &MtlFileReader::ReadAmbient },
      { "Kd", &MtlFileReader::ReadDiffuse },
      { "Ks", &MtlFileReader::ReadSpecular },
      { "Ke", &MtlFileReader::ReadNotImplemented },  // XXX light
      { "map_Ka", &MtlFileReader::ReadTexture },
      { "map_Kd", &MtlFileReader::ReadNotImplemented }
  });

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

    auto handler_itr = handlers.find(token_str);
    if (handler_itr == handlers.end()) {
      fprintf(stderr, "warning: unknown MTL feature \"%s\"\n", token);
      continue;
    }

    if (!(this->*handler_itr->second)(line)) {
      return false;
    }
  }

  // Commit the remaining material, if any.
  CommitMaterial();

  return true;
}

};  // namespace raytracer


