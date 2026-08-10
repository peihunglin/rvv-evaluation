#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "stream_kernels.h"

#ifndef STREAM_ARRAY_SIZE
#define STREAM_ARRAY_SIZE 50000000
#endif

#ifndef NTIMES
#define NTIMES 10
#endif
#if NTIMES <= 1
#undef NTIMES
#define NTIMES 10
#endif

#ifndef OFFSET
#define OFFSET 0
#endif

#define HLINE "-------------------------------------------------------------\n"
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

static STREAM_TYPE a[STREAM_ARRAY_SIZE+OFFSET];
static STREAM_TYPE b[STREAM_ARRAY_SIZE+OFFSET];
static STREAM_TYPE c[STREAM_ARRAY_SIZE+OFFSET];

static double avgtime[4] = {0,0,0,0};
static double maxtime[4] = {0,0,0,0};
static double mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static const char *label[4] = {
    "Copy:      ", "Scale:     ", "Add:       ", "Triad:     "
};

static const double bytes[4] = {
    2.0 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    2.0 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3.0 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3.0 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE
};

static double mysecond(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (double)tp.tv_sec + (double)tp.tv_usec * 1.0e-6;
}

static int env_int(const char *name, int fallback)
{
    const char *v = getenv(name);
    return (!v || !*v) ? fallback : atoi(v);
}

static int parse_cpu_list(const char *spec, int *cpus, int max_cpus)
{
    int count = 0;
    const char *p = spec;

    while (*p && count < max_cpus) {
        char *end = NULL;
        long first = strtol(p, &end, 10);
        long last = first;

        if (end == p)
            break;

        if (*end == '-') {
            p = end + 1;
            last = strtol(p, &end, 10);
        }

        for (long cpu = first; cpu <= last && count < max_cpus; ++cpu)
            cpus[count++] = (int)cpu;

        p = (*end == ',') ? end + 1 : end;
    }

    return count;
}

