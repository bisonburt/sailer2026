# Performance baseline & optimization roadmap

This file pins a reproducible performance **baseline** so future optimizations
can be measured against it.

## Test machine

| | |
|---|---|
| Chip | Apple **M5 Pro** — 15 cores (5 performance + 10 efficiency) |
| Memory | 24 GB |
| Compiler | Apple clang 17 (arm64) |
| Frameworks available | Accelerate, Metal |

## Benchmark scene

[`scenes/benchmark.json`](scenes/benchmark.json) — the `playground` scene at
**1600×1600**, reflection depth **8**, ~13 primitives (spheres, a cylinder,
a checkerboard floor, two CSG combos, an FNOISE marble paramap, a gradient sky).
= 2,560,000 primary rays plus shadow + reflection rays.

`render time` is printed by the tracer and times **only the ray-tracing loop**
(image encode/write excluded). Take the median of 5 runs.

```sh
make                                            # -O2 baseline build
for i in 1 2 3 4 5; do ./ray scenes/benchmark.json -o /tmp/b.png | grep "render time"; done
```

## Results

| Build | Threads | Render time (best) | Speedup | Output |
|---|---|---|---|---|
| **Baseline** `-O2 -g` | 1 | **478 ms** | 1.00× | reference |
| Release `-O3 -ffast-math -flto -mcpu` | 1 | 357 ms | 1.34× | identical |
| Baseline `-O2` | 15 | 43.9 ms | 10.9× | identical |
| **Release + threads** | 15 | **35.6 ms** | **13.4×** | identical |

Threaded scaling (default `-O2`, `scenes/benchmark.json`):

| Threads | Time | Speedup |
|---|---|---|
| 1 | 468 ms | 1.0× |
| 2 | 237 ms | 1.97× |
| 4 | 124 ms | 3.79× |
| 8 | 71 ms | 6.57× |
| 15 | 44 ms | 10.7× |

Near-linear to the 5 performance cores, then the 10 efficiency cores add the
rest. Output is **pixel-identical** across all thread counts and against the
baseline (verified with `cmp`). `-ffast-math` also produced identical output for
this scene; verify per-scene before trusting it.

Run it yourself: `make release && ./ray scenes/benchmark.json -t 15`

### BVH spatial index (item 3)

A BVH helps in proportion to object count. On the ~13-object benchmark its
traversal overhead roughly cancels the savings; on a **400-sphere** scene
([`scenes/spheres.json`](scenes/spheres.json)) it is transformative. All outputs
are pixel-identical to the linear scan (`cmp`-verified).

| Scene | Build | Linear (no BVH) | BVH | Speedup |
|---|---|---|---|---|
| benchmark (~13 obj) | 1 thread | 471 ms | 485 ms | 0.97× |
| spheres (400 obj) | 1 thread | 4004 ms | 303 ms | **13.2×** |
| spheres (400 obj) | 15 threads | 424 ms | 29.5 ms | **14.4×** |

400-sphere scene, single-thread linear → **BVH + 15 threads**: 4004 ms → 29.5 ms
= **136×**.

### Shadow cache (item 4)

Each shaded point casts a shadow ray toward the light; the object that occluded
the previous shadow ray is tested first and skips the full traversal on a hit.
With the BVH already making shadow traversal cheap, the remaining gain is small
(single-thread, BVH on in both):

| Scene | No cache | + Shadow cache | Speedup |
|---|---|---|---|
| benchmark | 494 ms | 482 ms | 1.02× |
| spheres (400 obj) | 298 ms | 287 ms | 1.04× |

Output is pixel-identical. The big shadow-cache wins belonged to the pre-BVH
O(N) era; it remains a cheap, safe extra here.

## Optimization roadmap

Ordered by effort-to-payoff. See git history / PRs for implementation.

