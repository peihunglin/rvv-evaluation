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

#ifndef HETERO_MODE_NAME
#define HETERO_MODE_NAME "rvv_rvv"
#endif

#define A_arr(i,j) A[(i) + order*(j)]
#define B_arr(i,j) B[(i) + order*(j)]
#define C_arr(i,j) C[(i) + order*(j)]
#define A16_arr(i,j) A16[(i)*order + (j)]
#define B16_arr(i,j) B16[(i)*order + (j)]
#define C32_arr(i,j) C32[(i)*order + (j)]

typedef _Float16 fp16_t;

#if !defined(HETERO_RVV_IME)
extern void gemm_hetero_rvv_x100(const MTYPE * RESTRICT,
                                 const MTYPE * RESTRICT,
                                 MTYPE * RESTRICT,
                                 long,long,long,int,int,int);
extern void gemm_hetero_rvv_a100(const MTYPE * RESTRICT,
                                 const MTYPE * RESTRICT,
                                 MTYPE * RESTRICT,
                                 long,long,long,int,int,int);
#else
extern void gemm_hetero_fp16_x100(const fp16_t * RESTRICT,
                                  const fp16_t * RESTRICT,
                                  float * RESTRICT,
                                  long,long,long,int,int);
extern void gemm_hetero_ime_a100(const fp16_t * RESTRICT,
                                 const fp16_t * RESTRICT,
                                 float * RESTRICT,
                                 long,long,long,int,int);
#endif

static int env_int(const char *name,int fallback)
{
  const char *v=getenv(name);
  return (!v || !*v) ? fallback : atoi(v);
}

static int parse_cpu_list(const char *spec,int *cpus,int max_cpus)
{
  int count=0;
  const char *p=spec;
  while(*p && count<max_cpus) {
    char *end;
    long first=strtol(p,&end,10), last=first;
    if(end==p) break;
    if(*end=='-') {
      p=end+1;
      last=strtol(p,&end,10);
    }
    for(long cpu=first;cpu<=last && count<max_cpus;++cpu)
      cpus[count++]=(int)cpu;
    p=(*end==',') ? end+1 : end;
  }
  return count;
}

