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

| Build | Flags | Render time (median) | Primary rays/s | Speedup | Output |
|---|---|---|---|---|---|
| **Baseline** | `-O2 -g` (default Makefile) | **478 ms** | 5.3 M/s | 1.00× | reference |
| Aggressive flags | `-O3 -ffast-math -flto -mcpu=apple-m4 -fomit-frame-pointer -DNDEBUG` | 360 ms | 7.1 M/s | **1.33×** | pixel-identical |

(Single-threaded in both cases. `-ffast-math` produced byte-identical output for
this scene; verify per-scene before trusting it.)

## Optimization roadmap (proposed, not yet implemented)

Ordered by effort-to-payoff. See git history / PRs for implementation.

| # | Change | Effort | Expected | Risk |
|---|---|---|---|---|
| 1 | Aggressive compiler flags (above) + a `release` make target | trivial | 1.3× (measured) | low (`-ffast-math` caveats) |
| 2 | **Multithread** the scanline loop (GCD `dispatch_apply`) | medium | 6–10× | needs per-thread intersection state |
| 3 | **BVH / spatial index** in `trace()` (currently O(N) per ray, no accel) | medium-high | 2–10× (scales with object count) | medium |
| 4 | Implement the **shadow cache** (counter exists, always 0) | low-medium | 1.1–1.5× | low |
| 5 | `double` → `float` + **NEON / `<simd/simd.h>`** vector math | high | 1.5–2× | precision, broad change |
| 6 | **SIMD ray packets** (4–8 rays/bundle, SoA) | high | 2–4× | large rewrite |
| 7 | **Metal GPU** compute backend | very high | 10–100× | separate backend |

### Key blocker for multithreading
Intersection results are written into **shared** `prim_type.inter` fields during
`trace()`, so parallel rays would clobber each other. Threading requires either
per-thread clones of the object database or moving hit results into per-ray
scratch. The render loop itself is otherwise embarrassingly parallel (independent
pixels), and `ImagePutPixel` should become an indexed `ImageSetPixel(x,y,rgb)`.
