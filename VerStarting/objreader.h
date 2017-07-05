#pragma once
// Wavefront .obj 3D scene and .mtl material readers.
#include <memory>
#include <string>
#include <vector>
#include "scene.h"
#include "math3d.h"

namespace raytracer {

using math3d::V3D;

class ObjFileReader {
 public:
  bool ReadObjFile(Scene *scene, const char *fname);

 private:
  bool ReadVertex(const char *line);
  bool ReadNormals(const char *line);
  bool ReadUVW(const char *line);  
  bool ReadFace(const char *line);
  bool ReadMaterialLibrary(const char *line);
  bool ReadUseMaterial(const char *line);
  bool ReadNotImplemented(const char *);  

  std::string base_directory;
  Scene *scene;

  // Temporary storage used while processing.
  std::vector<V3D> vertices;
  std::vector<V3D> texcoords;  
  std::vector<V3D> normals;
  Material *selected_material;
  int line_no;
};

class MtlFileReader {
 public:
  bool ReadMtlFile(Scene *scene, const char *fname);
 
 private:
  void CommitMaterial();
  bool ReadNewMaterial(const char *line);
  bool ReadAmbient(const char *line);
  bool ReadDiffuse(const char *line);
  bool ReadSpecular(const char *line);
  bool ReadSpecularExp(const char *line);
  bool ReadReflectance(const char *line);
  bool ReadTransparancy(const char *line);
  bool ReadTransmissionFilter(const char *line);
  bool ReadRefractionIndex(const char *line);  
  bool ReadTexture(const char *line);
  bool ReadNotImplemented(const char *);

  Texture *GetTexture(const char *fname);

  std::string base_directory;
  Scene *scene;

  // Temporary storage used while processing.
  std::unique_ptr<Material> mtl;
  std::string mtl_name;
};

};  // namespace raytracer

