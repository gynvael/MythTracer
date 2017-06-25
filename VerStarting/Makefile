CFLAGS=-Wall -Wextra

%.o: %.cc
	g++ $(CFLAGS) -c -o $@ $<

math3d_test: math3d_test.o test_helper.o
	g++ $(CFLAGS) \
	  math3d_test.o \
	  test_helper.o \
	  -o math3d_test

octtree_test: octtree_test.o octtree.o primitive_triangle.o test_helper.o
	g++ $(CFLAGS) \
	  octtree_test.o \
	  octtree.o \
	  primitive_triangle.o \
	  test_helper.o \
	  -o octtree_test

mythtracer: mythtracer.o octtree.o primitive_triangle.o
	g++ $(CFLAGS) \
	  mythtracer.o \
	  octtree.o \
	  primitive_triangle.o \
	  -o mythtracer	  

test: math3d_test octtree_test
	./math3d_test
	./octtree_test

run: mythtracer
	./mythtracer


