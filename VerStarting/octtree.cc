#include <algorithm>
#include "octtree.h"

namespace raytracer {

OctTree::OctTree() { }

void OctTree::AddPrimitive(Primitive *p) {
  primitives.push_back(std::unique_ptr<Primitive>(p));

  // Update axis-aligned bounding box.
  AABB aabb = p->GetAABB();
  root.aabb.Extend(aabb);
}

void OctTree::Finalize() {
  root.primitives.reserve(primitives.size());
  for (auto& p : primitives) {
    root.primitives.push_back(p.get());
  }

  root.AttemptSplit();
}

const Primitive* OctTree::IntersectRay(
    const Ray& ray, V3D *point, V3D::basetype *distance) const {
  V3D::basetype dist;
  if (!root.NodeIntersectRay(ray, &dist)) {
    return nullptr;
  }

  return root.PrimitiveIntersectRay(ray, point, distance);
}

AABB OctTree::GetAABB() const {
  return root.aabb;
}

void OctTree::Node::CalcCenter() {
  center.v[0] = aabb.min.v[0] + (aabb.max.v[0] - aabb.min.v[0]) / 2.0;
  center.v[1] = aabb.min.v[1] + (aabb.max.v[1] - aabb.min.v[1]) / 2.0;
  center.v[2] = aabb.min.v[2] + (aabb.max.v[2] - aabb.min.v[2]) / 2.0;  
}

void OctTree::Node::AttemptSplit() {
  if (primitives.size() < SPLIT_BOUNDARY) {
    return;
  }

  CalcCenter();
  nodes.resize(8);

  // Bottom nodes.
  nodes[0].aabb = { 
    aabb.min,
    center
  };

  nodes[1].aabb = { 
    { center.v[0], aabb.min.v[1], aabb.min.v[2] },
    { aabb.max.v[0], center.v[1], center.v[2] }
  };

  nodes[2].aabb = { 
    { aabb.min.v[0], aabb.min.v[1], center.v[2] },
    { center.v[0], center.v[1], aabb.max.v[2] }
  };

  nodes[3].aabb = { 
    { center.v[0], aabb.min.v[1], center.v[2] },
    { aabb.max.v[0], center.v[1], aabb.max.v[2] }
  };  

  // Top nodes.
  nodes[4].aabb = { 
    { aabb.min.v[0], center.v[1], aabb.min.v[2] },
    { center.v[0], aabb.max.v[1], center.v[2] }
  };

  nodes[5].aabb = { 
    { center.v[0], center.v[1], aabb.min.v[2] },
    { aabb.max.v[0], aabb.max.v[1], center.v[2] }
  };

  nodes[6].aabb = { 
    { aabb.min.v[0], center.v[1], center.v[2] },
    { center.v[0], aabb.max.v[1], aabb.max.v[2] }
  };  

  nodes[7].aabb = { 
    center,
    aabb.max
  };

  // Check whether the primitive is fully contained in one of the nodes.
  // If not, it should remain within this node.
  std::vector<Primitive*> remaining;

  for (Primitive *p : primitives) {
    AABB p_aabb = p->GetAABB();
    bool subnode_found = false;

    for (Node& n : nodes) {
      // TODO(gynvael): Use primitive x AABB intersection.
      if (n.aabb.FullyContains(p_aabb)) {
        n.primitives.push_back(p);
        subnode_found = true;
        break;
      }
    }

    if (!subnode_found) {
      remaining.push_back(p);
    }
  }

  //printf("%s %s -- %i / %i\n", V3DStr(aabb.min), V3DStr(aabb.max),
  //    (int)remaining.size(), (int)primitives.size());  

  // Clear and free capacity.
  primitives.clear();
  remaining.swap(primitives);

  // Split the nodes.
  for (Node& n : nodes) {
    n.AttemptSplit();
  }
}

// https://gamedev.stackexchange.com/questions/18436
bool OctTree::Node::NodeIntersectRay(
    const Ray& ray, V3D::basetype *dist) const {
  V3D dirfrac(
      1.0 / ray.direction.x(),
      1.0 / ray.direction.y(),
      1.0 / ray.direction.z());

  V3D::basetype t1 = (aabb.min.x() - ray.origin.x()) * dirfrac.x();
  V3D::basetype t2 = (aabb.max.x() - ray.origin.x()) * dirfrac.x();
  V3D::basetype t3 = (aabb.min.y() - ray.origin.y()) * dirfrac.y();
  V3D::basetype t4 = (aabb.max.y() - ray.origin.y()) * dirfrac.y();
  V3D::basetype t5 = (aabb.min.z() - ray.origin.z()) * dirfrac.z();
  V3D::basetype t6 = (aabb.max.z() - ray.origin.z()) * dirfrac.z();

  V3D::basetype tmin = std::max(
      std::max(std::min(t1, t2), std::min(t3, t4)),
      std::min(t5, t6));
  V3D::basetype tmax = std::min(
      std::min(std::max(t1, t2), std::max(t3, t4)),
      std::max(t5, t6));

  // If tmax is less than zero, ray (line) is intersecting AABB, but the whole
  // AABB is behind the ray.
  if (tmax < 0) {
    return false;
  }

  // If tmin is greater than tmax, ray doesn't intersect AABB.
  if (tmin > tmax) {
    return false;
  }

  *dist = tmin;
  return true;
}

const Primitive* OctTree::Node::PrimitiveIntersectRay(
    const Ray& ray, V3D *point, V3D::basetype *distance) const {

  const Primitive *closest_primitive = nullptr;
  V3D::basetype closest_distance{};
  V3D closest_point;

  // Start by looking through the list of primitives contained in this node.
  for (const Primitive *p : primitives) {
    V3D intersection_point;
    V3D::basetype intersection_distance;
    if (!p->IntersectRay(ray, &intersection_point, &intersection_distance)) {
      continue;
    }

    // Calculate the distance and check if it's closer than the previously
    // discovered primitive (if any).
    if (closest_primitive != nullptr && 
        intersection_distance > closest_distance) {
      // Previously discovered was closer. Continue.
      continue;
    }

    // The newly discovered primitive is the closest.
    closest_primitive = p;
    closest_distance = intersection_distance;
    closest_point = intersection_point;
  }

  // Check if there is a closer primitive in children nodes.
  for (const Node& n : nodes) {
    V3D::basetype dist;
    if (!n.NodeIntersectRay(ray, &dist)) {
      continue;
    }

    // TODO(gynvael): Sort AABB by distance before doing the intersection tests.
    V3D intersection_point;
    V3D::basetype intersection_distance;
    const Primitive *intersection_primitive = n.PrimitiveIntersectRay(
        ray, &intersection_point, &intersection_distance);

    if (intersection_primitive == nullptr) {
      continue;
    }

    // Calculate the distance and check if it's closer than the previously
    // discovered primitive (if any).
    if (closest_primitive != nullptr && 
        intersection_distance > closest_distance) {
      // Previously discovered was closer. Continue.
      continue;
    }

    // The newly discovered primitive is the closest.
    closest_primitive = intersection_primitive;
    closest_distance = intersection_distance;
    closest_point = intersection_point;
  }

  // Return the found coliding point, if any.
  if (closest_primitive == nullptr) {
    return nullptr;
  }

  *point = closest_point;
  *distance = closest_distance;
  return closest_primitive;
}

}  // namespace raytracer

