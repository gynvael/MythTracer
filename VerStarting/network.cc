#include <limits>
#include <memory>
#include "network.h"

namespace raytracer {
namespace netproto {

WorkerReady* WorkerReady::Make(const std::string &sender_id) {
  auto packet = std::make_unique<WorkerReady>();
  packet->id = sender_id;
  return packet.release();
}

MasterScene* MasterScene::Make(
    const std::string &destination_id, Scene *) {
  // TODO(gynvael): Add proper scene support.
  auto packet = std::make_unique<MasterScene>();
  packet->id = destination_id;
  return packet.release();
}

MasterSetCamera* MasterSetCamera::Make(
    const std::string &destination_id, Camera *camera) {
  auto packet = std::make_unique<MasterSetCamera>();
  packet->id = destination_id;
  camera->Serialize(&packet->bytes);
  return packet.release();
}

MasterRenderOrder* MasterRenderOrder::Make(
    const std::string &destination_id, WorkChunk *chunk) {
  auto packet = std::make_unique<MasterRenderOrder>();
  packet->id = destination_id;
  chunk->SerializeInput(&packet->bytes);
  return packet.release();
}

WorkerRenderResult* WorkerRenderResult::Make(
    const std::string &sender_id, WorkChunk *chunk) {
  auto packet = std::make_unique<WorkerRenderResult>();
  packet->id = sender_id;
  if (!chunk->SerializeOutput(&packet->bytes)) {
    return nullptr;
  }
  return packet.release();
}

// Helper functions.

// TODO(gynvael): Neither serialization nor network helper functions respect the
// LE setting. Add this everywhere.

bool SendPacket(NetSock *s, NetworkProto *packet) {
  // Prepare packet.
  packet->id.resize(8);

  // Send the wire format details.
  if (s->WriteAll(packet->GetTag().c_str(), 4) != 4) {
    return false;
  }

  if (s->WriteAll(packet->id.data(), 8) != 8) {
    return false;
  }

  if (packet->bytes.size() > std::numeric_limits<uint32_t>::max()) {
    // TODO(gynvael): Error message.
    return false;
  }

  uint32_t sz = packet->bytes.size();

  if (s->WriteAll(&sz, 4) != 4) {
    return false;
  }

  // TODO(gynvael): Change netsock to use sane types plz.
  if ((uint32_t)s->WriteAll(&packet->bytes[0], sz) != sz) {
    return false;
  }

  return true;
}

NetworkProto* ReceivePacket(NetSock *s, kCommunicationSide side) {
  uint8_t bytes[4 + 8 + 4]{};
  int ret = s->ReadAll(bytes, sizeof(bytes));
  if (ret != sizeof(bytes)) {
    return nullptr;
  }

  std::string tag((char*)bytes, 4);
  std::string id((char*)bytes + 4, 8);
  uint32_t length;
  memcpy(&length, bytes + 4 + 8, sizeof(uint32_t));

  // TODO(gynvael): Some early per-packed max size checks would be useful.

  // Sanity check.
  // TODO(gynvael): Actually make a better check for SCNE and PXLS later on.
  if (length > 1024 * 1024) {
    return nullptr;
  }

  std::vector<uint8_t> payload;
  payload.resize(length);
  ret = s->ReadAll(&payload[0], length);
  if (ret != (int)length) {
    return nullptr;
  }

  NetworkProto *packet = nullptr;

  if (side == kCommunicationSide::kMaster && tag == "RDY!") {
    packet = new WorkerReady;
  } else if (side == kCommunicationSide::kWorker && tag == "SCNE") {
    packet = new MasterScene;
  } else if (side == kCommunicationSide::kWorker && tag == "CAMR") {
    packet = new MasterSetCamera;
  } else if (side == kCommunicationSide::kWorker && tag == "WORK") {
    packet = new MasterRenderOrder;
  } else if (side == kCommunicationSide::kMaster && tag == "PXLS") {
    packet = new WorkerRenderResult;
  } else {
    return nullptr;
  }

  packet->id = std::move(id);
  packet->bytes = std::move(payload);

  // TODO(gynvael): Think about deserializing here.
  return packet;
}

}  // namespace netproto
}  // namespace raytracer

