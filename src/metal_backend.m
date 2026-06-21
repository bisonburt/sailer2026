/*
    metal_backend.m
    Metal compute backend for SAILER.
    Supports sphere, box, and cylinder top-level primitives with flat
    (pre-baked) surface colors. Compiled as Objective-C; linked with
    -framework Metal -framework Foundation.
*/

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metal_backend.h"

/* ------------------------------------------------------------------ */
/* Structs that mirror the MSL shader's buffer layouts.                */
/* ------------------------------------------------------------------ */

/* float4-aligned camera (matches MSL Camera struct) */
typedef struct {
    float vp[4];        /* xyz = viewpoint */
    float M[4];         /* xyz = M basis vector */
    float H[4];         /* xyz = H basis vector */
    float V[4];         /* xyz = V basis vector */
    int   width, height, maxdepth, _pad;  /* _pad carries nnodes */
} GPUCamera;

/* float4-aligned lighting */
typedef struct {
    float pos[4];       /* xyz = light world position */
    float color[4];     /* xyz = light rgb */
    float ambient[4];   /* xyz = ambient rgb, w = ambcoef */
} GPULight;

/* ------------------------------------------------------------------ */
/* Embedded MSL source                                                  */
/* ------------------------------------------------------------------ */

static const char *kShaderSrc =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"\n"
"struct Prim {\n"
"    float4 center;  /* xyz = center, w = radius (sphere) */\n"
"    float4 t0;      /* transform t[0..3] */\n"
"    float4 t1;      /* transform t[4..7] */\n"
"    float4 t2;      /* x=t[8], y=type, z=kdiff, w=kspec */\n"
"    float4 col;     /* xyz = color, w = highlight */\n"
"    float4 refl;    /* xyz = reflect tint * kspec */\n"
"};\n"
"struct Camera {\n"
"    float4 vp, M, H, V;\n"
"    int width, height, maxdepth, nnodes;\n"
"};\n"
"struct Light {\n"
"    float4 pos, color, ambient; /* ambient.w = ambcoef */\n"
"};\n"
"struct BVHNode {\n"
"    float4 lo, hi;       /* xyz = box min/max */\n"
"    int left, first, count, pad;\n"
"};\n"
"struct PrimHit { float t; float3 n; };  /* t<0 = miss; n is a normal LINE */\n"
"\n"
"/* v * M  (rows of M are t[0..2],t[3..5],t[6..8]) — CPU vec_t_mat */\n"
"static float3 vtmat(float3 v, float4 t0, float4 t1, float4 t2)\n"
"{\n"
"    return float3(dot(v, float3(t0.x, t0.y, t0.z)),\n"
"                  dot(v, float3(t0.w, t1.x, t1.y)),\n"
"                  dot(v, float3(t1.z, t1.w, t2.x)));\n"
"}\n"
"/* v * M^T — CPU vec_mat */\n"
"static float3 vmat(float3 v, float4 t0, float4 t1, float4 t2)\n"
"{\n"
"    return float3(v.x*t0.x + v.y*t0.w + v.z*t1.z,\n"
"                  v.x*t0.y + v.y*t1.x + v.z*t1.w,\n"
"                  v.x*t0.z + v.y*t1.y + v.z*t2.x);\n"
"}\n"
"static float max3(float3 v) { return max(max(v.x, v.y), v.z); }\n"
"\n"
"static PrimHit isect_sphere(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 oc = ro - p.center.xyz; float r = p.center.w;\n"
"    float a1 = dot(oc, rd);\n"
"    float a0 = dot(oc, oc) - r*r;\n"
"    if (a0 > 0.0 && a1 > 0.0) return h;\n"
"    float dis = a1*a1 - a0; if (dis < 1e-5) return h;\n"
"    float sq = sqrt(dis); float vi = -a1 - sq, vo = -a1 + sq;\n"
"    if (vo < 1e-4) return h;\n"
"    float t = (vi > 1e-4) ? vi : vo;\n"
"    h.t = t; h.n = normalize((ro + rd*t) - p.center.xyz);\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_box(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 sl = vtmat(ro - p.center.xyz, p.t0, p.t1, p.t2);\n"
"    float3 dl = vtmat(rd, p.t0, p.t1, p.t2);\n"
"    float st[6], di[6];\n"
"    st[0]=-sl.x-1.0; di[0]= dl.x; st[1]= sl.x-1.0; di[1]=-dl.x;\n"
"    st[2]=-sl.y-1.0; di[2]= dl.y; st[3]= sl.y-1.0; di[3]=-dl.y;\n"
"    st[4]=-sl.z-1.0; di[4]= dl.z; st[5]= sl.z-1.0; di[5]=-dl.z;\n"
"    float uen = -1e9, uex = 1e9; int pin = 0, pout = 0;\n"
"    for (int i = 0; i < 6; i++) {\n"
"        if (fabs(di[i]) < 0.001) { if (0.0 < st[i]) return h; }\n"
"        else {\n"
"            float u = st[i]/di[i];\n"
"            if (0.0 < di[i]) { if (uen < u) { uen = u; pin = i; } }\n"
"            else             { if (u < uex) { uex = u; pout = i; } }\n"
"        }\n"
"    }\n"
"    if (!(uen < uex && uex > 0.0)) return h;\n"
"    float u; int face;\n"
"    if (uen > 1e-4) { u = uen; face = pin; }\n"
"    else if (uex > 1e-4) { u = uex; face = pout; }\n"
"    else return h;\n"
"    int row = face / 2;\n"
"    float3 nrm = (row == 0) ? float3(p.t0.x, p.t0.y, p.t0.z)\n"
"               : (row == 1) ? float3(p.t0.w, p.t1.x, p.t1.y)\n"
"               :              float3(p.t1.z, p.t1.w, p.t2.x);\n"
"    h.t = u; h.n = normalize(nrm);\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_cyl(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 sl = vtmat(ro - p.center.xyz, p.t0, p.t1, p.t2);\n"
"    float3 dl = vtmat(rd, p.t0, p.t1, p.t2);\n"
"    float a0 = sl.x*sl.x + sl.z*sl.z - 1.0;\n"
"    float a1 = sl.x*dl.x + sl.z*dl.z;\n"
"    if (a0 > 0.0 && a1 > 0.0) return h;\n"
"    float a2 = dl.x*dl.x + dl.z*dl.z;\n"
"    float dis = a1*a1 - a2*a0;\n"
"    if (dis < 0.0) return h;\n"
"    dis = sqrt(dis);\n"
"    float cylu1 = (-a1 + dis)/a2;\n"
"    float cylu2 = (-a1 - dis)/a2;\n"
"    if (cylu1 < 0.0 && cylu2 < 0.0) return h;\n"
"    if (cylu2 < cylu1) { float tmp = cylu1; cylu1 = cylu2; cylu2 = tmp; }\n"
"    float capu1 = ( 1.0 - sl.y)/dl.y;\n"
"    float capu2 = (-1.0 - sl.y)/dl.y;\n"
"    if (capu2 < capu1) { float tmp = capu1; capu1 = capu2; capu2 = tmp; }\n"
"    float u1 = capu1; bool capin = true;\n"
"    if (capu1 < cylu1) { u1 = cylu1; capin = false; }\n"
"    float u2 = capu2; bool capout = true;\n"
"    if (capu2 > cylu2) { u2 = cylu2; capout = false; }\n"
"    if (u1 > u2) return h;\n"
"    if (cylu2 < capu1) return h;\n"
"    if (capu2 < cylu1) return h;\n"
"    float u; bool entry;\n"
"    if (u1 > 1e-4) { u = u1; entry = true; }\n"
"    else if (u2 > 1e-4) { u = u2; entry = false; }\n"
"    else return h;\n"
"    bool cap = entry ? capin : capout;\n"
"    float3 nrm;\n"
"    if (cap) {\n"
"        nrm = float3(p.t0.w, p.t1.x, p.t1.y);  /* axis dir = t[3..5] */\n"
"    } else {\n"
"        float3 nl = vtmat((ro + rd*u) - p.center.xyz, p.t0, p.t1, p.t2);\n"
"        nl.y = 0.0;\n"
"        nrm = vmat(nl, p.t0, p.t1, p.t2);\n"
"    }\n"
"    h.t = u; h.n = normalize(nrm);\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_ellipsoid(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 s = vtmat(ro - p.center.xyz, p.t0, p.t1, p.t2);\n"
"    float3 d = vtmat(rd, p.t0, p.t1, p.t2);\n"
"    float a2 = dot(d,d), a1 = dot(s,d), a0 = dot(s,s) - 1.0;\n"
"    float dis = a1*a1 - a2*a0;\n"
"    if (dis < 1e-6) return h;\n"
"    dis = sqrt(dis);\n"
"    float u1 = (-a1 + dis)/a2, u2 = (-a1 - dis)/a2;\n"
"    if (u1 < 1e-4 && u2 < 1e-4) return h;\n"
"    if (u2 < u1) { float t = u1; u1 = u2; u2 = t; }\n"
"    float u = (u1 > 1e-4) ? u1 : u2;\n"
"    float3 nl = s + u*d;\n"
"    h.t = u; h.n = normalize(vmat(nl, p.t0, p.t1, p.t2));\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_cone(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 s = vtmat(ro - p.center.xyz, p.t0, p.t1, p.t2);\n"
"    float3 d = vtmat(rd, p.t0, p.t1, p.t2);\n"
"    if ((s.y > -1e-5 && d.y > -1e-6) || (s.y < -0.99999 && d.y < 1e-6)) return h;\n"
"    float a2 = d.x*d.x - d.y*d.y + d.z*d.z;\n"
"    float a1 = s.x*d.x - s.y*d.y + s.z*d.z;\n"
"    float a0 = s.x*s.x - s.y*s.y + s.z*s.z;\n"
"    float dis = a1*a1 - a2*a0;\n"
"    if (dis < 0.0) return h;\n"
"    dis = sqrt(dis);\n"
"    float cylu1 = (-a1 + dis)/a2, cylu2 = (-a1 - dis)/a2;\n"
"    if (cylu1 < 1e-4 && cylu2 < 1e-4) return h;\n"
"    if (cylu2 < cylu1) { float t = cylu1; cylu1 = cylu2; cylu2 = t; }\n"
"    float capu1 = -s.y/d.y, capu2 = (-1.0 - s.y)/d.y;\n"
"    if (capu2 < capu1) { float t = capu1; capu1 = capu2; capu2 = t; }\n"
"    float u1 = capu1; bool capin = true;\n"
"    if (capu1 < cylu1) { u1 = cylu1; capin = false; }\n"
"    float u2 = capu2; bool capout = true;\n"
"    if (capu2 > cylu2) { u2 = cylu2; capout = false; }\n"
"    if (u1 > u2) return h;\n"
"    if (cylu2 < capu1) return h;\n"
"    if (capu2 < cylu1) return h;\n"
"    float u; bool entry;\n"
"    if (u1 > 1e-4) { u = u1; entry = true; }\n"
"    else if (u2 > 1e-4) { u = u2; entry = false; }\n"
"    else return h;\n"
"    bool cap = entry ? capin : capout;\n"
"    float3 nrm;\n"
"    if (cap) { nrm = float3(p.t0.w, p.t1.x, p.t1.y); }\n"
"    else {\n"
"        float3 nl = vtmat((ro + rd*u) - p.center.xyz, p.t0, p.t1, p.t2);\n"
"        nl.y = -nl.y;\n"
"        nrm = vmat(nl, p.t0, p.t1, p.t2);\n"
"    }\n"
"    h.t = u; h.n = normalize(nrm);\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_board(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,1,0);\n"
"    float by = p.t1.x;\n"
"    if (fabs(rd.y) < 1e-4) return h;\n"
"    float u = (by - ro.y)/rd.y;\n"
"    if (u < 0.0) return h;\n"
"    float hx = ro.x + u*rd.x, hz = ro.z + u*rd.z;\n"
"    if (hx > p.t0.x && hx < p.t0.y && hz > p.t0.z && hz < p.t0.w) { h.t = u; }\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect_tri(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    PrimHit h; h.t = -1.0; h.n = float3(0,0,1);\n"
"    float3 P0 = float3(p.t0.x, p.t0.y, p.t0.z);\n"
"    float3 P1 = float3(p.t0.w, p.t1.x, p.t1.y);\n"
"    float3 P2 = float3(p.t1.z, p.t1.w, p.t2.x);\n"
"    float3 N = cross(P1 - P0, P2 - P0);\n"
"    float den = dot(N, rd);\n"
"    if (fabs(den) < 1e-6) return h;\n"
"    float t = -(dot(N, ro) - dot(N, P0)) / den;\n"
"    if (t < 1e-4) return h;\n"
"    float3 P = ro + rd*t;\n"
"    if (dot(N, cross(P1 - P0, P - P0)) < 0.0) return h;\n"
"    if (dot(N, cross(P2 - P1, P - P1)) < 0.0) return h;\n"
"    if (dot(N, cross(P0 - P2, P - P2)) < 0.0) return h;\n"
"    h.t = t; h.n = normalize(N);\n"
"    return h;\n"
"}\n"
"\n"
"static PrimHit isect(Prim p, float3 ro, float3 rd)\n"
"{\n"
"    int type = (int)(p.t2.y + 0.5);\n"
"    if (type == 0) return isect_sphere(p, ro, rd);\n"
"    if (type == 1) return isect_box(p, ro, rd);\n"
"    if (type == 2) return isect_cyl(p, ro, rd);\n"
"    if (type == 3) return isect_cone(p, ro, rd);\n"
"    if (type == 4) return isect_ellipsoid(p, ro, rd);\n"
"    if (type == 5) return isect_board(p, ro, rd);\n"
"    return isect_tri(p, ro, rd);\n"
"}\n"
"\n"
"/* Branchless ray/AABB slab test (inv = 1/rd, guarded against 0). */\n"
"static bool slab(float3 lo, float3 hi, float3 ro, float3 inv)\n"
"{\n"
"    float3 t0 = (lo - ro) * inv;\n"
"    float3 t1 = (hi - ro) * inv;\n"
"    float3 tlo = min(t0, t1);\n"
"    float3 thi = max(t0, t1);\n"
"    float tmin = max(max(tlo.x, tlo.y), tlo.z);\n"
"    float tmax = min(min(thi.x, thi.y), thi.z);\n"
"    return tmax >= 0.0 && tmin <= tmax;\n"
"}\n"
"\n"
"struct ClosestRec { float t; int idx; float3 n; };\n"
"\n"
"static ClosestRec bvh_closest(device const Prim *prims, device const BVHNode *nodes,\n"
"                              float3 ro, float3 rd)\n"
"{\n"
"    ClosestRec h; h.t = 1e10; h.idx = -1; h.n = float3(0,0,1);\n"
"    float3 inv = select(1.0/rd, float3(1e30), fabs(rd) < 1e-8);\n"
"    int stack[64]; int sp = 0; stack[sp++] = 0;\n"
"    while (sp > 0) {\n"
"        BVHNode n = nodes[stack[--sp]];\n"
"        if (!slab(n.lo.xyz, n.hi.xyz, ro, inv)) continue;\n"
"        if (n.count > 0) {\n"
"            for (int i = 0; i < n.count; i++) {\n"
"                int si = n.first + i;\n"
"                PrimHit ph = isect(prims[si], ro, rd);\n"
"                if (ph.t > 0.0 && ph.t < h.t) { h.t = ph.t; h.idx = si; h.n = ph.n; }\n"
"            }\n"
"        } else {\n"
"            stack[sp++] = n.left;\n"
"            stack[sp++] = n.first;\n"
"        }\n"
"    }\n"
"    return h;\n"
"}\n"
"\n"
"static bool bvh_shadow(device const Prim *prims, device const BVHNode *nodes,\n"
"                       float3 ro, float3 rd, float maxd)\n"
"{\n"
"    float3 inv = select(1.0/rd, float3(1e30), fabs(rd) < 1e-8);\n"
"    int stack[64]; int sp = 0; stack[sp++] = 0;\n"
"    while (sp > 0) {\n"
"        BVHNode n = nodes[stack[--sp]];\n"
"        if (!slab(n.lo.xyz, n.hi.xyz, ro, inv)) continue;\n"
"        if (n.count > 0) {\n"
"            for (int i = 0; i < n.count; i++) {\n"
"                int si = n.first + i;\n"
"                PrimHit ph = isect(prims[si], ro, rd);\n"
"                if (ph.t > 0.001 && ph.t < maxd) return true;\n"
"            }\n"
"        } else {\n"
"            stack[sp++] = n.left;\n"
"            stack[sp++] = n.first;\n"
"        }\n"
"    }\n"
"    return false;\n"
"}\n"
"\n"
"kernel void ray_trace_spheres(\n"
"    device const Prim    *prims  [[buffer(0)]],\n"
"    constant uint        &nprim  [[buffer(1)]],\n"
"    device   uchar4      *pixels [[buffer(2)]],\n"
"    constant Camera      &cam    [[buffer(3)]],\n"
"    constant Light       &light  [[buffer(4)]],\n"
"    constant float3      &sky    [[buffer(5)]],\n"
"    device const BVHNode *nodes  [[buffer(6)]],\n"
"    uint2 gid [[thread_position_in_grid]])\n"
"{\n"
"    if ((int)gid.x >= cam.width || (int)gid.y >= cam.height) return;\n"
"\n"
"    float sx = (float)gid.x / (float)cam.width;\n"
"    float sy = (float)gid.y / (float)cam.height;\n"
"    float3 tgt = cam.M.xyz\n"
"               + (1.0 - 2.0*sx) * cam.H.xyz\n"
"               + (1.0 - 2.0*sy) * cam.V.xyz;\n"
"    float3 ro  = cam.vp.xyz;\n"
"    float3 rd  = normalize(tgt - ro);\n"
"\n"
"    float3 color  = float3(0.0);\n"
"    float3 weight = float3(1.0);\n"
"    float ambcoef = light.ambient.w;\n"
"    bool useBVH = (cam.nnodes > 0);\n"
"\n"
"    for (int depth = 0; depth <= cam.maxdepth && max3(weight) > 0.001; depth++) {\n"
"        ClosestRec hc;\n"
"        if (useBVH) hc = bvh_closest(prims, nodes, ro, rd);\n"
"        else {\n"
"            hc.t = 1e10; hc.idx = -1; hc.n = float3(0,0,1);\n"
"            for (uint i = 0; i < nprim; i++) {\n"
"                PrimHit ph = isect(prims[i], ro, rd);\n"
"                if (ph.t > 0.0 && ph.t < hc.t) { hc.t = ph.t; hc.idx = (int)i; hc.n = ph.n; }\n"
"            }\n"
"        }\n"
"        if (hc.idx < 0) { color += weight * sky; break; }\n"
"\n"
"        float3 hit = ro + rd * hc.t;\n"
"        float3 nor = hc.n;\n"
"        if (dot(rd, nor) > 0.0) nor = -nor;\n"
"        Prim pr = prims[hc.idx];\n"
"\n"
"        float3 litdir = light.pos.xyz - hit;\n"
"        float  ldist  = length(litdir); litdir /= ldist;\n"
"        float3 sorig  = hit + nor*0.001;\n"
"        bool   shadow;\n"
"        if (useBVH) shadow = bvh_shadow(prims, nodes, sorig, litdir, ldist);\n"
"        else {\n"
"            shadow = false;\n"
"            for (uint i = 0; i < nprim; i++) {\n"
"                PrimHit ph = isect(prims[i], sorig, litdir);\n"
"                if (ph.t > 0.001 && ph.t < ldist) { shadow = true; break; }\n"
"            }\n"
"        }\n"
"\n"
"        float3 sc = pr.col.xyz;\n"
"        float  kd = pr.t2.z;\n"
"        float  ks = pr.t2.w;\n"
"        float  hl = pr.col.w;\n"
"        float3 rgb;\n"
"        if (!shadow) {\n"
"            float dull = max(0.0, dot(nor, litdir));\n"
"            rgb = sc * (ambcoef + dull*(1.0-ambcoef));\n"
"            float3 flec = rd - 2.0*dot(rd,nor)*nor;\n"
"            float hdot = max(0.0, dot(litdir, flec));\n"
"            if (hl > 0.0 && hdot > 0.0) {\n"
"                float hc2 = pow(hdot, hl);\n"
"                rgb += hc2 * (light.color.xyz - rgb);\n"
"            }\n"
"        } else {\n"
"            rgb = sc * ambcoef;\n"
"        }\n"
"        rgb = clamp(rgb * kd, 0.0, 1.0);\n"
"        color += weight * rgb;\n"
"\n"
"        if (ks > 0.001 && depth < cam.maxdepth) {\n"
"            weight *= pr.refl.xyz;\n"
"            rd = normalize(rd - 2.0*dot(rd,nor)*nor);\n"
"            ro = hit + nor*0.001;\n"
"        } else break;\n"
"    }\n"
"\n"
"    color = clamp(color, 0.0, 1.0);\n"
"    pixels[gid.y * cam.width + gid.x] =\n"
"        uchar4((uchar)(color.r*255), (uchar)(color.g*255),\n"
"               (uchar)(color.b*255), 255);\n"
"}\n";

