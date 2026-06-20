/*
    bvh.c
    Binary AABB bounding-volume hierarchy over the scene's top-level
    renderables. See bvh.h.

    Build: recursive spatial-median split on the longest axis of the
    centroid bounds (object-median fallback when a split is degenerate).
    Traverse: iterative DFS with a ray/AABB slab test.
*/

#include <stdlib.h>
#include <math.h>
#include "structs.h"
#include "bvh.h"

#define LEAF_SIZE 2
#define EPS 1e-6

typedef struct { point_type lo, hi; } aabb_t;

typedef struct
{
    aabb_t box;
    int left;    /* internal: index of left child (right = left+1); leaf: -1 */
    int first;   /* leaf: start index into order[] */
    int count;   /* leaf: primitive count; internal: 0 */
} node_t;

struct bvh
{
    node_t      *nodes;
    int          nnodes;
    int          cap;
    prim_type  **order;       /* bounded prims, reordered by the build */
    int          nbounded;
    prim_type  **unbounded;   /* no finite bound: always visited */
    int          nunbounded;
};

static aabb_t prim_box(prim_type *p)
{
    aabb_t b;
    double r = p->boundinfo.r;
    b.lo.x = p->boundinfo.c.x - r; b.hi.x = p->boundinfo.c.x + r;
    b.lo.y = p->boundinfo.c.y - r; b.hi.y = p->boundinfo.c.y + r;
    b.lo.z = p->boundinfo.c.z - r; b.hi.z = p->boundinfo.c.z + r;
    return b;
}

static void box_reset(aabb_t *b)
{
    b->lo.x = b->lo.y = b->lo.z =  1e30;
    b->hi.x = b->hi.y = b->hi.z = -1e30;
}

static void box_add(aabb_t *b, const aabb_t *o)
{
    if (o->lo.x < b->lo.x) b->lo.x = o->lo.x;
    if (o->lo.y < b->lo.y) b->lo.y = o->lo.y;
    if (o->lo.z < b->lo.z) b->lo.z = o->lo.z;
    if (o->hi.x > b->hi.x) b->hi.x = o->hi.x;
    if (o->hi.y > b->hi.y) b->hi.y = o->hi.y;
    if (o->hi.z > b->hi.z) b->hi.z = o->hi.z;
}

static double centroid_axis(const aabb_t *b, int axis)
{
    switch (axis)
    {
        case 0: return 0.5 * (b->lo.x + b->hi.x);
        case 1: return 0.5 * (b->lo.y + b->hi.y);
        default: return 0.5 * (b->lo.z + b->hi.z);
    }
}

static int alloc_node(bvh_t *b)
{
    if (b->nnodes >= b->cap)
    {
        b->cap = b->cap ? b->cap * 2 : 8;
        b->nodes = (node_t *) realloc(b->nodes, b->cap * sizeof(node_t));
    }
    return b->nnodes++;
}

/* Recursively build a subtree over order[start..end). Returns node index. */
static int build_node(bvh_t *b, int start, int end)
{
    int self = alloc_node(b);
    aabb_t bounds, cbounds;
    int i, axis, mid;
    double extent[3], mids;

    /* bounds of all primitive boxes, and of their centroids */
    box_reset(&bounds);
    box_reset(&cbounds);
    for (i = start; i < end; i++)
    {
        aabb_t pb = prim_box(b->order[i]);
        aabb_t cb;
        box_add(&bounds, &pb);
        cb.lo.x = cb.hi.x = 0.5 * (pb.lo.x + pb.hi.x);
        cb.lo.y = cb.hi.y = 0.5 * (pb.lo.y + pb.hi.y);
        cb.lo.z = cb.hi.z = 0.5 * (pb.lo.z + pb.hi.z);
        box_add(&cbounds, &cb);
    }
    b->nodes[self].box = bounds;

    if (end - start <= LEAF_SIZE)
    {
        b->nodes[self].left = -1;
        b->nodes[self].first = start;
        b->nodes[self].count = end - start;
        return self;
    }

    /* choose the longest centroid axis */
    extent[0] = cbounds.hi.x - cbounds.lo.x;
    extent[1] = cbounds.hi.y - cbounds.lo.y;
    extent[2] = cbounds.hi.z - cbounds.lo.z;
    axis = 0;
    if (extent[1] > extent[axis]) axis = 1;
    if (extent[2] > extent[axis]) axis = 2;

    mids = centroid_axis(&cbounds, axis); /* spatial median */

    /* partition order[start..end) about mids on 'axis' */
    {
        int lo = start, hi = end - 1;
        while (lo <= hi)
        {
            aabb_t pb = prim_box(b->order[lo]);
            if (centroid_axis(&pb, axis) < mids)
                lo++;
            else
            {
                prim_type *tmp = b->order[lo];
                b->order[lo] = b->order[hi];
                b->order[hi] = tmp;
                hi--;
            }
        }
        mid = lo;
    }

    /* fall back to the object median if the split was degenerate */
    if (mid == start || mid == end)
        mid = (start + end) / 2;

    {
        int l = build_node(b, start, mid);
        int r = build_node(b, mid, end);
        (void) r;                 /* right child is always l+... see note */
        b->nodes[self].left = l;  /* children are NOT guaranteed contiguous */
        b->nodes[self].first = r; /* reuse 'first' to store right child idx */
        b->nodes[self].count = 0;
    }
    return self;
}

