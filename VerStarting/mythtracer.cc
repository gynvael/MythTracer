#include <stdint.h>
#include <time.h>
#include <memory>
#include <limits>
#include <cstring>

#include "mythtracer.h"

using namespace raytracer;
using math3d::V3D;
using math3d::M4D;

V3D MythTracer::TraceRayWorker(
    const Ray& ray, int level,
    bool in_object,  // Used in transparency.
    V3D::basetype current_reflection_coef,
    PerPixelDebugInfo *debug) {
  V3D intersection_point;
  V3D::basetype intersection_distance;
  auto primitive = scene.tree.IntersectRay(
      ray, &intersection_point, &intersection_distance);

  if (primitive == nullptr) {
    if (debug != nullptr) {
      debug->line_no = -1;
      debug->point = { NAN, NAN, NAN /* Batman! */ };
    }

    // Background color.
    return { 0.0, 0.0, 0.0 };
  }

  if (debug != nullptr) {
    debug->line_no = primitive->debug_line_no;
    debug->point = intersection_point;
  }

  V3D normal = primitive->GetNormal(intersection_point);

  V3D towards_camera = -ray.direction;
  V3D::basetype normal_ray_dot = normal.Dot(towards_camera);
  if (normal_ray_dot < 0.0) {
    normal = -normal;
    normal_ray_dot = normal.Dot(towards_camera);
  }

  // If no other material information is available, use only the normal-ray dot
  // product.
  if (primitive->mtl == nullptr) {
    normal_ray_dot = (normal_ray_dot + 1.0) * 0.5;    
    return { normal_ray_dot, normal_ray_dot, normal_ray_dot };
  }

  // Calculate the actual color.
  // Based on https://en.wikipedia.org/wiki/Phong_reflection_model
  auto mtl = primitive->mtl;

  V3D surface_color = mtl->ambient;
  if (mtl->tex) {
    V3D uvw = primitive->GetUVW(intersection_point);
    V3D tex_color = mtl->tex->GetColorAt(
        uvw.v[0], uvw.v[1], intersection_distance);
    surface_color *= tex_color;   
  }

  // Ray reflection.
  // http://paulbourke.net/geometry/reflected/
  V3D reflected_direction =
      ray.direction - normal * (2 * ray.direction.Dot(normal));
  Ray reflected_ray{
      // TODO(gynvael): Pick a better epsilon.
      intersection_point + (reflected_direction * 0.0001),
      reflected_direction
  };

  V3D color{};

  for (const auto& light : scene.lights) {
    V3D light_direction = light.position - intersection_point;
    light_direction.Norm();

    // Ambient light is always effective.
    color += light.ambient *
             surface_color;    

    // Cast a ray between the intersection point and the light to determine
    // whether the light affects the given point (or whether the point is in
    // the shadow).
    // Traverse through all transparent or translucent surfaces.
    V3D light_power{1.0, 1.0, 1.0};
    bool in_shadow = false;

    bool traversing_through_object = false;
    for (V3D start_point = intersection_point;;) {
      Ray shadow_ray{
        // TODO(gynvael): Pick a better epsilon.  
        start_point + (light_direction * 0.00001),
        light_direction
      };

      V3D::basetype light_distance = 
        start_point.Distance(light.position);

      V3D shadow_intersection_point;
      V3D::basetype shadow_distance;
      auto shadow_primitive = scene.tree.IntersectRay(
          shadow_ray, &shadow_intersection_point, &shadow_distance);

      if (shadow_primitive == nullptr) {
        // Nothing found. Done.
        break;
      }

      // Perhaps the light was closer.
      if (shadow_distance > light_distance) {
        // Primitive was behind the light source.
        break;
      }

      // If the primitive is not transparent, then we are in a shadow.
      if (shadow_primitive->mtl->transparency == 0.0) {
        light_power = { 0.0, 0.0, 0.0 };
        in_shadow = true;        
        break;
      }


      // Some light passes through.
      if (!traversing_through_object) {
        light_power *= shadow_primitive->mtl->transmission_filter *
                       shadow_primitive->mtl->transparency;
      }

      traversing_through_object = !traversing_through_object;

      // Change the starting point and continue.
      start_point = shadow_intersection_point + (light_direction * 0.0000001);

      // There is an unlikely event that the new starting point is actually
      // behind the light. In such case, break.
      if (intersection_point.SqrDistance(start_point) >
          intersection_point.SqrDistance(light.position)) {
        // Already behind the light. No more shadow opportunities.
        break;
      }

      // If the light power is below the ambient threashold, just stop here and
      // mark as shadow.
      if (light_power.v[0] <= 0.001 &&
          light_power.v[1] <= 0.001 &&
          light_power.v[2] <= 0.001) {
        light_power = { 0.0, 0.0, 0.0 };
        in_shadow = true;
        break;
      }
    }

    // Actually do use ambient for light power.
    light_power.v[0] = std::max(light_power.v[0], light.ambient.v[0]);
    light_power.v[1] = std::max(light_power.v[1], light.ambient.v[1]);
    light_power.v[2] = std::max(light_power.v[2], light.ambient.v[2]);

    color += mtl->diffuse *
             surface_color *
             light_direction.Dot(normal) *
             light.diffuse * 
             light_power;

    if (!in_shadow) {
      auto refl_dot = reflected_direction.Dot(towards_camera);
      if (refl_dot > 0) {
        color += mtl->specular *
                 surface_color *
                 pow(refl_dot, mtl->specular_exp) *
                 light.specular;
      }
    }
  }

  // Reflection.
  if (level < MAX_RECURSION_LEVEL && 
      mtl->reflectance > 0.0 &&
      current_reflection_coef > 0.01 &&
      !in_object) {
    color += TraceRayWorker(
        reflected_ray,
        level + 1, in_object, current_reflection_coef * mtl->reflectance,
        nullptr) * mtl->reflectance;
  }

  // Refration.
  if (level < MAX_RECURSION_LEVEL && mtl->transparency > 0.0) {
    V3D::basetype refraction_index = mtl->refraction_index;
    if (in_object) {
      //refraction_index = 1.0 / refraction_index;
    }

    V3D::basetype partial_res = 
        1.0 - refraction_index * refraction_index * (
            1.0 - normal_ray_dot * normal_ray_dot);

    // Due to floating point inacuracies this might be below zero. Saturate
    // at zero in that case.
    if (partial_res < 0.0) {
      partial_res = 0.0;
    }

    V3D refracted_direction = ray.direction;
    // TODO(gynvael): Fix this math.
        //normal * (refraction_index * normal_ray_dot - sqrt(partial_res)) -
        //ray.direction * refraction_index;
    refracted_direction.Norm();

    Ray refracted_ray{
        // TODO(gynvael): Pick a better epsilon.
        intersection_point + refracted_direction * 0.00001,
        refracted_direction
    };

    color += TraceRayWorker(
        refracted_ray,
        level + 1, !in_object,
        current_reflection_coef,
        nullptr) * mtl->transmission_filter * mtl->transparency;
  }

  return color;
}

