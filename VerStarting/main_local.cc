#include <stdio.h>
#include <stdint.h>
#include <vector>
#ifdef __unix__
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include <direct.h>
#endif
#include "mythtracer.h"
#include "camera.h"
#include "octtree.h"

using math3d::V3D;
using raytracer::MythTracer;
using raytracer::AABB;
using raytracer::PerPixelDebugInfo;
using raytracer::Camera;
using raytracer::Light;
const int W = 1920/4;  // 960 480
const int H = 1080/4;  // 540 270


int main(void) {
  puts("Creating anim/ directory");
#ifdef __unix__
  mkdir("anim", 0700);
#else
  _mkdir("anim");
#endif

  printf("Resolution: %u %u\n", W, H);

  MythTracer mt;
  if (!mt.LoadObj("../Models/Living Room USSU Design.obj")) {
    return 1;
  }

  AABB aabb = mt.GetScene()->tree.GetAABB();
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
    if (frame <= 73) {
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
  mt.GetScene()->lights.clear();
  mt.GetScene()->lights.push_back(
      Light{
          { 231.82174, 81.69966, -27.78259 },
          { 0.3, 0.3, 0.3 },
          { 1.0, 1.0, 1.0 },
          { 1.0, 1.0, 1.0 }
  });

  mt.GetScene()->lights.push_back(
      Light{
          { 200, 80.0, 0 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });

  mt.GetScene()->lights.push_back(
      Light{
          { 200, 80.0, 80 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });

  mt.GetScene()->lights.push_back(
      Light{
          { 200, 80.0, 160 },
          { 0.0, 0.0, 0.0 },
          { 0.3, 0.3, 0.3 },
          { 0.3, 0.3, 0.3 }
  });

  /*raytracer::WorkChunk chunk{
    W, H,
    100, 100, 100, 100,
    &cam,
    &bitmap,
    nullptr
  };*/

  //mt.RayTrace(&chunk);

  mt.RayTrace(W, H, &cam, &bitmap);


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

  //break;
  }

  puts("Done");


  return 0;
}

