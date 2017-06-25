#include <memory>
#include "octtree.h"
#include "primitive_triangle.h"
#include "test_helper.h"


using namespace test;
using raytracer::OctTree;
using raytracer::Primitive;
using raytracer::Triangle;
using raytracer::Ray;
using math3d::V3D;

int main(void) {
  OctTree tree;

  //         +  1,1
  //        /|
  //       / |
  // 0,0  +--+  1,0
  Triangle *tr0 = new Triangle();
  tr0->vertex[0] = { 1, 1, 0 };
  tr0->vertex[1] = { 1, 0, 0 };
  tr0->vertex[2] = { 0, 0, 0 };
  tree.AddPrimitive(tr0);

  Triangle *tr1 = new Triangle();
  tr1->vertex[0] = { 1, 1, 1 };
  tr1->vertex[1] = { 1, 0, 1 };
  tr1->vertex[2] = { 0, 0, 1 };
  tree.AddPrimitive(tr1);

  tree.Finalize();  

  {
    Ray front{
      { 0.9, 0.9, -10.0 },
      { 0.0, 0.0,   1.0 }
    };

    V3D point;
    V3D::basetype distance;
    auto p = tree.IntersectRay(front, &point, &distance);
    TESTEQ(p, (const Primitive*)tr0);
  }

  {
    Ray back{
      { 0.9, 0.9,  10.0 },
      { 0.0, 0.0,  -1.0 }
    };

    V3D point;
    V3D::basetype distance;
    auto p = tree.IntersectRay(back, &point, &distance);
    TESTEQ(p, (const Primitive*)tr1);
  } 

  {
    Ray miss{
      { 5.0, 5.0, 5.0 },
      { 0.0, 0.0,   1.0 }
    };

    V3D point;
    V3D::basetype distance;
    auto p = tree.IntersectRay(miss, &point, &distance);
    TESTEQ(p, (const Primitive*)nullptr);
  }   


  return 0;
}

