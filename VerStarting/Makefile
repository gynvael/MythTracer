CFLAGS=-Wall -Wextra -O3 -ggdb

%.o: %.cc
	g++ $(CFLAGS) -c -o $@ $<

math3d_test: math3d_test.o test_helper.o
	g++ $(CFLAGS) \
	  math3d_test.o \
	  test_helper.o \
	  -o math3d_test

octtree_test: octtree_test.o aabb.o octtree.o primitive_triangle.o test_helper.o
	g++ $(CFLAGS) \
	  octtree_test.o \
	  octtree.o \
	  primitive_triangle.o \
	  test_helper.o \
	  aabb.o \
	  -o octtree_test

mythtracer: mythtracer.o objreader.o octtree.o primitive_triangle.o aabb.o
	g++ $(CFLAGS) \
	  mythtracer.o \
	  objreader.o \
	  octtree.o \
	  primitive_triangle.o \
	  aabb.o \
	  -o mythtracer	  

test: math3d_test octtree_test
	./math3d_test
	./octtree_test

clean:
	del *.o

run: mythtracer
	./mythtracer

all: mythtracer test

