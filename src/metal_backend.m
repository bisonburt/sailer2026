/*
    metal_backend.m
    Metal compute backend for SAILER (sphere-only scenes).
    Compiled as Objective-C; linked with -framework Metal -framework Foundation.
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
    int   width, height, maxdepth, _pad;
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
"struct Sphere {\n"
"    float4 pos_r;     /* xyz=center, w=radius */\n"
"    float4 col_kd;    /* xyz=color, w=kdiff   */\n"
"    float4 ks_hl;     /* x=kspec, y=highlight */\n"
"};\n"
"struct Camera {\n"
"    float4 vp, M, H, V;\n"
"    int width, height, maxdepth, _pad;\n"
"};\n"
"struct Light {\n"
"    float4 pos, color, ambient; /* ambient.w = ambcoef */\n"
"};\n"
"\n"
"/* Returns entry distance t, or -1 if miss. */\n"
"static float sphere_t(float3 ro, float3 rd, float3 c, float r)\n"
"{\n"
"    float3 oc = ro - c;\n"
"    float  a1 = dot(oc, rd);\n"
"    float  a0 = dot(oc, oc) - r * r;\n"
"    if (a0 > 0.0 && a1 > 0.0) return -1.0;\n"
"    float dis = a1*a1 - a0;\n"
"    if (dis < 1e-5) return -1.0;\n"
"    float sq = sqrt(dis);\n"
"    float vi = -a1 - sq, vo = -a1 + sq;\n"
"    if (vo < 1e-4) return -1.0;\n"
"    return (vi > 1e-4) ? vi : vo;\n"
"}\n"
"\n"
"kernel void ray_trace_spheres(\n"
"    device const Sphere *spheres [[buffer(0)]],\n"
"    constant uint        &nsph   [[buffer(1)]],\n"
"    device   uchar4      *pixels [[buffer(2)]],\n"
"    constant Camera      &cam    [[buffer(3)]],\n"
"    constant Light       &light  [[buffer(4)]],\n"
"    constant float3      &sky    [[buffer(5)]],\n"
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
"    float  weight = 1.0;\n"
"    float ambcoef = light.ambient.w;\n"
"\n"
"    for (int depth = 0; depth <= cam.maxdepth && weight > 0.001; depth++) {\n"
"        /* closest sphere */\n"
"        float best = 1e10;  int bi = -1;\n"
"        for (uint i = 0; i < nsph; i++) {\n"
"            float t = sphere_t(ro, rd, spheres[i].pos_r.xyz, spheres[i].pos_r.w);\n"
"            if (t > 0.0 && t < best) { best = t; bi = (int)i; }\n"
"        }\n"
"        if (bi < 0) { color += weight * sky; break; }\n"
"\n"
"        float3 hit = ro + rd * best;\n"
"        float3 nor = normalize(hit - spheres[bi].pos_r.xyz);\n"
"        if (dot(rd, nor) > 0.0) nor = -nor;\n"
"\n"
"        /* shadow */\n"
"        float3 litdir = light.pos.xyz - hit;\n"
"        float  ldist  = length(litdir); litdir /= ldist;\n"
"        bool   shadow = false;\n"
"        for (uint i = 0; i < nsph; i++) {\n"
"            float t = sphere_t(hit + nor*0.001, litdir,\n"
"                               spheres[i].pos_r.xyz, spheres[i].pos_r.w);\n"
"            if (t > 0.001 && t < ldist) { shadow = true; break; }\n"
"        }\n"
"\n"
"        float3 sc  = spheres[bi].col_kd.xyz;\n"
"        float  kd  = spheres[bi].col_kd.w;\n"
"        float  ks  = spheres[bi].ks_hl.x;\n"
"        float  hl  = spheres[bi].ks_hl.y;\n"
"        float3 rgb;\n"
"        if (!shadow) {\n"
"            float dull = max(0.0, dot(nor, litdir));\n"
"            rgb = sc * (ambcoef + dull*(1.0-ambcoef));\n"
"            float3 flec = rd - 2.0*dot(rd,nor)*nor;\n"
"            float hdot = max(0.0, dot(litdir, flec));\n"
"            if (hl > 0.0 && hdot > 0.0) {\n"
"                float hc = pow(hdot, hl);\n"
"                rgb += hc * (light.color.xyz - rgb);\n"
"            }\n"
"        } else {\n"
"            rgb = sc * ambcoef;\n"
"        }\n"
"        rgb = clamp(rgb * kd, 0.0, 1.0);\n"
"        color += weight * rgb;\n"
"\n"
"        if (ks > 0.001 && depth < cam.maxdepth) {\n"
"            weight *= ks;\n"
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
    const MetalSphere *spheres, int nspheres,
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
    id<MTLBuffer> bSpheres = [g_device
        newBufferWithBytes:spheres
                   length:(NSUInteger)(nspheres * sizeof(MetalSphere))
                  options:MTLResourceStorageModeShared];

    uint32_t nsph32 = (uint32_t)nspheres;
    id<MTLBuffer> bNsph = [g_device
        newBufferWithBytes:&nsph32
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
    cam.width=width; cam.height=height; cam.maxdepth=maxdepth; cam._pad=0;
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
    [enc setBuffer:bSpheres offset:0 atIndex:0];
    [enc setBuffer:bNsph    offset:0 atIndex:1];
    [enc setBuffer:bPixels  offset:0 atIndex:2];
    [enc setBuffer:bCam     offset:0 atIndex:3];
    [enc setBuffer:bLight   offset:0 atIndex:4];
    [enc setBuffer:bSky     offset:0 atIndex:5];

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
