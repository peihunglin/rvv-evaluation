#!/bin/bash
BUILD_DIRS=${BUILD_DIRS:-"build/x100-clang24 build/x100-gcc15 build/a100-clang24 build/a100-gcc15"}

for BUILD_DIR in ${BUILD_DIRS}; do
  echo "== ${BUILD_DIR} =="
  for OPS in vle vse; do
    for RES in v; do
      for lmul in m1; do
        for databits in 8 16 32 64; do
          for tail in tu; do
            for mask in mu; do
              for maskcode in 1010 1 2 4 8 16 32; do
                exe="${OPS}_v${RES}_${databits}_${lmul}_${tail}_${mask}_${maskcode}"
                echo "${exe}"
			if [[ "$BUILD_DIR" == *a100* ]]; then
		                "$HOME/bin/ai" "${BUILD_DIR}/${exe}.x"
			else
		                "${BUILD_DIR}/${exe}.x"
			fi
              done
            done
          done
        done
      done
    done
  done
done
