#include <stdio.h>
#include <stdint.h>
#include <vector>
#ifdef __unix__
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include <direct.h>
#endif
#include <thread>
#include <memory>
#include "mythtracer.h"
#include "camera.h"
#include "octtree.h"
#include "network.h"

using math3d::V3D;
using namespace raytracer;


int main(int argc, char **argv) {
  if (argc != 3) {
    puts("usage: mythtracer_worker <tag> <master_address>\n"
         "note : tag should have at most 8 characters");
    return 1;
  }


  MythTracer mt;
  if (!mt.LoadObj("../Models/Living Room USSU Design.obj")) {
    return 1;
  }

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

  NetSock::InitNetworking();

  std::string id(argv[1]);
  printf("Name of this worker: %s\n", id.c_str());

  for (;;) {
    puts("Connecting...");

    auto s = std::make_unique<NetSock>();
    if (!s->Connect(argv[2], 12345)) {
      printf("error: failed to connect to %s:12345\n", argv[2]);

      // Sleep.
      std::this_thread::sleep_for(1s);
      continue;
    }

    puts("Connected!");
    std::unique_ptr<netproto::NetworkProto> p;

    // Introduce to the server.
    p.reset(netproto::WorkerReady::Make(id));
    if (!netproto::SendPacket(s.get(), p.get())) {
      printf("error: disconnected when sending RDY!\n");
      fflush(stdout);
      std::this_thread::sleep_for(2s);      
      continue;
    }

    // Wait for work.
    Camera cam;
    for (;;) {
      p.reset(netproto::ReceivePacket(
            s.get(), netproto::kCommunicationSide::kWorker));
      if (p == nullptr) {
        printf("error: invalid proto or disconnected\n");
        fflush(stdout);
        std::this_thread::sleep_for(2s);      
        break;
      }

      if (p->GetTag() == "CAMR") {
        if (!cam.Deserialize(p->bytes)) {
          printf("error: failed to deserialize camera\n");
          fflush(stdout);
          std::this_thread::sleep_for(2s);      
          break;
        }

        printf("Received new camera settings:\n"
               "Position      : %f %f %f\n"
               "Pitch/yaw/roll: %f, %f, %f\n"
               "Angle of view : %f deg\n",
               cam.origin.v[0], cam.origin.v[1], cam.origin.v[2],
               cam.pitch, cam.yaw, cam.roll,
               cam.aov);
        fflush(stdout);
        continue;
      }

      if (p->GetTag() == "WORK") {
        WorkChunk work;
        if (!work.DeserializeInput(p->bytes)) {
          printf("error: failed to deserialize work chunk\n");
          fflush(stdout);
          std::this_thread::sleep_for(2s);
          break;
        }

        size_t sz = work.chunk_width * work.chunk_height;
        printf("Received work:\n"
               "Final resolution : %i x %i (24bpp)\n"
               "Chunk position   : %i, %i\n"
               "Chunk size       : %i x %i\n"
               "Initial ray count: %i rays\n",
               work.image_width, work.image_height,
               work.chunk_x, work.chunk_y,
               work.chunk_width, work.chunk_height,
               (int)sz);

        printf("Rendering"); fflush(stdout);
        work.output_bitmap.resize(sz * 3);
        work.camera = cam;
        if (!mt.RayTrace(&work)) {
          printf("error: failed while raytracing (weird); exiting\n");
          fflush(stdout);
          exit(1);
        }

        puts("Done! Sending chunk to master.");
        p.reset(netproto::WorkerRenderResult::Make(id, &work));
        if (!netproto::SendPacket(s.get(), p.get())) {
          printf("error: disconnected when sending PXLS\n");
          fflush(stdout);
          std::this_thread::sleep_for(2s);      
          break;
        }

        printf("Sent! %i points to %s!\n", (int)sz, id.c_str());
      }
    }
  }

  return 0;
}