V3D MythTracer::TraceRay(
    const Ray& ray, PerPixelDebugInfo *debug) {
  return TraceRayWorker(ray, 0, false, 1.0, debug);
}

void MythTracer::V3DtoRGB(const V3D& v, uint8_t rgb[3]) {
  for (int i = 0; i < 3; i++) {
    rgb[i] = v.v[i] > 1.0 ? 255 :
             v.v[i] < 0.0 ? 0 :
             (uint8_t)(v.v[i] * 255);
  }
}

Scene *MythTracer::GetScene() {
  return &scene;
}

bool MythTracer::LoadObj(const char *fname) {
  puts("Reading .OBJ file.");
  ObjFileReader objreader;
  if (!objreader.ReadObjFile(&scene, fname)) {
    return false;
  }

  was_scene_finalized = false;
  return true;
}

bool MythTracer::RayTrace(
    int image_width, int image_height, 
    Camera *camera,
    std::vector<uint8_t> *output_bitmap) {
  WorkChunk chunk{
      image_width, image_height,
      0, 0, image_width, image_height,
      *camera, {}, {}
  };
  chunk.output_bitmap.resize(image_width * image_height * 3);

  bool res = RayTrace(&chunk);
  if (!res) {
    return false;
  }

  *output_bitmap = std::move(chunk.output_bitmap);

  return true;
}