static void bind_x100_worker(const char *name,const char *fallback,int worker_id)
{
  int cpus[CPU_SETSIZE];
  const char *spec=getenv(name);
  cpu_set_t set;
  if(!spec || !*spec) spec=fallback;
  int count=parse_cpu_list(spec,cpus,CPU_SETSIZE);
  if(count<1) {
    fprintf(stderr,"ERROR: bad CPU list %s=%s\n",name,spec);
    exit(EXIT_FAILURE);
  }
  CPU_ZERO(&set);
  CPU_SET(cpus[worker_id%count],&set);
  if(sched_setaffinity(0,sizeof(set),&set)!=0) {
    fprintf(stderr,"ERROR: sched_setaffinity worker %d: %s\n",
            worker_id,strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void write_ai_binding_id(const char *kind,long id)
{
  char buf[32];
  int len=snprintf(buf,sizeof(buf),"%ld",id);
  int fd=open("/proc/set_ai_thread",O_WRONLY);
  if(fd<0) {
    fprintf(stderr,"ERROR: open /proc/set_ai_thread (%s): %s\n",
            kind,strerror(errno));
    exit(EXIT_FAILURE);
  }
  if(write(fd,buf,(size_t)len)!=len) {
    fprintf(stderr,"ERROR: write /proc/set_ai_thread (%s): %s\n",
            kind,strerror(errno));
    close(fd);
    exit(EXIT_FAILURE);
  }
  close(fd);
}

static void bind_a100_thread(void)
{
  write_ai_binding_id("thread",(long)syscall(SYS_gettid));
}

static void bind_a100_process(void)
{
  write_ai_binding_id("process",(long)getpid());
}

/* Align heterogeneous boundary to 8 columns, satisfying X100 NR=8
 * and A100 NR=4 without introducing a boundary-tail penalty.
 */
static long split_columns(long order,int a100_share)
{
  if(a100_share<0) a100_share=0;
  if(a100_share>100) a100_share=100;

  long a100_cols=(order*(long)a100_share)/100;
  long split=order-a100_cols;

if(split>0 && split<order) {
#if defined(HETERO_RVV_IME)
    /* A100 rm=2,rn=4 consumes 32-column macro-panels. */
    split=(split/32)*32;
#else
    split=(split/8)*8;
#endif
  }
  return split;
}

#if defined(HETERO_RVV_IME)
static inline fp16_t init_a16(long k)
{ return (fp16_t)((int)(k%13)-6); }
static inline fp16_t init_b16(long k)
{ return (fp16_t)((int)(k%7)-3); }
static double fp16_reference_checksum(long n,int iterations)
{
  double dot=0.0;
  for(long k=0;k<n;++k) dot+=(double)init_a16(k)*(double)init_b16(k);
  return dot*(double)n*(double)n*(double)(iterations+1);
}
#endif

int main(int argc,char **argv)
{
  if(argc!=3 && argc!=4) {
    printf("Usage: %s <# iterations> <matrix order> [KC/tile]\n",argv[0]);
    return EXIT_FAILURE;
  }

  const int iterations=atoi(argv[1]);
  const long order=atol(argv[2]);
  const int block=(argc==4)?atoi(argv[3]):64;

  if(iterations<1 || order<1 || block<1) return EXIT_FAILURE;

  const int x100_threads=env_int("X100_THREADS",8);
  const int a100_threads=env_int("A100_THREADS",8);
  const int total_threads=x100_threads+a100_threads;
  const int a100_share=env_int("A100_SHARE",40);

  if(x100_threads<0 || a100_threads<0 || total_threads<1)
    return EXIT_FAILURE;

  long split=split_columns(order,a100_share);
  if(x100_threads==0) split=0;
  if(a100_threads==0) split=order;

  const char *a100_bind_mode=getenv("A100_BIND");
  if(!a100_bind_mode || !*a100_bind_mode)
    a100_bind_mode=(x100_threads==0)?"process":"thread";

  const int a100_bind_process=!strcmp(a100_bind_mode,"process");
  if(a100_bind_process && x100_threads>0 && a100_threads>0) {
    fprintf(stderr,"ERROR: use A100_BIND=thread for mixed runs\n");
    return EXIT_FAILURE;
  }
  if(strcmp(a100_bind_mode,"process") && strcmp(a100_bind_mode,"thread")) {
    fprintf(stderr,"ERROR: A100_BIND must be process or thread\n");
    return EXIT_FAILURE;
  }
  if(a100_threads>0 && a100_bind_process) bind_a100_process();

  double checksum=0.0,ref_checksum=0.0,start_time=0.0,elapsed;
#if defined(HETERO_RVV_IME) || defined(MTYPE_IS_FLOAT)
  const double epsilon=1.e-3;
#else
  const double epsilon=1.e-8;
#endif

#if !defined(HETERO_RVV_IME)
  MTYPE *A=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  MTYPE *B=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  MTYPE *C=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  if(!A||!B||!C) return EXIT_FAILURE;

  const double fn=(double)order;
  ref_checksum=0.25*fn*fn*fn*(fn-1.0)*(fn-1.0)*(double)(iterations+1);

  #pragma omp parallel for schedule(static)
  for(long j=0;j<order;++j)
    for(long i=0;i<order;++i) {
      A_arr(i,j)=B_arr(i,j)=(MTYPE)j;
      C_arr(i,j)=(MTYPE)0;
    }
#else
  fp16_t *A16=(fp16_t*)prk_malloc(order*order*sizeof(fp16_t));
  fp16_t *B16=(fp16_t*)prk_malloc(order*order*sizeof(fp16_t));
  float *C32=(float*)prk_malloc(order*order*sizeof(float));
  if(!A16||!B16||!C32) return EXIT_FAILURE;

  ref_checksum=fp16_reference_checksum(order,iterations);
  #pragma omp parallel for schedule(static)
  for(long i=0;i<order;++i)
    for(long j=0;j<order;++j) {
      A16_arr(i,j)=init_a16(j);
      B16_arr(i,j)=init_b16(i);
      C32_arr(i,j)=0.0f;
    }
#endif

  printf("Parallel Research Kernels version %s\n",PRKVERSION);
  printf("OpenMP heterogeneous GEMM (%s) - optimized workers\n",HETERO_MODE_NAME);
  printf("Matrix order          = %ld\n",order);
#if !defined(HETERO_RVV_IME)
  printf("RVV KC                = %d\n",block);
  printf("X100 kernel           = MC32/NC64/NR8/KU2\n");
  printf("A100 kernel           = MC64/NC64/NR4/KU2\n");
#else
  printf("X100 kernel           = FP16 RVV MR8 x VL, FP32 accum\n");
  printf("A100 kernel           = IME rm2/rn4 16x32, Kblock8\n");
#endif
  printf("Number of iterations  = %d\n",iterations);
  printf("X100 threads          = %d\n",x100_threads);
  printf("A100 threads          = %d\n",a100_threads);
  printf("A100 column share     = %d\n",a100_share);
  printf("Column split          = %ld / %ld\n",split,order);
  printf("Persistent OMP team   = yes\n");

  omp_set_dynamic(0);

  #pragma omp parallel num_threads(total_threads) shared(start_time)
  {
    const int tid=omp_get_thread_num();

    /* Bind once, not once per iteration. */
    if(tid<x100_threads) {
      bind_x100_worker("X100_CPUS","0-7",tid);
    } else if(!a100_bind_process) {
      bind_a100_thread();
    }

    #pragma omp barrier

    for(int iter=0;iter<=iterations;++iter) {
      if(iter==1) {
        #pragma omp barrier
        #pragma omp master
        start_time=wtime();
      }

      if(tid<x100_threads) {
#if !defined(HETERO_RVV_IME)
        gemm_hetero_rvv_x100(A,B,C,order,0,split,
                             x100_threads,tid,block);
#else
        gemm_hetero_fp16_x100(A16,B16,C32,order,0,split,
                              x100_threads,tid);
#endif
      } else {
        const int atid=tid-x100_threads;
#if !defined(HETERO_RVV_IME)
        gemm_hetero_rvv_a100(A,B,C,order,split,order,
                             a100_threads,atid,block);
#else
        gemm_hetero_ime_a100(A16,B16,C32,order,split,order,
                             a100_threads,atid);
#endif
      }

      #pragma omp barrier
    }
  }

  elapsed=wtime()-start_time;

#if !defined(HETERO_RVV_IME)
  for(long j=0;j<order;++j)
    for(long i=0;i<order;++i) checksum+=C_arr(i,j);
#else
  for(long j=0;j<order;++j)
    for(long i=0;i<order;++i) checksum+=C32_arr(i,j);
#endif

  const double err=ABS(checksum-ref_checksum);
  if((ref_checksum==0.0 && err>epsilon) ||
     (ref_checksum!=0.0 && err/ABS(ref_checksum)>epsilon)) {
    printf("ERROR: Checksum = %lf, Reference = %lf\n",checksum,ref_checksum);
    return EXIT_FAILURE;
  }

  printf("Solution validates\n");
  const double avg=elapsed/(double)iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.e-6*(2.0*(double)order*(double)order*(double)order)/avg,avg);

#if !defined(HETERO_RVV_IME)
  prk_free(A); prk_free(B); prk_free(C);
#else
  prk_free(A16); prk_free(B16); prk_free(C32);
#endif
  return EXIT_SUCCESS;
}
