#define _GNU_SOURCE

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef RVV_LMUL_VALUE
#define RVV_LMUL_VALUE 1
#endif

#ifndef HETERO_MODE_NAME
#define HETERO_MODE_NAME "rvv_rvv"
#endif

#define A_arr(i, j) A[(i) + order * (j)]
#define B_arr(i, j) B[(i) + order * (j)]
#define C_arr(i, j) C[(i) + order * (j)]
#define A16_arr(i, j) A16[(i) * order + (j)]
#define B16_arr(i, j) B16[(i) * order + (j)]
#define C32_arr(i, j) C32[(i) * order + (j)]

typedef _Float16 fp16_t;

#if !defined(HETERO_RVV_IME)
extern void gemm_hetero_rvv_x100(const MTYPE * RESTRICT A,
                                 const MTYPE * RESTRICT B,
                                 MTYPE * RESTRICT C,
                                 long order,
                                 long col_begin,
                                 long col_end,
                                 int workers,
                                 int worker_id);
extern void gemm_hetero_rvv_a100(const MTYPE * RESTRICT A,
                                 const MTYPE * RESTRICT B,
                                 MTYPE * RESTRICT C,
                                 long order,
                                 long col_begin,
                                 long col_end,
                                 int workers,
                                 int worker_id);
#else
extern void gemm_hetero_fp16_x100(const fp16_t * RESTRICT A,
                                  const fp16_t * RESTRICT B,
                                  float * RESTRICT C,
                                  long order,
                                  long col_begin,
                                  long col_end,
                                  int workers,
                                  int worker_id);
extern void gemm_hetero_ime_a100(const fp16_t * RESTRICT A,
                                 const fp16_t * RESTRICT B,
                                 float * RESTRICT C,
                                 long order,
                                 long col_begin,
                                 long col_end,
                                 int workers,
                                 int worker_id);
#endif

static int env_int(const char *name, int fallback)
{
  const char *value = getenv(name);
  if (!value || !*value) return fallback;
  return atoi(value);
}

static int parse_cpu_list(const char *spec, int *cpus, int max_cpus)
{
  int count = 0;
  const char *p = spec;

  while (*p && count < max_cpus) {
    char *end;
    long first = strtol(p, &end, 10);
    long last = first;

    if (end == p) break;
    if (*end == '-') {
      p = end + 1;
      last = strtol(p, &end, 10);
    }

    for (long cpu = first; cpu <= last && count < max_cpus; cpu++)
      cpus[count++] = (int)cpu;

    if (*end == ',') p = end + 1;
    else p = end;
  }

  return count;
}

