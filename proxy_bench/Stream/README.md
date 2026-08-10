# K3 Heterogeneous STREAM — Persistent Team Revision

This version fixes the main timing problem in the previous heterogeneous
implementation.

## Key change

The old driver entered a new OpenMP parallel region for every Copy, Scale,
Add and Triad call. In mixed mode that also repeated A100 thread binding
through `/proc/set_ai_thread` inside every timed benchmark invocation.

This revision uses one persistent OpenMP team:

```text
create team once
  |
  +-- bind 8 X100 workers once
  +-- bind 8 A100 workers once
  +-- calculate each worker range once
  |
  +-- initialize
  +-- calibrate
  |
  +-- NTIMES:
        Copy
        Scale
        Add
        Triad
```

Only the architecture-specific STREAM worker plus synchronization barriers
are now part of the timing loop.

## Architecture-specific compilation

Unchanged from the previous revision:

- `stream_x100.c` is compiled with X100 flags.
- `stream_a100.c` is compiled with A100 flags.
- worker functions are noinline.
- LTO is disabled.

## Build

```bash
make clean
make
```

If required:

```bash
make clean
make \
  CC=/home/lin32/opt/llvm-main/bin/clang \
  X100_MARCH="rv64gcv" \
  X100_MCPU="spacemit-x100" \
  A100_MARCH="<your working A100 march>" \
  A100_MCPU="spacemit-a100"
```

## Re-run the three baseline configurations

### X100 only

```bash
X100_THREADS=8 \
A100_THREADS=0 \
X100_CPUS=0-7 \
./stream_hetero
```

### A100 only

```bash
X100_THREADS=0 \
A100_THREADS=8 \
A100_BIND=process \
/home/lin32/bin/ai ./stream_hetero
```

### Mixed 8+8

```bash
X100_THREADS=8 \
A100_THREADS=8 \
X100_CPUS=0-7 \
A100_BIND=thread \
STREAM_A100_SHARE=50 \
./stream_hetero
```

Current old-driver Triad references:

```text
X100 only          16.79 GB/s
A100 only          19.56 GB/s
mixed 8+8 50/50    15.39 GB/s
```

The mixed value must be re-evaluated with this persistent-team driver before
concluding that simultaneous X100+A100 memory traffic is intrinsically slower.

## Then sweep

```bash
make sweep
```

Focus on Triad and Add first. If mixed bandwidth still remains below A100-only
after this correction, that becomes meaningful evidence of shared memory-system
contention rather than repeated team/binding overhead.
