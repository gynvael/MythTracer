#pragma once
#include <vector>
#include <stdint.h>
#include "camera.h"
#include "objreader.h"
#include "octtree.h"

namespace raytracer {
using math3d::V3D;

const int MAX_RECURSION_LEVEL = 5;

struct PerPixelDebugInfo { 
  int line_no;
  V3D point;
};

class WorkChunk {
 public:
  // Input.
  int image_width, image_height;
  int chunk_x, chunk_y;
  int chunk_width, chunk_height;
  Camera camera;

  // Note: Camera is not being serialized/deserialized.
  // TODO(gynvael): Mabe change this? It's somewhat weird.
  static const size_t kSerializedInputSize = 
    /* image_width */  sizeof(uint32_t) +
    /* image_height */ sizeof(uint32_t) +
    /* chunk_x */      sizeof(uint32_t) +
    /* chunk_y */      sizeof(uint32_t) +    
    /* chunk_width */  sizeof(uint32_t) +
    /* chunk_height */ sizeof(uint32_t);

  void SerializeInput(std::vector<uint8_t> *bytes);
  bool DeserializeInput(const std::vector<uint8_t>& bytes);

  // Output.
  std::vector<uint8_t> output_bitmap;
  std::vector<PerPixelDebugInfo> output_debug;

  // TODO(gynvael): Add PerPixelDebugInfo serialization.
  static const size_t kSerializedOutputMinimumSize = 
    /* number of bytes */ sizeof(uint32_t);
    /* followed by said amount of bytes */

  // Note: To deserialize WorkChunk output please be sure to fill in the
  // chunk_width and chunk_height fields.
  bool SerializeOutput(std::vector<uint8_t> *bytes);
  bool DeserializeOutput(const std::vector<uint8_t>& bytes);  

};

class MythTracer {
 public:
  Scene *GetScene();

  bool LoadObj(const char *fname);

  bool RayTrace(
      int image_width, int image_height, 
      Camera *camera,
      std::vector<uint8_t> *output_bitmap);
  
  bool RayTrace(WorkChunk *chunk);

 private:
  Scene scene;
  bool was_scene_finalized = false;

  V3D TraceRayWorker(
      const Ray& ray, int level,
      bool in_object,  // Used in transparency.
      V3D::basetype current_reflection_coef,
      PerPixelDebugInfo *debug);
  V3D TraceRay(const Ray& ray, PerPixelDebugInfo *debug);
  void V3DtoRGB(const V3D& v, uint8_t rgb[3]);
};

}  // namespace raytracer

