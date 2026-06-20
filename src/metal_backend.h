/*
    metal_backend.h
    GPU ray-tracing path via Metal compute shaders (Apple Silicon / macOS).

    Scope: top-level spheres only. If the scene has non-sphere top-level
    primitives (CSG, conic, box, board) the caller should fall back to the
    CPU path. CSG sub-tree members (isInCSG != 0) are always excluded.

    Call metal_available() first; if it returns 0, Metal is not present and
    all other calls are no-ops.
*/

#ifndef METAL_BACKEND_H
#define METAL_BACKEND_H

/* Per-sphere GPU record (layout must match the MSL shader's Sphere struct).
   All floats; arranged in three float4s for 16-byte aligned GPU access. */
typedef struct {
    float cx, cy, cz, radius;       /* world-space center + radius */
    float r, g, b, kdiff;           /* surface color (0-1) + diffuse coeff */
    float kspec, highlight, pad0, pad1; /* specular coeff, highlight exponent */
} MetalSphere;

/* Returns 1 if a Metal device is available, 0 otherwise. */
int metal_available(void);

/*
    Render the sphere list on the GPU.
    On success writes RGBA8 pixels (width * height * 4 bytes) to 'out_pixels'
    and returns 0.  Returns -1 if Metal setup or dispatch failed; the caller
    should retry on the CPU.

    vp / M / H / V   camera vectors (from GetCameraVectors / GetViewpoint)
    light_pos         world-space light position
    light_rgb         light color (0-1 each channel)
    ambient_rgb       ambient color (0-1 each channel)
    ambcoef           ambient coefficient
    sky_rgb           background color for rays that hit nothing
    maxdepth          max reflection depth
    out_pixels        caller-allocated RGBA8 buffer, width*height*4 bytes
*/
int metal_render_spheres(
    const MetalSphere *spheres, int nspheres,
    float vp[3], float M[3], float H[3], float V[3],
    int width, int height,
    float light_pos[3], float light_rgb[3],
    float ambient_rgb[3], float ambcoef,
    float sky_rgb[3], int maxdepth,
    unsigned char *out_pixels
);

#endif /* METAL_BACKEND_H */
