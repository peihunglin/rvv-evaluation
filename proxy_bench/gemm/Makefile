include ./common/OPENMP.defs

##### User configurable options #####

OPTFLAGS     = $(DEFAULT_OPT_FLAGS) 
#description: change above into something that is a decent optimization on you system

#uncomment any of the following flags (and change values) to change defaults

USERFLAGS     =    
#description: parameter to specify optional flags

EXTOBJS      = 
#LIBS         = -lmkl_ia32
#LIBPATHS     = -L/opt/intel/mkl/10.0.3.020/lib/32
#INCLUDEPATHS = -I/opt/intel/mkl/10.0.3.020/include

#LIBS += -lmyperf
#LIBPATHS += -L../libperf
#INCLUDEPATHS += -I../libperf

### End User configurable options ###

ifndef MKL
  MKL=0
endif
#description: set this flag to call the tuned mkl library

ifndef VERBOSE
  VERBOSE=0
endif
#description: default diagnostic style is silent

ifndef RESTRICT_KEYWORD
  RESTRICT_KEYWORD=0
endif
#description: the "restrict" keyword can be used on IA platforms to disambiguate  
#             data accessed through pointers (requires -restrict compiler flag)

ifndef DEFAULTBLOCK
  DEFAULTBLOCK=32
endif
#description: default tile size is 32 (used only for non-MKL version)

ifndef BOFFSET
  BOFFSET=12
endif
#description: set this flag to some value to override default first array 
#             dimension padding (12) of tiles used in non-MKL version

ifndef MAXTHREADS
  MAXTHREADS=512
endif
#description: default thread limit is 512

nonvec=0
clang=0
ime=0
rvv=0
hetero=0
RVV_LMUL ?=
RVV_INTRINSIC_LMUL ?= 1
RVV_MCPU ?=
HETERO_MODE ?= rvv_rvv
HETERO_RVV_LMUL ?= 1
HETERO_X100_MCPU ?= spacemit-x100
HETERO_A100_MCPU ?= spacemit-a100
IME_MARCH ?= rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii
USERFLAGS += -static
ifeq (${ime},1)
	USERFLAGS += -DUSE_IME_ASM
	ifeq (${clang},1)
		USERFLAGS += -march=$(IME_MARCH) -mcpu=spacemit-a100 -fno-vectorize -fno-slp-vectorize
	else
		USERFLAGS += -march=$(IME_MARCH) -mcpu=spacemit-a100 -fno-tree-vectorize
	endif
else
ifeq (${hetero},1)
	USERFLAGS += -DUSE_HETERO -DRVV_LMUL_VALUE=$(HETERO_RVV_LMUL) -march=rv64gcv_zfh_zvfh
	ifeq ($(HETERO_MODE),rvv_ime)
		USERFLAGS += -DHETERO_RVV_IME=1 -DHETERO_MODE_NAME=\"rvv_ime\"
	else
		USERFLAGS += -DHETERO_MODE_NAME=\"rvv_rvv\"
	endif
else
ifeq (${rvv},1)
	USERFLAGS += -DUSE_RVV_INTRINSICS -DRVV_LMUL_VALUE=$(RVV_INTRINSIC_LMUL) -march=rv64gcv_zfh_zvfh
	ifneq ($(RVV_MCPU),)
		USERFLAGS += -mcpu=$(RVV_MCPU)
	endif
else
ifeq (${nonvec},1)
	ifeq (${clang},1)
		USERFLAGS += -march=rv64gc -mllvm -scalable-vectorization=off
	else
		USERFLAGS += -march=rv64gc -fno-tree-vectorize
	endif
endif

ifeq (${nonvec},0)
	ifeq (${clang},1)
		USERFLAGS += -march=rv64gcv -mllvm -scalable-vectorization=on
		ifneq ($(RVV_LMUL),)
			USERFLAGS += -mllvm -riscv-v-register-bit-width-lmul=$(RVV_LMUL)
		endif
	else
		USERFLAGS += -march=rv64gcv
		ifneq ($(RVV_LMUL),)
			USERFLAGS += -mrvv-max-lmul=m$(RVV_LMUL)
		endif
	endif
endif
endif
endif
endif

float=0
ifeq ($(float),1)
        USERFLAGS += -DMTYPE=float -DMTYPE_IS_FLOAT=1
else
        USERFLAGS += -DMTYPE_IS_DOUBLE=1
endif


PROGRAM     = gemm
MKLFLAG     = -DMKL=$(MKL)
VERBOSEFLAG = -DVERBOSE=$(VERBOSE)
RESTRICTFLAG= -DRESTRICT_KEYWORD=$(RESTRICT_KEYWORD)
BLOCKFLAG   = -DDEFAULTBLOCK=$(DEFAULTBLOCK)
OFFSETFLAG  = -DBOFFSET=$(BOFFSET)
NTHREADFLAG = -DMAXTHREADS=$(MAXTHREADS)

