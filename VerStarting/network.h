#pragma once
#include <stdint.h>
#include <memory>
#include <vector>
#include "NetSock/NetSock.h"
#include "math3d.h"
#include "scene.h"
#include "camera.h"
#include "mythtracer.h"

namespace raytracer {

using math3d::V3D;

namespace netproto {
// Wire format is the following:
// 4 bytes Tag
// 8 bytes Sender/Destination ID (8-byte string)
// 4 bytes Length (excluding Tag, ID and Length)
// N bytes Data
// The wire format of each class is known only to the class itself.
// The Tag, ID and Length are automatically sent; there is no need for the class
// to add them manually to the bytes vector, however the Sender/Destination ID
// field must be filled. Worker puts its own ID in the Sender ID fields. Master
// puts the Destination ID there.
// All values are LE.

enum class kCommunicationSide {
  kMaster,
  kWorker
};

class NetworkProto {
 public:
  virtual ~NetworkProto() { };
  virtual std::string GetTag() const = 0;
  std::vector<uint8_t> bytes;
  std::string id;  // Sender/Destination ID.
};

// Worker->Master: Worker ready to receive the scene.
class WorkerReady : public NetworkProto {
 public:
  static WorkerReady* Make(const std::string &sender_id);
  std::string GetTag() const override {
    return "RDY!";
  };
};

// Master->Worker: Serialized scene (primitives, textures, lights).
class MasterScene : public NetworkProto {
 public:
  static MasterScene* Make(const std::string &destination_id, Scene *scene);
  std::string GetTag() const override {
    return "SCNE";
  };
};

// TODO(gynvael): Perhaps merge this with WORK?
// Master->Worker: Serialized Camera.
class MasterSetCamera : public NetworkProto {
 public:
  static MasterSetCamera* Make(
      const std::string &destination_id, Camera *camera);
  std::string GetTag() const override {
    return "CAMR";
  };
};

// Master->Worker: Serialized WorkChunk.
class MasterRenderOrder : public NetworkProto {
 public:
  static MasterRenderOrder* Make(
      const std::string &destination_id, WorkChunk *chunk);
  std::string GetTag() const override {
    return "WORK";
  };
};

// Worker->Master: Bitmap and logs/stats.
class WorkerRenderResult : public NetworkProto {
 public:
  // Uses only the output_* part of WorkChunk.
  static WorkerRenderResult* Make(
      const std::string &sender_id, WorkChunk *chunk);

  std::string GetTag() const override {
    return "PXLS";
  };
};

bool SendPacket(NetSock *s, NetworkProto *packet);
NetworkProto* ReceivePacket(NetSock *s, kCommunicationSide side);

}  // namespace netproto

}  // namespace raytracer