static void bind_x100_worker(const char *name, const char *fallback, int worker_id)
{
  int cpus[CPU_SETSIZE];
  const char *spec = getenv(name);
  int count;
  cpu_set_t set;

  if (!spec || !*spec) spec = fallback;
  count = parse_cpu_list(spec, cpus, CPU_SETSIZE);
  if (count < 1) {
    printf("ERROR: Could not parse CPU list %s=%s\n", name, spec);
    exit(EXIT_FAILURE);
  }

  CPU_ZERO(&set);
  CPU_SET(cpus[worker_id % count], &set);
  if (sched_setaffinity(0, sizeof(set), &set) != 0) {
    printf("ERROR: sched_setaffinity failed for %s worker %d on CPU %d: %s\n",
           name, worker_id, cpus[worker_id % count], strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void write_ai_binding_id(const char *kind, long id)
{
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%ld", id);
  int fd;

  if (len < 1 || len >= (int)sizeof(buf)) {
    printf("ERROR: Could not format A100 %s id %ld for /proc/set_ai_thread\n",
           kind, id);
    exit(EXIT_FAILURE);
  }

  fd = open("/proc/set_ai_thread", O_WRONLY);
  if (fd < 0) {
    printf("ERROR: open /proc/set_ai_thread failed for A100 %s id %ld: %s\n",
           kind, id, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (write(fd, buf, (size_t)len) != len) {
    printf("ERROR: write /proc/set_ai_thread failed for A100 %s id %ld: %s\n",
           kind, id, strerror(errno));
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

static long split_columns(long order, int a100_share)
{
  long a100_cols;
  if (a100_share < 0) a100_share = 0;
  if (a100_share > 100) a100_share = 100;
  a100_cols = (order * (long)a100_share) / 100;
  return order - a100_cols;
}

#if defined(HETERO_RVV_IME)
static inline fp16_t init_a16(long k)
{
  return (fp16_t)((int)(k % 13) - 6);
}

static inline fp16_t init_b16(long k)
{
  return (fp16_t)((int)(k % 7) - 3);
}

static double fp16_reference_checksum(long order, int iterations)
{
  double dot = 0.0;
  for (long k = 0; k < order; k++)
    dot += (double)init_a16(k) * (double)init_b16(k);
  return dot * (double)order * (double)order * (double)(iterations + 1);
}
#endif

int main(int argc, char **argv)
{
  int iterations;
  int x100_threads;
  int a100_threads;
  int total_threads;
  int a100_share;
  int a100_bind_process;
  int block;
  long order;
  long split;
  double start_time = 0.0;
  double elapsed;
  double avgtime;
  double checksum = 0.0;
  double ref_checksum;
  double checksum_error;
#if defined(HETERO_RVV_IME) || defined(MTYPE_IS_FLOAT)
  double epsilon = 1.e-3;
#else
  double epsilon = 1.e-8;
#endif
#if !defined(HETERO_RVV_IME)
  MTYPE * RESTRICT A;
  MTYPE * RESTRICT B;
  MTYPE * RESTRICT C;
  double forder;
#else
  fp16_t * RESTRICT A16;
  fp16_t * RESTRICT B16;
  float * RESTRICT C32;
#endif
  const char *a100_bind_mode;

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("OpenMP heterogeneous GEMM (%s)\n", HETERO_MODE_NAME);

  if (argc != 3 && argc != 4) {
    printf("Usage: %s <# iterations> <matrix order> [tile size]\n", *argv);
    exit(EXIT_FAILURE);
  }

  iterations = atoi(*++argv);
  if (iterations < 1) {
    printf("ERROR: Iterations must be positive: %d\n", iterations);
    exit(EXIT_FAILURE);
  }

  order = atol(*++argv);
  if (order < 1) {
    printf("ERROR: Matrix order must be positive: %ld\n", order);
    exit(EXIT_FAILURE);
  }

  if (argc == 4) block = atoi(*++argv);
  else block = DEFAULTBLOCK;

  x100_threads = env_int("X100_THREADS", 8);
  a100_threads = env_int("A100_THREADS", 8);
  a100_share = env_int("A100_SHARE", 50);
  if (x100_threads < 0 || a100_threads < 0 || x100_threads + a100_threads < 1) {
    printf("ERROR: X100_THREADS and A100_THREADS must request at least one worker\n");
    exit(EXIT_FAILURE);
  }
  total_threads = x100_threads + a100_threads;
  split = split_columns(order, a100_share);
  if (x100_threads == 0) split = 0;
  if (a100_threads == 0) split = order;

  a100_bind_mode = getenv("A100_BIND");
  if (!a100_bind_mode || !*a100_bind_mode)
    a100_bind_mode = (x100_threads == 0) ? "process" : "thread";
  if (strcmp(a100_bind_mode, "process") == 0) {
    if (x100_threads > 0 && a100_threads > 0) {
      printf("ERROR: A100_BIND=process moves the whole process; use A100_BIND=thread for mixed X100+A100 runs\n");
      exit(EXIT_FAILURE);
    }
    a100_bind_process = 1;
  }
  else if (strcmp(a100_bind_mode, "thread") == 0) {
    a100_bind_process = 0;
  }
  else {
    printf("ERROR: A100_BIND must be process or thread\n");
    exit(EXIT_FAILURE);
  }

  if (a100_threads > 0 && a100_bind_process)
    bind_a100_process();

#if !defined(HETERO_RVV_IME)
  forder = 1.0 * order;
  A = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  B = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  C = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  if (!A || !B || !C) {
    printf("ERROR: Could not allocate space for matrices\n");
    exit(EXIT_FAILURE);
  }
  ref_checksum = 0.25 * forder * forder * forder *
                 (forder - 1.0) * (forder - 1.0) * (double)(iterations + 1);

  #pragma omp parallel for
  for (long j = 0; j < order; j++) {
    for (long i = 0; i < order; i++) {
      A_arr(i, j) = B_arr(i, j) = (MTYPE)j;
      C_arr(i, j) = (MTYPE)0.0;
    }
  }
#else
  A16 = (fp16_t *)prk_malloc(order * order * sizeof(fp16_t));
  B16 = (fp16_t *)prk_malloc(order * order * sizeof(fp16_t));
  C32 = (float *)prk_malloc(order * order * sizeof(float));
  if (!A16 || !B16 || !C32) {
    printf("ERROR: Could not allocate space for matrices\n");
    exit(EXIT_FAILURE);
  }
  ref_checksum = fp16_reference_checksum(order, iterations);

  #pragma omp parallel for
  for (long i = 0; i < order; i++) {
    for (long j = 0; j < order; j++) {
      A16_arr(i, j) = init_a16(j);
      B16_arr(i, j) = init_b16(i);
      C32_arr(i, j) = 0.0f;
    }
  }
#endif

  printf("Matrix order          = %ld\n", order);
  printf("Blocking factor       = %d\n", block);
  printf("Number of iterations  = %d\n", iterations);
  printf("X100 threads          = %d\n", x100_threads);
  printf("A100 threads          = %d\n", a100_threads);
  printf("A100 column share     = %d\n", a100_share);
  printf("Column split          = %ld / %ld\n", split, order);
  printf("RVV LMUL              = m%d\n", RVV_LMUL_VALUE);
  printf("X100 bind method      = sched_setaffinity\n");
  printf("A100 bind method      = /proc/set_ai_thread (%s)\n", a100_bind_mode);

  for (int iter = 0; iter <= iterations; iter++) {
    if (iter == 1) start_time = wtime();

    #pragma omp parallel num_threads(total_threads)
    {
      int tid = omp_get_thread_num();
      if (tid < x100_threads) {
        bind_x100_worker("X100_CPUS", "0-7", tid);
#if !defined(HETERO_RVV_IME)
        gemm_hetero_rvv_x100(A, B, C, order, 0, split, x100_threads, tid);
#else
        gemm_hetero_fp16_x100(A16, B16, C32, order, 0, split, x100_threads, tid);
#endif
      }
      else {
        int a100_tid = tid - x100_threads;
        if (!a100_bind_process)
          bind_a100_thread();
#if !defined(HETERO_RVV_IME)
        gemm_hetero_rvv_a100(A, B, C, order, split, order, a100_threads, a100_tid);
#else
        gemm_hetero_ime_a100(A16, B16, C32, order, split, order, a100_threads, a100_tid);
#endif
      }
    }
  }
  elapsed = wtime() - start_time;

#if !defined(HETERO_RVV_IME)
  for (long j = 0; j < order; j++)
    for (long i = 0; i < order; i++)
      checksum += C_arr(i, j);
#else
  for (long j = 0; j < order; j++)
    for (long i = 0; i < order; i++)
      checksum += C32_arr(i, j);
#endif

  checksum_error = ABS(checksum - ref_checksum);
  if ((ref_checksum == 0.0 && checksum_error > epsilon) ||
      (ref_checksum != 0.0 && checksum_error / ABS(ref_checksum) > epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",
           checksum, ref_checksum);
    exit(EXIT_FAILURE);
  }

  printf("Solution validates\n");
  avgtime = elapsed / iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.0E-06 * (2.0 * (double)order * (double)order * (double)order) / avgtime,
         avgtime);

#if !defined(HETERO_RVV_IME)
  prk_free(A);
  prk_free(B);
  prk_free(C);
#else
  prk_free(A16);
  prk_free(B16);
  prk_free(C32);
#endif

  exit(EXIT_SUCCESS);
}
