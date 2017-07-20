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
#include <mutex>
#include <unordered_map>
#include <list>
#include <time.h>
#include "mythtracer.h"
#include "camera.h"
#include "octtree.h"
#include "network.h"

using math3d::V3D;
using namespace raytracer;
const int W = 1920;  // 960 480
const int H = 1080;  // 540 270
const int CHUNK_W = 128;
const int CHUNK_H = 128;

class ReadyWorkChunk {
 public:
  std::unique_ptr<WorkChunk> work;
  std::string id;  // Who has done the work.
};

std::mutex g_work_available_guard;
std::list<unique_ptr<WorkChunk>> g_work_available;

std::mutex g_work_finished_guard;
std::list<unique_ptr<ReadyWorkChunk>> g_work_finished;

// https://stackoverflow.com/questions/10890242/get-the-status-of-a-stdfuture
/*template<typename R>
  bool is_ready(std::future<R> const& f)
  { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
*/


void CommitWorkChunk(WorkChunk *work, const std::string& id) {
  auto ready = std::make_unique<ReadyWorkChunk>();
  ready->work.reset(work);
  ready->id = id;

  std::lock_guard<std::mutex> lock(g_work_finished_guard);
  g_work_finished.push_back(std::move(ready));
}

void ReturnWorkChunk(WorkChunk *work_ptr) {
  puts("Returning work to queue.");
  std::lock_guard<std::mutex> lock(g_work_available_guard);
  std::unique_ptr<WorkChunk> work(work_ptr);
  g_work_available.push_back(std::move(work));
}

WorkChunk *GetWorkChunk() {
  // Work might not be available immediately.
  WorkChunk *work = nullptr;
  for (;;)
  {
    {
      std::lock_guard<std::mutex> lock(g_work_available_guard);
      if (!g_work_available.empty()) {
        work = g_work_available.front().release();  
        g_work_available.pop_front();
        break;
      }
    }
    
    std::this_thread::sleep_for(100ms);
  }

  return work;
}

void WorkerHandler(NetSock *sock) {
  unique_ptr<NetSock> s(sock);
  char addr[32]{};
  sprintf(addr, "%s:%u", s->GetStrIP(), s->GetPort());

  // Handle initial RDY! packet.
  std::unique_ptr<netproto::NetworkProto> p(
      netproto::ReceivePacket(s.get(), netproto::kCommunicationSide::kMaster));
  if (p == nullptr) {
    printf("WH:%s: invalid proto or disconnected before RDY\n", addr);
    fflush(stdout);
    return;
  }

  if (p->GetTag() != "RDY!") {
    printf("WH:%s: expected RDY!, got %s\n", addr, p->GetTag().c_str());
    fflush(stdout);
    return;
  }

  std::string id = p->id;
  p.reset();

  printf("WH:%s is %s\n", addr, id.c_str());

  // Until the worker disconnects or times out, send it stuff to do.
  // Unless there is no more stuff to do.
  for (;;) {
    struct WorkReturner {
      void operator()(WorkChunk *work) const {
        ReturnWorkChunk(work);
      };
    };
    std::unique_ptr<WorkChunk, WorkReturner> work(GetWorkChunk());
    if (work == nullptr) {
      // Done!
      break;
    }

    // Send camera.
    p.reset(netproto::MasterSetCamera::Make(id, &work->camera));
    if (!netproto::SendPacket(s.get(), p.get())) {
      printf("WH:%s: failed to send or disconnected\n", id.c_str());
      fflush(stdout);
      return;
    }

    // Send work chunk.
    p.reset(netproto::MasterRenderOrder::Make(id, work.get()));
    if (!netproto::SendPacket(s.get(), p.get())) {
      printf("WH:%s: failed to send or disconnected\n", id.c_str());
      fflush(stdout);
      return;
    }

    printf("WH:%s: camera and work sent\n", id.c_str());
    fflush(stdout);

    // Wait for response.
    p.reset(netproto::ReceivePacket(s.get(), 
                                    netproto::kCommunicationSide::kMaster));
    if (p == nullptr) {
      printf("WH:%s: invalid proto or disconnected\n", id.c_str());
      fflush(stdout);
      return;
    }

    if (p->GetTag() != "PXLS") {
      printf("WH:%s: expected PXLS, got %s\n", id.c_str(), p->GetTag().c_str());
      fflush(stdout);
      return;
    }

    if (!work->DeserializeOutput(p->bytes)) {
      printf("WH:%s: failed to deserialize PXLS\n", id.c_str());
      fflush(stdout);
      return;
    }

    printf("WH:%s: sent in pixels!\n", id.c_str());
    fflush(stdout);

    CommitWorkChunk(work.release(), id);
  }

  printf("WH:%s: no more work, disconnecting!\n", addr);
  fflush(stdout);
}