static void bind_x100_worker(int worker_id)
{
    const char *spec = getenv("X100_CPUS");
    int cpus[CPU_SETSIZE];

    if (!spec || !*spec)
        spec = "0-7";

    int count = parse_cpu_list(spec, cpus, CPU_SETSIZE);
    if (count <= 0) {
        fprintf(stderr, "ERROR: invalid X100_CPUS=%s\n", spec);
        exit(EXIT_FAILURE);
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpus[worker_id % count], &set);

    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        fprintf(stderr, "ERROR: X100 affinity worker %d: %s\n",
                worker_id, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void write_ai_binding_id(const char *kind, long id)
{
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%ld", id);

    int fd = open("/proc/set_ai_thread", O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: open /proc/set_ai_thread (%s): %s\n",
                kind, strerror(errno));
        exit(EXIT_FAILURE);
    }

    ssize_t nw = write(fd, buf, (size_t)len);
    if (nw != len) {
        fprintf(stderr, "ERROR: write /proc/set_ai_thread (%s): %s\n",
                kind, strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
}

static void bind_a100_thread(void)
{
    write_ai_binding_id("thread", (long)syscall(SYS_gettid));
}

static void bind_a100_process(void)
{
    write_ai_binding_id("process", (long)getpid());
}

static void split_range(ssize_t n,
                        int worker_id,
                        int workers,
                        ssize_t *begin,
                        ssize_t *end)
{
    *begin = (n * (ssize_t)worker_id) / workers;
    *end   = (n * (ssize_t)(worker_id + 1)) / workers;
}

static ssize_t heterogeneous_split(int x100_threads, int a100_threads)
{
    const int total = x100_threads + a100_threads;
    int default_share = total ? (100 * a100_threads / total) : 0;
    int a100_share = env_int("STREAM_A100_SHARE", default_share);

    if (x100_threads == 0)
        a100_share = 100;
    else if (a100_threads == 0)
        a100_share = 0;

    if (a100_share < 0) a100_share = 0;
    if (a100_share > 100) a100_share = 100;

    ssize_t split =
        (STREAM_ARRAY_SIZE * (ssize_t)(100 - a100_share)) / 100;

    ssize_t elems_per_line = (ssize_t)(64 / sizeof(STREAM_TYPE));
    if (elems_per_line < 1)
        elems_per_line = 1;

    split = (split / elems_per_line) * elems_per_line;
    return split;
}

static int checktick(void)
{
#define M 20
    int minDelta = 1000000;
    double timesfound[M];

    for (int i = 0; i < M; ++i) {
        double t1 = mysecond(), t2;
        while (((t2 = mysecond()) - t1) < 1.0e-6)
            ;
        timesfound[i] = t2;
    }

    for (int i = 1; i < M; ++i) {
        int delta = (int)(1.0e6 * (timesfound[i] - timesfound[i-1]));
        minDelta = MIN(minDelta, MAX(delta, 0));
    }
    return minDelta;
#undef M
}

static void checkSTREAMresults(void)
{
    STREAM_TYPE aj = 1.0, bj = 2.0, cj = 0.0;
    STREAM_TYPE scalar = 3.0;

    aj = 2.0E0 * aj;

    for (int k = 0; k < NTIMES; ++k) {
        cj = aj;
        bj = scalar * cj;
        cj = aj + bj;
        aj = bj + scalar * cj;
    }

    long double aSumErr = 0.0L;
    long double bSumErr = 0.0L;
    long double cSumErr = 0.0L;

    for (ssize_t j = 0; j < STREAM_ARRAY_SIZE; ++j) {
        aSumErr += fabsl((long double)a[j] - (long double)aj);
        bSumErr += fabsl((long double)b[j] - (long double)bj);
        cSumErr += fabsl((long double)c[j] - (long double)cj);
    }

    long double aAvgErr = aSumErr / (long double)STREAM_ARRAY_SIZE;
    long double bAvgErr = bSumErr / (long double)STREAM_ARRAY_SIZE;
    long double cAvgErr = cSumErr / (long double)STREAM_ARRAY_SIZE;

    double epsilon = (sizeof(STREAM_TYPE) == 4) ? 1.0e-6 : 1.0e-13;
    int err = 0;

    if (fabsl(aAvgErr / (long double)aj) > epsilon) ++err;
    if (fabsl(bAvgErr / (long double)bj) > epsilon) ++err;
    if (fabsl(cAvgErr / (long double)cj) > epsilon) ++err;

    if (!err) {
        printf("Solution Validates: avg error less than %e on all three arrays\n",
               epsilon);
    } else {
        printf("ERROR: validation failed\n");
        printf("a avg abs err = %.6Le\n", aAvgErr);
        printf("b avg abs err = %.6Le\n", bAvgErr);
        printf("c avg abs err = %.6Le\n", cAvgErr);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
#ifndef _OPENMP
    fprintf(stderr, "ERROR: this K3 heterogeneous version requires OpenMP\n");
    return EXIT_FAILURE;
#else
    const int x100_threads = env_int("X100_THREADS", 8);
    const int a100_threads = env_int("A100_THREADS", 8);
    const int total_threads = x100_threads + a100_threads;

    if (x100_threads < 0 || a100_threads < 0 || total_threads <= 0) {
        fprintf(stderr, "ERROR: invalid thread configuration\n");
        return EXIT_FAILURE;
    }

    const char *bind = getenv("A100_BIND");
    if (!bind || !*bind)
        bind = (x100_threads == 0) ? "process" : "thread";

    const int a100_bind_process = (strcmp(bind, "process") == 0);

    if (strcmp(bind, "process") && strcmp(bind, "thread")) {
        fprintf(stderr, "ERROR: A100_BIND must be process or thread\n");
        return EXIT_FAILURE;
    }

    if (a100_bind_process && x100_threads > 0 && a100_threads > 0) {
        fprintf(stderr,
                "ERROR: mixed X100+A100 run requires A100_BIND=thread\n");
        return EXIT_FAILURE;
    }

    if (a100_threads > 0 && a100_bind_process)
        bind_a100_process();

    omp_set_dynamic(0);

    int a100_share = env_int(
        "STREAM_A100_SHARE",
        total_threads ? 100 * a100_threads / total_threads : 0);

    if (x100_threads == 0) a100_share = 100;
    if (a100_threads == 0) a100_share = 0;

    if (a100_share < 0) a100_share = 0;
    if (a100_share > 100) a100_share = 100;

    const ssize_t split =
        heterogeneous_split(x100_threads, a100_threads);

    printf(HLINE);
    printf("STREAM version $Revision: 5.10 $\n");
    printf("K3 heterogeneous variant (persistent team, separate X100/A100 objects)\n");
    printf(HLINE);
    printf("Array size            = %llu elements\n",
           (unsigned long long)STREAM_ARRAY_SIZE);
    printf("Element size          = %zu bytes\n", sizeof(STREAM_TYPE));
    printf("Memory per array      = %.1f MiB\n",
           sizeof(STREAM_TYPE) *
           ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024.0));
    printf("Total array memory    = %.1f MiB\n",
           3.0 * sizeof(STREAM_TYPE) *
           ((double)STREAM_ARRAY_SIZE / 1024.0 / 1024.0));
    printf("NTIMES                = %d\n", NTIMES);
    printf("X100 threads          = %d\n", x100_threads);
    printf("A100 threads          = %d\n", a100_threads);
    printf("A100 array share      = %d%%\n", a100_share);
    printf("Array split           = %lld / %llu elements\n",
           (long long)split,
           (unsigned long long)STREAM_ARRAY_SIZE);
    printf("A100 bind             = %s\n", bind);
    printf("X100 CPU list         = %s\n",
           getenv("X100_CPUS") ? getenv("X100_CPUS") : "0-7");
    printf("Persistent OMP team   = yes\n");

    STREAM_TYPE scalar = 3.0;
    double times[4][NTIMES];
    double start[4] = {0,0,0,0};

    /*
     * One OpenMP team for initialization, calibration, all timed kernels,
     * and all NTIMES iterations. Affinity/A100 binding is performed once.
     */
#pragma omp parallel num_threads(total_threads) shared(times,start)
    {
        const int tid = omp_get_thread_num();
        const int is_x100 = (tid < x100_threads);

        int worker_id, workers;
        ssize_t begin, end;

        if (is_x100) {
            worker_id = tid;
            workers = x100_threads;
            bind_x100_worker(worker_id);
            split_range(split, worker_id, workers, &begin, &end);
        } else {
            worker_id = tid - x100_threads;
            workers = a100_threads;

            if (!a100_bind_process)
                bind_a100_thread();

            ssize_t lb, le;
            const ssize_t nlocal = STREAM_ARRAY_SIZE - split;
            split_range(nlocal, worker_id, workers, &lb, &le);
            begin = split + lb;
            end = split + le;
        }

        /* Initialize arrays inside the already-bound persistent team. */
        for (ssize_t j = begin; j < end; ++j) {
            a[j] = 1.0;
            b[j] = 2.0;
            c[j] = 0.0;
        }

#pragma omp barrier

        /* Calibration operation, excluded from benchmark timings. */
#pragma omp master
        start[0] = mysecond();

#pragma omp barrier

        for (ssize_t j = begin; j < end; ++j)
            a[j] = 2.0E0 * a[j];

#pragma omp barrier

#pragma omp master
        {
            double cal_us = 1.0e6 * (mysecond() - start[0]);
            int quantum = checktick();
            if (quantum < 1) quantum = 1;
            printf("Calibration           = %.0f us (~%.0f timer ticks)\n",
                   cal_us, cal_us / quantum);
            printf(HLINE);
        }

#pragma omp barrier

        for (int k = 0; k < NTIMES; ++k) {
            /*
             * COPY
             * Barrier before the master timestamp ensures every worker
             * completed the previous kernel. Barrier after timestamp makes
             * all workers start from the same phase.
             */
#pragma omp barrier
#pragma omp master
            start[0] = mysecond();
#pragma omp barrier

            if (is_x100)
                stream_copy_x100(c, a, begin, end);
            else
                stream_copy_a100(c, a, begin, end);

#pragma omp barrier
#pragma omp master
            times[0][k] = mysecond() - start[0];

            /* SCALE */
#pragma omp barrier
#pragma omp master
            start[1] = mysecond();
#pragma omp barrier

            if (is_x100)
                stream_scale_x100(b, c, scalar, begin, end);
            else
                stream_scale_a100(b, c, scalar, begin, end);

#pragma omp barrier
#pragma omp master
            times[1][k] = mysecond() - start[1];

            /* ADD */
#pragma omp barrier
#pragma omp master
            start[2] = mysecond();
#pragma omp barrier

            if (is_x100)
                stream_add_x100(c, a, b, begin, end);
            else
                stream_add_a100(c, a, b, begin, end);

#pragma omp barrier
#pragma omp master
            times[2][k] = mysecond() - start[2];

            /* TRIAD */
#pragma omp barrier
#pragma omp master
            start[3] = mysecond();
#pragma omp barrier

            if (is_x100)
                stream_triad_x100(a, b, c, scalar, begin, end);
            else
                stream_triad_a100(a, b, c, scalar, begin, end);

#pragma omp barrier
#pragma omp master
            times[3][k] = mysecond() - start[3];
        }
    }

    for (int k = 1; k < NTIMES; ++k) {
        for (int j = 0; j < 4; ++j) {
            avgtime[j] += times[j][k];
            mintime[j] = MIN(mintime[j], times[j][k]);
            maxtime[j] = MAX(maxtime[j], times[j][k]);
        }
    }

    printf("Function    Best Rate MB/s  Avg time     Min time     Max time\n");
    for (int j = 0; j < 4; ++j) {
        avgtime[j] /= (double)(NTIMES - 1);
        printf("%s%12.1f  %11.6f  %11.6f  %11.6f\n",
               label[j],
               1.0e-6 * bytes[j] / mintime[j],
               avgtime[j],
               mintime[j],
               maxtime[j]);
    }
    printf(HLINE);

    checkSTREAMresults();
    printf(HLINE);

    return 0;
#endif
}
