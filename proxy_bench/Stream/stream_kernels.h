#ifndef K3_STREAM_KERNELS_H
#define K3_STREAM_KERNELS_H

#include <stddef.h>
#include <sys/types.h>

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

#ifdef __cplusplus
extern "C" {
#endif

void stream_copy_x100(STREAM_TYPE * restrict c,
                      const STREAM_TYPE * restrict a,
                      ssize_t begin, ssize_t end);
void stream_scale_x100(STREAM_TYPE * restrict b,
                       const STREAM_TYPE * restrict c,
                       STREAM_TYPE scalar,
                       ssize_t begin, ssize_t end);
void stream_add_x100(STREAM_TYPE * restrict c,
                     const STREAM_TYPE * restrict a,
                     const STREAM_TYPE * restrict b,
                     ssize_t begin, ssize_t end);
void stream_triad_x100(STREAM_TYPE * restrict a,
                       const STREAM_TYPE * restrict b,
                       const STREAM_TYPE * restrict c,
                       STREAM_TYPE scalar,
                       ssize_t begin, ssize_t end);

void stream_copy_a100(STREAM_TYPE * restrict c,
                      const STREAM_TYPE * restrict a,
                      ssize_t begin, ssize_t end);
void stream_scale_a100(STREAM_TYPE * restrict b,
                       const STREAM_TYPE * restrict c,
                       STREAM_TYPE scalar,
                       ssize_t begin, ssize_t end);
void stream_add_a100(STREAM_TYPE * restrict c,
                     const STREAM_TYPE * restrict a,
                     const STREAM_TYPE * restrict b,
                     ssize_t begin, ssize_t end);
void stream_triad_a100(STREAM_TYPE * restrict a,
                       const STREAM_TYPE * restrict b,
                       const STREAM_TYPE * restrict c,
                       STREAM_TYPE scalar,
                       ssize_t begin, ssize_t end);

#ifdef __cplusplus
}
#endif
#endif
