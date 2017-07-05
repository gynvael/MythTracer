#include <stdint.h>
#include <time.h>
#include <memory>
#include <vector>
#ifdef __unix__
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include <direct.h>
#endif
#include "camera.h"
#include "objreader.h"
#include "octtree.h"

using namespace raytracer;
using math3d::V3D;
using math3d::M4D;

const int W = 1920/4;  // 960 480
const int H = 1080/4;  // 540 270

const int MAX_RECURSION_LEVEL = 5;

struct PerPixelDebugInfo { 
  int line_no;
  V3D point;
};

V3D TraceRayWorker(
    Scene *scene, const Ray& ray, int level,
    bool in_object,  // Used in transparency.
    V3D::basetype current_reflection_coef,
    PerPixelDebugInfo *debug) {
  V3D intersection_point;
  V3D::basetype intersection_distance;
  auto primitive = scene->tree.IntersectRay(
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

  for (const auto& light : scene->lights) {
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
      auto shadow_primitive = scene->tree.IntersectRay(
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
        scene, reflected_ray,
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
        scene, refracted_ray,
        level + 1, !in_object,
        current_reflection_coef,
        nullptr) * mtl->transmission_filter * mtl->transparency;
  }

  return color;
}

V3D TraceRay(Scene *scene, const Ray& ray, PerPixelDebugInfo *debug) {
  return TraceRayWorker(scene, ray, 0, false, 1.0, debug);
}

void V3DtoRGB(const V3D& v, uint8_t rgb[3]) {
  for (int i = 0; i < 3; i++) {
    rgb[i] = v.v[i] > 1.0 ? 255 :
             v.v[i] < 0.0 ? 0 :
             (uint8_t)(v.v[i] * 255);
  }
}

int main(void) {
  puts("Creating anim/ directory");
#ifdef __unix__
  mkdir("anim", 0700);
#else
  _mkdir("anim");
#endif

  printf("Resolution: %u %u\n", W, H);

  Scene scene;

  puts("Reading .OBJ file.");
  ObjFileReader objreader;
  if (!objreader.ReadObjFile(&scene, "../Models/Living Room USSU Design.obj")) {
    return -1;
  }
  
  puts("Finalizing tree.");
  scene.tree.Finalize();

  AABB aabb = scene.tree.GetAABB();
  printf("%f %f %f x %f %f %f\n",
      aabb.min.v[0],
      aabb.min.v[1],
      aabb.min.v[2],
      aabb.max.v[0],
      aabb.max.v[1],
      aabb.max.v[2]);


  std::vector<PerPixelDebugInfo> debug(W * H);
  std::vector<uint8_t> bitmap(W * H * 3);
  int frame = 0;
  for (double angle = 0.0; angle <= 360.0; angle += 2.0, frame++) {

    // Skip some frames    
    if (frame <= 33) {
      continue;
    }

  // Really good camera setting.
  /*Camera cam{
    { 300.0, 57.0, 160.0 },
     0.0, 180.0, 0.0,
     110.0
  };*/

  // Camera set a lamp.
  /*Camera cam{
    { 250.0, 50.0, -20.0 },
    -90.0, 180.0, 0.0,
    110.0
  };*/

  Camera cam{
    { 300.0, 107.0, 40.0 },
     30.0, angle + 90, 0.0,
     110.0
  };

  // XXX: light at camera
  scene.lights.clear();
  scene.lights.push_back(
      Light{
          { 231.82174, 81.69966, -27.78259 },
          { 0.3, 0.3, 0.3 },
          { 1.0, 1.0, 1.0 },
          { 1.0, 1.0, 1.0 }
  });

  scene.lights.push_back(
      Light{
          { 200, 80.0, 0 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });

  scene.lights.push_back(
      Light{
          { 200, 80.0, 80 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });

  scene.lights.push_back(
      Light{
          { 200, 80.0, 160 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });  

  puts("Rendering.");
  clock_t tm_start = clock();  
  Camera::Sensor sensor = cam.GetSensor(W, H);

  #pragma omp parallel  
  {
  #pragma omp for
  for (int j = 0; j < H; j++) {
    for (int i = 0; i < W; i++) {
      V3D color = TraceRay(&scene, sensor.GetRay(i, j), 
          nullptr /* &debug[j * W + i] */);
      V3DtoRGB(color, &bitmap[(j * W + i) * 3]);
    }
    putchar('.'); fflush(stdout);
  }
  }

  clock_t tm_end = clock();
  float tm = (float)(tm_end - tm_start) / (float)CLOCKS_PER_SEC;
  printf("%.3fs\n", tm);

  puts("Writing");

  char fname[256];
  sprintf(fname, "anim/dump_%.5i.raw", frame);

  FILE *f = fopen(fname, "wb");
  fwrite(&bitmap[0], bitmap.size(), 1, f);
  fclose(f);

  /*sprintf(fname, "anim/dump_%.5i.txt", frame);
  f = fopen(fname, "w");
  int idx = 0;
  for (int j = 0; j < H; j++) {
    for (int i = 0; i < W; i++, idx++) {
      fprintf(f, "%i, %i: line %i  point %s\n",
          i, j, 
          debug[idx].line_no,
          V3DStr(debug[idx].point));
    }
  }
  fclose(f);
  */

  break;
  }

  puts("Done");


  return 0;
}

