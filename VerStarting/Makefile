CXX=g++
CFLAGS=-Wall -Wextra -O3 -ggdb -fno-omit-frame-pointer \
       -std=c++1z \
       -D_USE_MATH_DEFINES \
       #-march=broadwell -mtune=intel -mavx2

ifeq ($(OS),Windows_NT)
	WINSOCK=-lws2_32
else
	WINSOCK=
endif


%.o: %.cc
	$(CXX) $(CFLAGS) -c -o $@ $< -fopenmp

math3d_test: math3d_test.o test_helper.o
	$(CXX) $(CFLAGS) \
	  math3d_test.o \
	  test_helper.o \
	  -o math3d_test

octtree_test: octtree_test.o aabb.o octtree.o primitive_triangle.o test_helper.o
	$(CXX) $(CFLAGS) \
	  octtree_test.o \
	  octtree.o \
	  primitive_triangle.o \
	  test_helper.o \
	  aabb.o \
	  -o octtree_test

mythtracer: mythtracer.o objreader.o octtree.o primitive_triangle.o aabb.o camera.o texture.o main_local.o
	$(CXX) $(CFLAGS) \
	  mythtracer.o \
	  objreader.o \
	  octtree.o \
	  primitive_triangle.o \
	  aabb.o \
	  camera.o \
	  texture.o \
	  main_local.o \
	  -o mythtracer	\
	  -lgomp -lSDL2 -lSDL2_image

mythtracer_worker: mythtracer.o objreader.o octtree.o primitive_triangle.o aabb.o camera.o texture.o main_net_worker.o network.o
	g++ $(CFLAGS) \
	  mythtracer.o \
	  objreader.o \
	  octtree.o \
	  primitive_triangle.o \
	  aabb.o \
	  camera.o \
	  texture.o \
	  main_net_worker.o \
	  network.o \
	  -o mythtracer_worker \
	  NetSock/NetSock.cpp \
	  -lgomp -lSDL2 -lSDL2_image $(WINSOCK) -static-libgcc -static-libstdc++

mythtracer_master: mythtracer.o objreader.o octtree.o primitive_triangle.o aabb.o camera.o texture.o main_net_master.o network.o
	g++ $(CFLAGS) \
	  mythtracer.o \
	  objreader.o \
	  octtree.o \
	  primitive_triangle.o \
	  aabb.o \
	  camera.o \
	  texture.o \
	  main_net_master.o \
	  network.o \
	  -o mythtracer_master \
	  NetSock/NetSock.cpp \
	  -lpthread -fopenmp -lSDL2 -lSDL2_image -lSDL2main \
	  -lgomp -lSDL2 -lSDL2_image $(WINSOCK)

test: math3d_test octtree_test
	./math3d_test
	./octtree_test

clean:
ifeq ($(OS),Windows_NT)
	del *.o
else
	rm *.o
endif
	

run: mythtracer_worker
	./mythtracer_worker

all: mythtracer test

