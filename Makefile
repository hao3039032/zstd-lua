CC ?= gcc
CXX ?= g++
LUA_INCDIR ?= /opt/homebrew/include/lua5.4
ZSTD_INCDIR ?= /opt/homebrew/include
ZSTD_LIBDIR ?= /usr/local/lib
LIBFLAG ?= -shared
CFLAGS ?= -O2 -fPIC

all:
	$(CXX) $(CFLAGS) -std=c++17 $(CFLAGS) -c lzstd.cpp -o lzstd.o -I ${LUA_INCDIR} -I ${ZSTD_INCDIR}
	$(CXX) $(LIBFLAG) -o zstd.so lzstd.o -l zstd -L ${ZSTD_LIBDIR}

install: all
	cp zstd.so $(INST_LIBDIR)