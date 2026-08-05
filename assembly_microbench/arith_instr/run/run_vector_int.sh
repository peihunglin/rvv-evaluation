#!/bin/bash

BUILD_DIRS=${BUILD_DIRS:-"build/x100-clang24 build/x100-gcc15 build/a100-clang24 build/a100-gcc15"}

# make veryclean
#./gen/gen_vector_int.sh
for BUILD_DIR in ${BUILD_DIRS}; do
  echo "== ${BUILD_DIR} =="
  for OPS in vadd vmul vdiv vmacc; do
    for RES in v; do
      for lmul in m1; do
        for databits in 8 16 32 64; do
          for tail in ta; do
            for mask in ma; do
              echo "${OPS}_v${RES}_${databits}_${lmul}_${tail}_${mask}"
              exe="${BUILD_DIR}/${OPS}_v${RES}_${databits}_${lmul}_${tail}_${mask}.x"
	      if [[ "$BUILD_DIR" == *a100* ]]; then
		      "$HOME/bin/ai" "$exe"
	      else
		      "$exe"
	      fi
            done
          done
        done
      done
    done
  done
done
