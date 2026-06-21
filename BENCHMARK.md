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
| 7 | ✅ **DONE** — Metal GPU compute backend **+ GPU BVH** | high | **3–7× vs CPU (measured)** | sphere-only; auto CPU fallback |
| 8 | ✅ **DONE** — binned SAH BVH build | medium | **1.16–1.30× (measured)** | build-only; pixel-identical; helps CPU **and** GPU |
| 9 | ✅ **DONE** — GPU box + cylinder primitives | medium-high | **7.6× on xmas 4K (measured)** | unlocks realistic scenes on the GPU; flat-color surfaces |
| 10 | ✅ **DONE** — GPU cone + ellipsoid + board + triangle | medium | **~6× on all-prim 4K (measured)** | all primitives except CSG now on GPU; fixed 2 CPU/GPU fidelity bugs |

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
`bvh_traverse` call (eliminating per-AABB division), and the lo/hi/origin/
inv pairs are loaded with contiguous `vld1q_f64` rather than element-wise
inserts. Non-ARM builds fall back to the same branchless scalar path with
precomputed reciprocals.

| Scene | Before SIMD | After SIMD | Speedup |
|---|---|---|---|
| benchmark (~13 obj) | 43 ms | 43 ms | 1.00× |
| spheres (400 obj, 15 threads) | 29.5 ms | 23.4 ms | **1.26×** |

BVH traversal is not the bottleneck on the 13-object benchmark; the gain
is visible where there are enough objects that AABB tests dominate. The
remaining CPU SIMD headroom is in a 4-wide BVH (QBVH) that tests four
child boxes per node, or ray packets — both larger rewrites.

### Metal GPU backend + GPU BVH (item 7)

`--gpu` flag enables the Metal path. Scene must contain only sphere
top-level primitives; any non-sphere prim causes automatic CPU fallback.
The GPU executes a per-pixel compute kernel: ray generation → **BVH
traversal** → shadow ray (any-hit BVH) → Phong shading → iterative
reflections up to `maxdepth`. The Metal shader is compiled from embedded
source at startup (no `xcrun`/`metallib` build step).

The CPU-built BVH is flattened into a GPU node buffer (node 0 = root;
leaf `first` indexes a sphere array emitted in BVH leaf order) and
traversed with a per-thread explicit stack. Boxes are expanded by a small
epsilon on export so the `float32` GPU slab test stays conservative
against the `double` build.

GPU renders in `float32`; CPU uses `double` throughout. Outputs are
visually identical (mean per-channel diff < 0.7/255, ~0.06 % of pixels
differ, all at silhouette edges).

**Linear GPU scan vs GPU BVH** — adding the BVH made the GPU faster at
*every* size and made object count nearly free (O(log N) per ray):

| Scene | GPU linear (old) | GPU + BVH (now) |
|---|---|---|
| 400 spheres 4K   | 41.9 ms | **11.7 ms** |
| 5000 spheres 1080p | 155 ms | **11.6 ms** |

**GPU + BVH vs CPU (15 threads + BVH)** — best of 3, warm:

| Scene | CPU (15t) | GPU + BVH | Speedup |
|---|---|---|---|
| 400 spheres 800×800   | 5.5 ms  | 3.5 ms  | 1.6× |
| 1000 spheres 1080p    | 24.8 ms | 7.8 ms  | 3.2× |
| 400 spheres 4K        | 47.0 ms | 11.7 ms | 4.0× |
| 5000 spheres 1080p    | 78.2 ms | 11.6 ms | **6.7×** |

With the GPU BVH the GPU now wins across the board (the earlier linear
scan only won at 4K). Because traversal is O(log N), the 5000-sphere
scene costs barely more than the 400-sphere scene at the same resolution.

### GPU multi-primitive support (item 9)

The Metal backend now renders **boxes** and **cylinders** in addition to
spheres (the three primitives a flat-shaded realistic scene needs). The
GPU primitive buffer became a tagged union (type + center + a 3×3
transform + baked color); the shader dispatches per type in the BVH leaf
loop. The box and cylinder intersection math is ported faithfully from
[box.c](src/box.c) / [conic.c](src/conic.c); a useful simplification is
that the kernel already orients the hit normal toward the viewer, so each
intersector only needs to return the normal *line*, not its sign — which
sidesteps the entry/exit cap-normal bookkeeping.

