#include "stream_kernels.h"

#if defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

/*
 * Deliberately simple loops: each translation unit is compiled with a
 * different -mcpu/-march, allowing Clang to generate the best RVV code
 * for that subsystem while preserving STREAM semantics.
 *
 * The noinline attribute keeps the worker in its architecture-specific
 * object and prevents LTO/inlining from accidentally retuning it for the
 * driver target.
 */

NOINLINE
void stream_copy_x100(STREAM_TYPE * restrict c,
                       const STREAM_TYPE * restrict a,
                       ssize_t begin, ssize_t end)
{
#pragma clang loop vectorize(enable)
    for (ssize_t j = begin; j < end; ++j)
        c[j] = a[j];
}

NOINLINE
void stream_scale_x100(STREAM_TYPE * restrict b,
                        const STREAM_TYPE * restrict c,
                        STREAM_TYPE scalar,
                        ssize_t begin, ssize_t end)
{
#pragma clang loop vectorize(enable)
    for (ssize_t j = begin; j < end; ++j)
        b[j] = scalar * c[j];
}

NOINLINE
void stream_add_x100(STREAM_TYPE * restrict c,
                      const STREAM_TYPE * restrict a,
                      const STREAM_TYPE * restrict b,
                      ssize_t begin, ssize_t end)
{
#pragma clang loop vectorize(enable)
    for (ssize_t j = begin; j < end; ++j)
        c[j] = a[j] + b[j];
}

NOINLINE
void stream_triad_x100(STREAM_TYPE * restrict a,
                        const STREAM_TYPE * restrict b,
                        const STREAM_TYPE * restrict c,
                        STREAM_TYPE scalar,
                        ssize_t begin, ssize_t end)
{
#pragma clang loop vectorize(enable)
    for (ssize_t j = begin; j < end; ++j)
        a[j] = b[j] + scalar * c[j];
}
