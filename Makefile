all: tools/cpuviewshed/viewshed tools/gpuviewshed/viewshed
clean: libraryclean toolclean

CXX = clang++
CC = clang

# Libraries

LIB_CFLAGS = -I include
LIBRARIES = libcpu.a libgpu.a

src/common.o: include/common.h src/common.c
	$(CC) $(LIB_CFLAGS) -c src/common.c -o src/common.o

src/cpu.o: include/cpu.h src/cpu.c
	$(CC) $(LIB_CFLAGS) -c src/cpu.c -o src/cpu.o
src/gpu.o: include/gpu.h src/gpu.c
	$(CC) $(LIB_CFLAGS) -l OpenCL -c src/gpu.c -o src/gpu.o

libcpu.a: src/cpu.o src/common.o
	$(AR) rcs $@ $^
libgpu.a: src/gpu.o src/common.o
	$(AR) rcs $@ $^

libraryclean:
	rm -f *.a src/*.o

# Tools
tools/cpuviewshed/viewshed: tools/cpuviewshed/main.c libcpu.a
	$(CC) -I include -lm -lpng -lz -ldl $^ -o $@

tools/gpuviewshed/viewshed: tools/gpuviewshed/main.c libgpu.a
	$(CC) -I include -lm -lpng -lz -ldl -lOpenCL $^ -o $@

toolclean:
	rm -f tools/cpuviewshed/viewshed tools/gpuviewshed/viewshed

	$(CXX) $(GTEST_CPPFLAGS) -I$(GTEST_DIR) $(GTEST_CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc -o $@