bvh_t *bvh_build(prim_type *database)
{
    bvh_t *b;
    prim_type *p;
    int total = 0;

    for (p = database; p != NULL; p = p->next)
        if (p->isInCSG == 0) total++;
    if (total == 0) return NULL;

    b = (bvh_t *) calloc(1, sizeof(bvh_t));
    b->order     = (prim_type **) malloc(total * sizeof(prim_type *));
    b->unbounded = (prim_type **) malloc(total * sizeof(prim_type *));
    b->nbounded = b->nunbounded = 0;

    for (p = database; p != NULL; p = p->next)
    {
        if (p->isInCSG != 0) continue;
        if (p->boundinfo.r > 0.0) b->order[b->nbounded++] = p;
        else                      b->unbounded[b->nunbounded++] = p;
    }

    if (b->nbounded > 0)
        build_node(b, 0, b->nbounded);

    return b;
}

/* Forward ray vs AABB slab test (hits at t >= 0). */
static int ray_aabb(const aabb_t *bx, const ray_type *r)
{
    double tmin = 0.0, tmax = 1e30;
    double t1, t2, inv;

    /* x */
    if (fabs(r->d.x) < EPS) { if (r->s.x < bx->lo.x || r->s.x > bx->hi.x) return 0; }
    else { inv = 1.0/r->d.x; t1 = (bx->lo.x - r->s.x)*inv; t2 = (bx->hi.x - r->s.x)*inv;
           if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
           if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return 0; }
    /* y */
    if (fabs(r->d.y) < EPS) { if (r->s.y < bx->lo.y || r->s.y > bx->hi.y) return 0; }
    else { inv = 1.0/r->d.y; t1 = (bx->lo.y - r->s.y)*inv; t2 = (bx->hi.y - r->s.y)*inv;
           if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
           if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return 0; }
    /* z */
    if (fabs(r->d.z) < EPS) { if (r->s.z < bx->lo.z || r->s.z > bx->hi.z) return 0; }
    else { inv = 1.0/r->d.z; t1 = (bx->lo.z - r->s.z)*inv; t2 = (bx->hi.z - r->s.z)*inv;
           if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
           if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2; if (tmin > tmax) return 0; }

    return tmax >= 0.0;
}

void bvh_traverse(const bvh_t *b, const ray_type *ray,
                  bvh_visitor visit, void *ctx)
{
    int stack[128];
    int sp = 0;
    int i;

    if (b == NULL) return;

    for (i = 0; i < b->nunbounded; i++)
        if (visit(b->unbounded[i], ctx)) return;

    if (b->nnodes == 0) return;

    stack[sp++] = 0;
    while (sp > 0)
    {
        const node_t *n = &b->nodes[stack[--sp]];
        if (!ray_aabb(&n->box, ray)) continue;

        if (n->count > 0) /* leaf */
        {
            for (i = 0; i < n->count; i++)
                if (visit(b->order[n->first + i], ctx)) return;
        }
        else /* internal: left in .left, right in .first */
        {
            if (sp + 2 <= (int)(sizeof(stack)/sizeof(stack[0])))
            {
                stack[sp++] = n->left;
                stack[sp++] = n->first;
            }
        }
    }
}

void bvh_free(bvh_t *b)
{
    if (b == NULL) return;
    free(b->nodes);
    free(b->order);
    free(b->unbounded);
    free(b);
}