OPTIONSSTRING="Make options:\n\
OPTION                 MEANING                                              DEFAULT \n\
MKL=0/1                disable/enable MKL library                              [0]  \n\
RESTRICT_KEYWORD=0/1   disable/enable restrict keyword (aliasing)              [0]  \n\
DEFAULTBLOCK=?         set tile size (non-MKL version)                         [32] \n\
BOFFSET=?              override default first array dimension padding of tiles [12] \n\
MAXTHREADS=?           set maximum number of OpenMP threads                    [512]\n\
VERBOSE=0/1            omit/include verbose run information                    [0]"

TUNEFLAGS   = $(BLOCKFLAG)   $(MKLFLAG)     $(OFFSETFLAG) $(RESTRICTFLAG) \
              $(VERBOSEFLAG) $(NTHREADFLAG) $(USERFLAGS)
OBJS        = $(PROGRAM).o $(COMOBJS) 
IME_OBJS    = gemm_ime.o $(COMOBJS)
RVV_OBJS    = gemm_rvv.o $(COMOBJS)
HETERO_RVV_RVV_OBJS = gemm_hetero.o gemm_hetero_rvv_x100.o gemm_hetero_rvv_a100.o $(COMOBJS)
HETERO_RVV_IME_OBJS = gemm_hetero.o gemm_hetero_fp16_x100.o gemm_hetero_ime_a100.o $(COMOBJS)
OPT_OBJS = gemm_opt.o $(COMOBJS)
RVV_X100_OPT_OBJS = gemm_rvv_x100_opt.o $(COMOBJS)
RVV_A100_OPT_OBJS = gemm_rvv_a100_opt.o $(COMOBJS)
IME_OPT_OBJS = gemm_ime_opt.o $(COMOBJS)
EXTRA_CLEAN = gemm_ime gemm_ime.o gemm_rvv gemm_rvv.o gemm_hetero gemm_hetero.o gemm_hetero_rvv_x100.o gemm_hetero_rvv_a100.o gemm_hetero_fp16_x100.o gemm_hetero_ime_a100.o gemm_opt gemm_opt.o gemm_rvv_x100_opt gemm_rvv_x100_opt.o gemm_rvv_a100_opt gemm_rvv_a100_opt.o gemm_ime_opt gemm_ime_opt.o

include ./common/make.common

gemm_ime: $(IME_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(IME_OBJS) $(EXTOBJS) $(LIBS)

gemm_rvv: $(RVV_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(RVV_OBJS) $(EXTOBJS) $(LIBS)

gemm_hetero: $(if $(filter rvv_ime,$(HETERO_MODE)),$(HETERO_RVV_IME_OBJS),$(HETERO_RVV_RVV_OBJS))
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $^ $(EXTOBJS) $(LIBS)

gemm_hetero_rvv_x100.o: gemm_hetero_rvv_kernel.c
	$(CCOMPILER) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHSPLUS) -mcpu=$(HETERO_X100_MCPU) -DKERNEL_NAME=gemm_hetero_rvv_x100 -c $< -o $@

gemm_hetero_rvv_a100.o: gemm_hetero_rvv_kernel.c
	$(CCOMPILER) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHSPLUS) -mcpu=$(HETERO_A100_MCPU) -DKERNEL_NAME=gemm_hetero_rvv_a100 -c $< -o $@

gemm_hetero_fp16_x100.o: gemm_hetero_fp16_x100_kernel.c
	$(CCOMPILER) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHSPLUS) -mcpu=$(HETERO_X100_MCPU) -c $< -o $@

gemm_hetero_ime_a100.o: gemm_hetero_ime_kernel.c
	$(CCOMPILER) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHSPLUS) -march=$(IME_MARCH) -mcpu=$(HETERO_A100_MCPU) -DUSE_IME_ASM -c $< -o $@

gemm_opt: $(OPT_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(OPT_OBJS) $(EXTOBJS) $(LIBS)

gemm_rvv_x100_opt: $(RVV_X100_OPT_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(RVV_X100_OPT_OBJS) $(EXTOBJS) $(LIBS)

gemm_rvv_a100_opt: $(RVV_A100_OPT_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(RVV_A100_OPT_OBJS) $(EXTOBJS) $(LIBS)

gemm_ime_opt: $(IME_OPT_OBJS)
	$(CLINKER) -o $@ $(LIBPATHS) $(CFLAGS) $(IME_OPT_OBJS) $(EXTOBJS) $(LIBS)

gemm_ime_opt.o: gemm_ime_opt.c
	$(CCOMPILER) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHSPLUS) -march=$(IME_MARCH) -mcpu=spacemit-a100 -DUSE_IME_ASM -c $< -o $@
