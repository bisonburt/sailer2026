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

## Optimization roadmap (proposed, not yet implemented)

Ordered by effort-to-payoff. See git history / PRs for implementation.

| # | Change | Effort | Expected | Risk |
|---|---|---|---|---|
| 1 | ✅ **DONE** — aggressive flags + `release` target | trivial | **1.34× (measured)** | low (`-ffast-math` caveats) |
| 2 | ✅ **DONE** — multithread scanlines (pthreads + per-thread DB clone) | medium | **10.7× (measured)** | resolved via clone + `__thread` CSG scratch |
| 3 | **BVH / spatial index** in `trace()` (currently O(N) per ray, no accel) | medium-high | 2–10× (scales with object count) | medium |
| 4 | Implement the **shadow cache** (counter exists, always 0) | low-medium | 1.1–1.5× | low |
| 5 | `double` → `float` + **NEON / `<simd/simd.h>`** vector math | high | 1.5–2× | precision, broad change |
| 6 | **SIMD ray packets** (4–8 rays/bundle, SoA) | high | 2–4× | large rewrite |
| 7 | **Metal GPU** compute backend | very high | 10–100× | separate backend |

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