This unlocks the **1091-object Christmas tree** ([scenes/xmas.json](scenes/xmas.json)),
which previously fell back to the CPU because of its boxes (gifts, floor)
and a cylinder (trunk):

| Scene | CPU (15t) | Metal GPU | Speedup |
|---|---|---|---|
| xmas 4K (1081 spheres + 9 boxes + 1 cylinder) | 177.5 ms | 23.2 ms | **7.6×** |

Geometry, ornaments, shadows, and the reflective floor match the CPU
render; the only visible difference is the background, a procedural
horizontal gradient (paramap→range) that the GPU approximates with a
single flat color sampled along the central view ray (porting procedural
textures to MSL is future work). Scenes using cone/ellipsoid, CSG, or
procedural surfaces still fall back to the CPU automatically.

A latent bug surfaced and was fixed along the way: `RunRange`
([src/range.c](src/range.c)) dereferenced a NULL `defaultv` for a range
with a fade but no default value. The normal CPU render never reaches that
branch (a real ray always lands inside the fade), so the guard leaves all
CPU output bit-identical; it only matters when sampling the background at
an arbitrary point, as the GPU sky-baking does.

### Binned SAH BVH build (item 8)

The build now chooses split planes with the **Surface Area Heuristic**
instead of the spatial median. For each node it bins the primitives into
`NUM_BINS` (12) slots along each of the three axes, sweeps the bins to
evaluate every candidate split by estimated cost
`SA(left)·Nleft + SA(right)·Nright`, and keeps the cheapest — falling back
to the object median only when all centroids coincide. A normalised
leaf-vs-split test (capped at `MAX_LEAF` = 8) avoids over-splitting.

This is a **build-only** change: traversal, the GPU export, and the
intersection math are untouched, so every output is **bit-identical**
(`cmp`-verified on spheres, benchmark, cone, xmas, and the GPU path). A
better-pruned tree simply visits fewer nodes and tests fewer primitives
per ray, which helps **both** the CPU and the (shared) GPU tree.

| Scene | Median split | SAH split | Speedup |
|---|---|---|---|
| spheres (400 obj, 1 thread) | 252 ms | 204 ms | **1.23×** |
| xmas 4K (1091 obj, clustered, CPU) | 216.8 ms | 186.8 ms | **1.16×** |
| spheres5k 1080p (GPU) | 11.6 ms | 8.9 ms | **1.30×** |
| spheres (400 obj, 15 threads) | ~22 ms | ~22 ms | ~1× (noise-bound) |

The win is clearest single-threaded and on clustered geometry; on a
uniform grid the median split is already near-optimal, and at 15 threads
the small scenes are dominated by run-to-run noise.

### GPU: all primitives except CSG (item 10)

The Metal shader now implements **cone, ellipsoid, board, and triangle** in
addition to sphere/box/cylinder — every primitive except CSG. The conic
intersections are ported from [conic.c](src/conic.c); board and triangle were
given bounding spheres ([board.c](src/board.c), [tri.c](src/tri.c)) so they
fit the existing bounded-BVH GPU path (this also lets the **CPU** BVH cull
them — previously they were "always visited").

| Scene | CPU (15t) | Metal GPU | Speedup |
|---|---|---|---|
| all 7 primitives, 4K | 65.9 ms | 11.1 ms | **5.9×** |

Bringing the new primitives up exposed two CPU/GPU fidelity bugs, both fixed:
- **Reflections ignored the surface tint.** The GPU reflection weight used
  only `kspec`, while the CPU multiplies the reflected color by *both* `kspec`
  and the attribute's `reflect` rgb. The GPU now bakes a per-channel
  reflectivity (`reflect × kspec`), so tinted-metal reflections match.
- **Board/triangle self-shadowing.** Primitives with a `nor_func` (sphere,
  box, conic) get a `0.001·normal` shadow-bias offset in `trace()`; board and
  triangle set their hit point exactly on the surface and got none, so their
  shadow ray instantly re-hit the surface and float rounding made the floor
  flicker between lit and shadowed per pixel. `trace()` now applies the same
  bias to non-`nor_func` hits. (`nor_func` scenes stay bit-identical;
  board/triangle scenes get a clean floor.)

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
