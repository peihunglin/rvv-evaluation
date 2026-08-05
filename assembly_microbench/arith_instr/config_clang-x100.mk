CXX = clang++
CXXFLAGS = -static -march=rv64gcv_zfh_zvfh -mcpu=spacemit-x100
build = build/x100-clang24
LD = $(CXX)
LDFLAGS =
