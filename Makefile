all: test
clean: testclean libraryclean toolclean

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
	$(CC) -I include -lm -lpng -lz $^ -o $@

tools/gpuviewshed/viewshed: tools/gpuviewshed/main.c libgpu.a
	$(CC) -I include -lm -lpng -lz -lOpenCL $^ -o $@

toolclean:
	rm -f tools/cpuviewshed/viewshed tools/gpuviewshed/viewshed

# Tests

TEST_CXXFLAGS = -I $(GTEST_DIR)/include -I include -lOpenCL
TEST_OBJECTS = tests/commontest.o tests/cputest.o tests/gputest.o
TEST_LIBRARIES = $(LIBRARIES) tests/gtest_main.a

test: tests/test.out
	# Exclude the slow perf tests
	tests/test.out --gtest_filter="-Performance*"

tests/commontest.o: tests/testsrc/commontest.cpp src/common.o include/common.h
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/cputest.o: tests/testsrc/cputest.cpp include/cpu.h
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/gputest.o: tests/testsrc/gputest.cpp include/gpu.h
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/test.out: $(TEST_OBJECTS) $(TEST_LIBRARIES)
	$(CXX) $(TEST_CXXFLAGS) -lpthread $^ -o $@

testclean:
	rm -f tests/*.a tests/*.o tests/*.out

# Google Test Lib

GTEST_DIR = tests/googletest/googletest
GTEST_CPPFLAGS += -isystem $(GTEST_DIR)/include
GTEST_CXXFLAGS += -g -Wall -Wextra -pthread
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

tests/gtest_main.a: tests/gtest-all.o tests/gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

tests/gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(GTEST_CPPFLAGS) -I$(GTEST_DIR) $(GTEST_CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc -o $@

tests/gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(GTEST_CPPFLAGS) -I$(GTEST_DIR) $(GTEST_CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc -o $@

.PHONY: all clean test testclean libraryclean toolclean
