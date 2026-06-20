/*
    bvh.h
    Bounding-volume hierarchy (AABB, binary) over the scene's top-level
    renderables, used to accelerate trace() from O(N) per ray toward
    O(log N). Built from each primitive's bounding sphere (boundinfo).

    A BVH is built per render thread over that thread's own database clone,
    so it is single-threaded data — no locking needed.
*/

#ifndef BVH_H
#define BVH_H

#include "structs.h"

typedef struct bvh bvh_t;

/* Visitor called for each candidate primitive. Return nonzero to stop the
   traversal early (e.g. an occluder was found for a shadow ray). */
typedef int (*bvh_visitor)(prim_type *prim, void *ctx);

/* Build a BVH over the top-level renderables (isInCSG == 0) reachable from
   'database'. Objects without a finite bounding sphere (boundinfo.r <= 0)
   are kept in an "always visit" list. Returns NULL if there is nothing to
   build. */
bvh_t *bvh_build(prim_type *database);

/* Visit every renderable whose bounding box the forward ray may intersect
   (plus all unbounded objects). Stops early if visit() returns nonzero. */
void bvh_traverse(const bvh_t *b, const ray_type *ray,
                  bvh_visitor visit, void *ctx);

void bvh_free(bvh_t *b);

#endif /* BVH_H */