void ConnectionHandler(NetSock *server_sock) {
  std::unique_ptr<NetSock> server(server_sock);

  printf("CH: Listening at: %s:%u\n",
      server->GetStrBindIP(), server->GetBindPort());
  fflush(stdout);

  // TODO: Add a future/promise to know when to exit.
  for (;;) {
     NetSock *s = server->Accept();
     if (s == nullptr) {
       continue;
     }

     std::thread worker_handler_thread(WorkerHandler, s);
     worker_handler_thread.detach();

     printf("CH: New connection from %s:%u\n",
         server->GetStrIP(), server->GetPort());
     fflush(stdout);
  }
}

// TODO(gynvael): Add scene support.
size_t GenerateWork(const MythTracer&, const Camera& cam,
                    int width, int height) {
  std::lock_guard<std::mutex> lock(g_work_available_guard);
  // TODO(gynvael): Add an assert that .empty() is true.
  g_work_available.clear();

  size_t total_chunk_count = 0;
  for (int j = 0; j < height; j += CHUNK_H) {
    for (int i = 0; i < width; i += CHUNK_W, total_chunk_count++) {
      int chunk_width = std::min(CHUNK_W, width - i);
      int chunk_height = std::min(CHUNK_H, height - j);      

      auto work = std::make_unique<WorkChunk>();
      work->image_width = width;
      work->image_height = height;
      work->chunk_x = i;
      work->chunk_y = j;
      work->chunk_width = chunk_width;
      work->chunk_height = chunk_height;
      work->camera = cam;

      g_work_available.push_back(std::move(work));
    }
  }

  return total_chunk_count;
}

void BlitWorkChunk(std::vector<uint8_t> *bitmap, WorkChunk *work) {
  auto src_bitmap = work->output_bitmap;

  for (int j = 0; j < work->chunk_height; j++) {
    for (int i = 0; i < work->chunk_width; i++) {
      const size_t dst_idx =
        ((j + work->chunk_y) * work->image_width + (i + work->chunk_x)) * 3;
      const size_t src_idx = (j * work->chunk_width + i) * 3;
      for (int k = 0; k < 3; k++) {
        bitmap->at(dst_idx + k) = src_bitmap.at(src_idx + k);
      }
    }
  }
}


int main(void) {
  puts("Creating /anim directory");
#ifdef __unix__
  mkdir("anim", 0700);
#else
  _mkdir("anim");
#endif

  puts("Loading scene...");
  MythTracer mt;
  if (!mt.LoadObj("../Models/Living Room USSU Design.obj")) {
    return 1;
  }

  // Add lights.
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

  printf("Resolution: %u %u\n", W, H);

  // Really good camera setting.
  Camera cam{
    { 300.0, 57.0, 160.0 },
     0.0, 180.0, 0.0,
     110.0
  };

  std::vector<uint8_t> bitmap(W * H * 3);

  puts("Starting server...");
  NetSock::InitNetworking();
  auto server = std::make_unique<NetSock>();
  // TODO(gynvael): Add option to listen to only specific interfaces.
  const short tcp_port = 12345;
  if (!server->ListenAll(tcp_port)) {
    fprintf(stderr, "error: failed to listen on TCP port %u\n", tcp_port);
    return 1;
  }

  std::thread connection_thread(ConnectionHandler, server.release());

  size_t total_work_chunks = 0;
  size_t completed_work_chunks = 0;
  int frame = 0;
  time_t last_dump = time(nullptr);
  for (;;) {
    // Check if work for the frame needs to be generated.
    if (total_work_chunks == 0) {
      puts("Generating new work..."); fflush(stdout);
      completed_work_chunks = 0;      
      total_work_chunks = GenerateWork(mt, cam, W, H);
    }

    // Check if any new work items finished.
    {
      std::lock_guard<std::mutex> lock(g_work_finished_guard);
      if (!g_work_finished.empty()) {
        // Apply work chunks to the bitmap.
        for (const auto& ready : g_work_finished) {
          BlitWorkChunk(&bitmap, ready->work.get());
        }

        g_work_finished.clear();
      }
    }

    // Perhaps dump the current frame.
    if (time(nullptr) > last_dump + 2) {
      FILE *f = fopen("anim/frame_dump.raw", "wb");
      fwrite(&bitmap[0], bitmap.size(), 1, f);
      fclose(f);
      last_dump = time(nullptr);
      puts("Saved frame to disk.");
    }

    // Check if frame is ready.
    if (total_work_chunks == completed_work_chunks) {
      puts("Writing frame...");
      char fname[256];
      sprintf(fname, "anim/dump_%.5i.raw", frame);

      FILE *f = fopen(fname, "wb");
      fwrite(&bitmap[0], bitmap.size(), 1, f);
      fclose(f);

      // Reset stuff.
      memset(&bitmap[0], 0, bitmap.size());
      total_work_chunks = 0;

      // TODO(gynvael): Iterate frame.
      frame++;

      continue;  // Don't sleep.
    }

    // Sleep.
    std::this_thread::sleep_for(100ms);
  }
  
  puts("Done");

  return 0;
}

