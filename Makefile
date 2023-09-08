include platform.mk

LUA_INCLUDE ?= /opt/homebrew/include/lua5.4
ZSTD_INCLUDE ?= /opt/homebrew/include

all:
	$(CXX) $(CFLAGS) -std=c++17 -O2 -fPIC -c lzstd.cpp -o lzstd.o -I ${LUA_INCLUDE} -I ${ZSTD_INCLUDE}
	$(CXX) $(SHARED) -o zstd.so lzstd.o -l zstd