bool MythTracer::RayTrace(WorkChunk *chunk) {
  if (!was_scene_finalized) {
    puts("Finalizing tree.");    
    scene.tree.Finalize();
    was_scene_finalized = true;
  }
  
  puts("Rendering.");
  const clock_t tm_start = clock();  
  Camera::Sensor sensor = chunk->camera.GetSensor(
      chunk->image_width, chunk->image_height);  

  #pragma omp parallel
  {
  #pragma omp for
  for (int j = 0; j < chunk->chunk_height; j++) {
    for (int i = 0; i < chunk->chunk_width; i++) {
      V3D color = TraceRay(
          sensor.GetRay(chunk->chunk_x + i, chunk->chunk_y + j),
          !chunk->output_debug.empty() ? 
            &chunk->output_debug[j * chunk->chunk_width + i] : nullptr);
      V3DtoRGB(color, &chunk->output_bitmap[(j * chunk->chunk_width + i) * 3]);
    }
    putchar('.'); fflush(stdout);
  }
  }

  const clock_t tm_end = clock();
  const float tm = (float)(tm_end - tm_start) / (float)CLOCKS_PER_SEC;
  printf("%.3fs\n", tm);

  return true;
}

void WorkChunk::SerializeInput(std::vector<uint8_t> *bytes) {
  bytes->resize(kSerializedInputSize);

  // TODO(gynvael): Make this sane.
  uint8_t *ptr = &(*bytes)[0];

  uint32_t u_image_width = image_width;
  uint32_t u_image_height = image_height;
  uint32_t u_chunk_x = chunk_x;
  uint32_t u_chunk_y = chunk_y;
  uint32_t u_chunk_width = chunk_width;
  uint32_t u_chunk_height = chunk_height;

  memcpy(ptr, &u_image_width, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &u_image_height, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &u_chunk_x, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &u_chunk_y, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &u_chunk_width, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &u_chunk_height, sizeof(uint32_t)); 
}

bool WorkChunk::DeserializeInput(const std::vector<uint8_t>& bytes) {
  if (bytes.size() != kSerializedInputSize) {
    return false;
  }

  uint32_t u_image_width;
  uint32_t u_image_height;
  uint32_t u_chunk_x;
  uint32_t u_chunk_y;
  uint32_t u_chunk_width;
  uint32_t u_chunk_height;

  // TODO(gynvael): Make this sane.
  const uint8_t *ptr = &bytes[0];
  memcpy(&u_image_width, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(&u_image_height, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(&u_chunk_x, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(&u_chunk_y, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(&u_chunk_width, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(&u_chunk_height, ptr, sizeof(uint32_t));

  // A set of constraints.
  // TODO(gynvael): Break this up, add error messages. Here and everywhere else.
  if (u_image_width > 100000 ||
      u_image_height > 100000 ||
      u_chunk_x > u_image_width ||
      u_chunk_y > u_image_height ||
      u_chunk_width > u_image_width ||
      u_chunk_height > u_image_height ||
      u_chunk_x + u_chunk_width > u_image_width ||
      u_chunk_y + u_chunk_height > u_image_height ||
      u_image_width == 0 ||
      u_image_height == 0 ||
      u_chunk_width == 0 ||
      u_chunk_height == 0) {
    return false;
  }

  image_width = u_image_width;
  image_height = u_image_height;
  chunk_x = u_chunk_x;
  chunk_y = u_chunk_y;
  chunk_width = u_chunk_width;
  chunk_height = u_chunk_height;

  return true;
}

bool WorkChunk::SerializeOutput(std::vector<uint8_t> *bytes) {
  if (output_bitmap.size() > std::numeric_limits<uint32_t>::max()) {
    fprintf(stderr, "error: too large WorkerChunk, cannot serialize\n");
    return false;
  }

  uint32_t sz = output_bitmap.size();

  // TODO(gynvael): Make this sane.
  bytes->resize(sizeof(uint32_t) + sz);
  uint8_t *ptr = &(*bytes)[0];
  memcpy(ptr, &sz, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  memcpy(ptr, &output_bitmap[0], output_bitmap.size());
  return true;
}

bool WorkChunk::DeserializeOutput(const std::vector<uint8_t>& bytes) {
  if (bytes.size() < kSerializedOutputMinimumSize) {
    return false;
  }

  // TODO(gynvael): Make this sane.
  const uint8_t *ptr = &bytes[0];

  uint32_t sz;
  memcpy(&sz, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

  uint64_t partial_chunk_sz = (uint64_t)chunk_width * (uint64_t)chunk_height;
  if (sz / 3 != partial_chunk_sz || sz % 3 != 0) {
    return false;
  }

  uint64_t chunk_sz = partial_chunk_sz * 3;
  // TODO(gynvael): This check is probably redundant. Remove if so.
  if (chunk_sz != sz) {
    return false;
  }

  if (chunk_sz > std::numeric_limits<size_t>::max()) {
    return false;
  }

  output_bitmap.resize(chunk_sz);
  memcpy(&output_bitmap[0], ptr, chunk_sz);

  return true;
}