/* ------------------------------------------------------------------ */
/* Lazily-created global Metal objects                                  */
/* ------------------------------------------------------------------ */

static id<MTLDevice>              g_device   = nil;
static id<MTLCommandQueue>        g_queue    = nil;
static id<MTLComputePipelineState> g_pipe    = nil;

static int setup_metal(void)
{
    if (g_device != nil) return 1;   /* already initialised */

    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) return 0;

    g_queue = [g_device newCommandQueue];
    if (!g_queue) { g_device = nil; return 0; }

    NSError *err = nil;
    NSString *src = [NSString stringWithUTF8String:kShaderSrc];
    MTLCompileOptions *opts = [[MTLCompileOptions alloc] init];
    id<MTLLibrary> lib = [g_device newLibraryWithSource:src options:opts error:&err];
    if (!lib) {
        NSLog(@"Metal shader compile error: %@", err);
        g_device = nil; return 0;
    }

    id<MTLFunction> fn = [lib newFunctionWithName:@"ray_trace_spheres"];
    if (!fn) { g_device = nil; return 0; }

    g_pipe = [g_device newComputePipelineStateWithFunction:fn error:&err];
    if (!g_pipe) {
        NSLog(@"Metal pipeline error: %@", err);
        g_device = nil; return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int metal_available(void)
{
    return setup_metal();
}

int metal_render_spheres(
    const MetalPrim *prims, int nprims,
    const MetalBVHNode *nodes, int nnodes,
    float vp[3], float M[3], float H[3], float V[3],
    int width, int height,
    float light_pos[3], float light_rgb[3],
    float ambient_rgb[3], float ambcoef,
    float sky_rgb[3], int maxdepth,
    unsigned char *out_pixels)
{
    if (!setup_metal()) return -1;

    NSUInteger pixbytes = (NSUInteger)(width * height * 4);

    /* -- build GPU buffers -- */
    id<MTLBuffer> bPrims = [g_device
        newBufferWithBytes:prims
                   length:(NSUInteger)(nprims * sizeof(MetalPrim))
                  options:MTLResourceStorageModeShared];

    /* BVH nodes (at least one element so the buffer is never zero-length) */
    id<MTLBuffer> bNodes = [g_device
        newBufferWithBytes:(nnodes > 0 ? (const void *)nodes : (const void *)prims)
                   length:(NSUInteger)((nnodes > 0 ? nnodes : 1) * sizeof(MetalBVHNode))
                  options:MTLResourceStorageModeShared];

    uint32_t nprim32 = (uint32_t)nprims;
    id<MTLBuffer> bNprim = [g_device
        newBufferWithBytes:&nprim32
                   length:sizeof(uint32_t)
                  options:MTLResourceStorageModeShared];

    id<MTLBuffer> bPixels = [g_device
        newBufferWithLength:pixbytes
                   options:MTLResourceStorageModeShared];

    GPUCamera cam;
    cam.vp[0]=vp[0]; cam.vp[1]=vp[1]; cam.vp[2]=vp[2]; cam.vp[3]=0;
    cam.M[0]=M[0];   cam.M[1]=M[1];   cam.M[2]=M[2];   cam.M[3]=0;
    cam.H[0]=H[0];   cam.H[1]=H[1];   cam.H[2]=H[2];   cam.H[3]=0;
    cam.V[0]=V[0];   cam.V[1]=V[1];   cam.V[2]=V[2];   cam.V[3]=0;
    cam.width=width; cam.height=height; cam.maxdepth=maxdepth; cam._pad=nnodes;
    id<MTLBuffer> bCam = [g_device
        newBufferWithBytes:&cam length:sizeof(cam) options:MTLResourceStorageModeShared];

    GPULight light;
    light.pos[0]=light_pos[0]; light.pos[1]=light_pos[1]; light.pos[2]=light_pos[2]; light.pos[3]=0;
    light.color[0]=light_rgb[0]; light.color[1]=light_rgb[1]; light.color[2]=light_rgb[2]; light.color[3]=0;
    light.ambient[0]=ambient_rgb[0]; light.ambient[1]=ambient_rgb[1]; light.ambient[2]=ambient_rgb[2];
    light.ambient[3]=ambcoef;
    id<MTLBuffer> bLight = [g_device
        newBufferWithBytes:&light length:sizeof(light) options:MTLResourceStorageModeShared];

    float sky4[4] = { sky_rgb[0], sky_rgb[1], sky_rgb[2], 0.0f };
    id<MTLBuffer> bSky = [g_device
        newBufferWithBytes:sky4 length:sizeof(sky4) options:MTLResourceStorageModeShared];

    /* -- dispatch -- */
    id<MTLCommandBuffer> cmd = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    [enc setComputePipelineState:g_pipe];
    [enc setBuffer:bPrims  offset:0 atIndex:0];
    [enc setBuffer:bNprim  offset:0 atIndex:1];
    [enc setBuffer:bPixels offset:0 atIndex:2];
    [enc setBuffer:bCam    offset:0 atIndex:3];
    [enc setBuffer:bLight  offset:0 atIndex:4];
    [enc setBuffer:bSky    offset:0 atIndex:5];
    [enc setBuffer:bNodes  offset:0 atIndex:6];

    NSUInteger tgsize = g_pipe.maxTotalThreadsPerThreadgroup;
    if (tgsize > 256) tgsize = 256;
    NSUInteger tgw = 16, tgh = tgsize / tgw;
    MTLSize threadgroups = MTLSizeMake(
        ((NSUInteger)width  + tgw - 1) / tgw,
        ((NSUInteger)height + tgh - 1) / tgh,
        1);
    MTLSize tgdim = MTLSizeMake(tgw, tgh, 1);
    [enc dispatchThreadgroups:threadgroups threadsPerThreadgroup:tgdim];
    [enc endEncoding];
    [cmd commit];
    [cmd waitUntilCompleted];

    /* -- blit result -- */
    memcpy(out_pixels, [bPixels contents], pixbytes);
    return 0;
}
