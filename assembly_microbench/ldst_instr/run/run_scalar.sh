#!/bin/bash
BUILD_DIRS=${BUILD_DIRS:-"build/x100-clang24 build/x100-gcc15 build/a100-clang24 build/a100-gcc15"}

for BUILD_DIR in ${BUILD_DIRS}; do
  echo "== ${BUILD_DIR} =="
  for exe in \
    lb_8 lh_16 lw_32 ld_64 \
    sb_8 sh_16 sw_32 sd_64 

  do
    echo "${exe}"
	if [[ "$BUILD_DIR" == *a100* ]]; then
	    "$HOME/bin/ai" "${BUILD_DIR}/${exe}.x"
	else
	    "${BUILD_DIR}/${exe}.x"
	fi
  done
done