| # | Change | Effort | Expected | Risk |
|---|---|---|---|---|
| 1 | ✅ **DONE** — aggressive flags + `release` target | trivial | **1.34× (measured)** | low (`-ffast-math` caveats) |
| 2 | ✅ **DONE** — multithread scanlines (pthreads + per-thread DB clone) | medium | **10.7× (measured)** | resolved via clone + `__thread` CSG scratch |
| 3 | ✅ **DONE** — BVH/AABB spatial index in `trace()` | medium-high | **13× @ 400 objects (measured)** | resolved; per-thread, pixel-identical |
| 4 | ✅ **DONE** — shadow cache (last-occluder reuse) | low | **~1.02–1.04× (measured)** | low; small now that the BVH already accelerates shadow rays |
| 5 | ❌ **REJECTED** — naive `double`→`float` (point/rgb types) | medium | **slower + crashed** | see findings below |
| 6 | ✅ **DONE** — NEON SIMD BVH AABB tests | low-medium | **1.26× @ 400 objects (measured)** | ARM-only; scalar fallback for other platforms |
| 7 | ✅ **DONE** — Metal GPU compute backend | high | **1.11× @ 4K / 400 spheres** | sphere-only; scales better at high resolution |

### Item 5 findings — why scalar `float` was rejected
Converting `point_type`/`rgb_type` from `double` to `float` (leaving math
intermediates and matrices `double`) was implemented and measured, then
reverted:
- **Slower**, not faster: spheres 280 → 315 ms single-thread. Apple Silicon
  has a full-throughput double FPU, so narrowing storage gives no compute win,
  while the mixed float-storage / double-math paths add constant float↔double
  **conversion** overhead on every component access.
- **Crashed** (SIGSEGV) on `benchmark.json`: reduced precision pushed an index
  out of bounds in the noise/CSG path.

Lesson: a scalar type swap is the wrong lever here. Real SIMD gains require
processing *multiple* values per instruction (item 6, ray packets in SoA
layout) or moving to the GPU (item 7) — not narrowing the scalar type. Items
1–4 (flags, threads, BVH) already deliver the bulk of the achievable CPU win.

### NEON SIMD BVH (item 6)

ARM NEON `float64x2_t` processes two slab axes (X and Y) in one SIMD
pass; Z is scalar. Reciprocal ray directions are precomputed once per
`bvh_traverse` call (eliminating per-AABB division). Non-ARM builds
fall back to the same branchless scalar path with precomputed reciprocals.

| Scene | Before SIMD | After SIMD | Speedup |
|---|---|---|---|
| benchmark (~13 obj) | 43 ms | 43 ms | 1.00× |
| spheres (400 obj, 15 threads) | 29.5 ms | 23.4 ms | **1.26×** |

BVH traversal is not the bottleneck on the 13-object benchmark; the gain
is visible where there are enough objects that AABB tests dominate.

### Metal GPU backend (item 7)

`--gpu` flag enables the Metal path. Scene must contain only sphere
top-level primitives; any non-sphere prim causes automatic CPU fallback.
The GPU executes a per-pixel compute kernel: ray generation → linear
sphere scan → shadow ray → Phong shading → iterative reflections up to
`maxdepth`. The Metal shader is compiled from embedded source at startup
(no `xcrun`/`metallib` build step).

GPU renders in `float32`; CPU uses `double` throughout. Outputs are
visually identical (mean per-channel diff < 0.7/255); occasional
silhouette edge pixels may differ due to precision.

| Scene | CPU (15 threads + BVH) | Metal GPU | GPU vs CPU |
|---|---|---|---|
| 400 spheres 800×800 | 5.8 ms | 7.8 ms warm | 0.74× (CPU wins; BVH > GPU linear) |
| 400 spheres 1080p | 23 ms | 27 ms warm | 0.85× |
| 400 spheres 4K | 46.5 ms | **41.9 ms** | **1.11×** (GPU wins at high res) |

The GPU crossover is at approximately 4K resolution for sphere-only scenes.
The GPU linear scan is O(N) per ray; a GPU BVH would push the crossover to
lower resolutions and extend the advantage to larger scenes.

### How the multithreading blocker was resolved (item 2)
Intersection results are written into **shared** `prim_type.inter` fields during
`trace()`, and CSG used a shared static `interT[]` scratch array. The fix:
- `CloneObjectDatabase()` ([src/init.c](src/init.c)) gives each thread its own
  deep copy of the primitive list, remapping `next`, CSG `left`/`right`, and the
  `origin` back-pointers; read-only surface modules stay shared.
- `database` is now `__thread`; each worker clones once and frees on exit.
- CSG's `interT`/`curinter` are `__thread` ([src/csg.c](src/csg.c)).
- `ImageSetPixel(x,y,rgb)` lets workers write disjoint pixels; scanlines are
  interleaved (`row % nthreads`) for load balance across P/E cores.
- The stat counters (`G_*`, one was always 0) were removed rather than made
  atomic, to keep the hot path lock-free.
