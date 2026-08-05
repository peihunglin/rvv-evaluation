CXX = clang++
CXXFLAGS = -static -march=rv64gcv_zfh_zvfh -mcpu=spacemit-a100
build = build/a100-clang24
LD = $(CXX)
LDFLAGS =
