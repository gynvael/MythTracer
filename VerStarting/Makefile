CXX=g++
CFLAGS=-Wall -Wextra -O3 -ggdb -fno-omit-frame-pointer \
       -march=broadwell -mtune=intel -mavx2 -std=c++1z \
       -D_USE_MATH_DEFINES

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

mythtracer: mythtracer.o objreader.o octtree.o primitive_triangle.o aabb.o camera.o texture.o
	$(CXX) $(CFLAGS) \
	  mythtracer.o \
	  objreader.o \
	  octtree.o \
	  primitive_triangle.o \
	  aabb.o \
	  camera.o \
	  texture.o \
	  -o mythtracer	\
	  -lgomp -lSDL2 -lSDL2_image

test: math3d_test octtree_test
	./math3d_test
	./octtree_test

clean:
ifeq ($(OS),Windows_NT)
	del *.o
else
	rm *.o
endif
	

run: mythtracer
	./mythtracer

all: mythtracer test

