/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | raymain.c
    Version:  | 2.0 SAIL, 2.0 SAILER
              |
    Date:     | 29 Mar 1993
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Ray tracer kernal
              |
              |
==============================================================================
*/


/* includes go here */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include "mathtype.h"
#include "structs.h"
#include "init.h"
#include "view.h"
#include "mathfun.h"
#include "image.h"
#include "bvh.h"
#include "sailpub.h"
#include "metal_backend.h"

/* protos for some sail functions */
void GetBackgroundInfo(double *,struct sail_module_struct **);
void GetLightingInfo(rgb_type *,double *,point_type *,rgb_type *);
void GetMaxDepth(int *);


static rgb_type ambient,lightcolor;
static double ambcoef,bkgrndsize;
static point_type lightsrc;
static struct sail_module_struct *bkgrndmodule;
/* The shared, read-only master scene; each render thread gets its own clone
   in the thread-local 'database' that trace() and the sect_*() funcs use. */
static prim_type *master_database;
static __thread prim_type *database;
static __thread bvh_t *thread_bvh;
static __thread prim_type *shadow_cache; /* last shadow-ray occluder */
static int maxdepth;
static int num_threads = 1;
static int use_gpu = 0;
static char outputfile[1024];
static int  outputquality = 90;

void trace(prim_type **,prim_type *,ray_type *,point_type *,point_type *,double);
void Illum(prim_type *,point_type,point_type,point_type,point_type,rgb_type *,int);
void raytrace(void);

/*
======================================
    ProcessImage()
    Load a JSON scene, render it, and write the result to outfile.
    The output format is chosen from the outfile extension by image.c.
    Returns 0 on success, non-zero on error.
======================================
*/
int ProcessImage(const char *infile, const char *outfile, int quality, int threads, int gpu)
{
    if (InitSail((char *)infile) == FALSE)
    {
        printf("Error reading scene file, aborting\n");
        return(1);
    }

    snprintf(outputfile, sizeof(outputfile), "%s", outfile);
    outputquality = quality;
    use_gpu = gpu;

    /* threads <= 0 means "auto": use all online cores */
    if (threads <= 0)
    {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        num_threads = (n > 0) ? (int)n : 1;
    }
    else
    {
        num_threads = threads;
    }

    master_database = GetObjectDataBase();
    GetBackgroundInfo(&bkgrndsize,&bkgrndmodule);
    GetLightingInfo(&ambient,&ambcoef,&lightsrc,&lightcolor);
    GetMaxDepth(&maxdepth);
    printf("Writing %s as output file\n",outputfile);
    raytrace();
    printf("done\n");
    return(0);
}



/*
======================================
    render_worker()
    Each worker renders an interleaved set of scanlines
    (row r handled by thread r % nthreads, for load balance).
    It owns a private clone of the scene database so the per-object
    intersection scratch in trace()/sect_*() is never shared.
======================================
*/
typedef struct
{
    int thread_id;
    int nthreads;
    int width;
    int height;
    point_type viewpoint;
    point_type lite;
} render_ctx;

static void *render_worker(void *arg)
{
    render_ctx *c = (render_ctx *)arg;
    ray_type ray;
    point_type hit,nor;
    prim_type *keyp;
    rgb_type rayrgb;
    int x,y;
    int cloned = (c->nthreads > 1);

    /* thread-local scene: clone for parallel runs, share for single-thread */
    database = cloned ? CloneObjectDatabase(master_database) : master_database;

    /* per-thread BVH over this thread's primitives (read-only, cheap to build) */
    thread_bvh = bvh_build(database);
    shadow_cache = NULL;

    ray.s = c->viewpoint;

    for (y = c->thread_id; y < c->height; y += c->nthreads)
    {
        for (x = 0; x < c->width; x++)
        {
            GetView(x,y,&ray.d);
            ray.d.x -= ray.s.x;
            ray.d.y -= ray.s.y;
            ray.d.z -= ray.s.z;
            normalize(&ray.d); /* normalize ray */
            trace(&keyp,NULL,&ray,&hit,&nor,0.0);
            Illum(keyp,ray.d,hit,nor,c->lite,&rayrgb,0);
            ImageSetPixel(x,y,rayrgb);
        }
    }

    bvh_free(thread_bvh);
    thread_bvh = NULL;
    if (cloned) /* release this thread's clone (shared modules untouched) */
    {
        prim_type *pp = database, *nx;
        while (pp != NULL) { nx = pp->next; free(pp); pp = nx; }
    }
    return NULL;
}

/*
======================================
    raytrace()
======================================
*/
void raytrace()
{
int Width,Height;
int n,t;
struct timeval t0,t1;
double render_ms;
pthread_t *tids;
render_ctx *ctx;

    Width = GetWidth();
    Height = GetHeight();

    if (ImageOpen(Width,Height) != 0)
        return;

    /* ----------------------------------------------------------------
       GPU (Metal) path — sphere/box/cylinder scenes on Apple Silicon.
       Primitive type is identified by comparing the intersect function
       pointer, so the older module code needs no changes. Scenes using
       any other primitive (cone, ellipsoid, CSG, board, …) fall back.
    ---------------------------------------------------------------- */
    extern void sect_sphere(prim_type *, ray_type *);
    extern void sect_box(prim_type *, ray_type *);
    extern void sect_cylinder(prim_type *, ray_type *);
    extern void sect_cone(prim_type *, ray_type *);
    extern void sect_ellipsoid(prim_type *, ray_type *);
    extern void sect_board(prim_type *, ray_type *);
    extern void sect_tri(prim_type *, ray_type *);
    if (use_gpu)
    {
        /* Check top-level prims; bail if any is an unsupported type.
           The GPU shader implements every primitive except CSG. */
        prim_type *pp;
        int all_ok = 1;
        for (pp = master_database; pp != NULL; pp = pp->next)
            if (pp->isInCSG == 0)
            {
                if (pp->sec_func != sect_sphere &&
                    pp->sec_func != sect_box &&
                    pp->sec_func != sect_cylinder &&
                    pp->sec_func != sect_cone &&
                    pp->sec_func != sect_ellipsoid &&
                    pp->sec_func != sect_board &&
                    pp->sec_func != sect_tri)
                { all_ok = 0; break; }
            }

        if (!all_ok)
            printf("GPU: scene uses CSG (or another unsupported primitive) "
                   "— falling back to CPU\n");
        else if (!metal_available())
            printf("GPU: Metal not available — falling back to CPU\n");
        else
        {
            /* Build a BVH over the prims and flatten it for the GPU. The
               primitive array is emitted in BVH leaf order so that each
               leaf's 'first' index lines up with the GPU prim buffer. */
            bvh_t *gb = bvh_build(master_database);
            int nnodes   = bvh_node_count(gb);
            int nbounded = bvh_bounded_count(gb);
            int si;

            if (bvh_unbounded_count(gb) != 0)
            {
                /* these primitives always have a finite bound */
                printf("GPU: unbounded primitive present — falling back to CPU\n");
                bvh_free(gb);
                goto cpu_path;
            }

            /* Serialize prims (in BVH order) and pre-bake flat surface color */
            MetalPrim *ms = (MetalPrim *) malloc((size_t)nbounded * sizeof(MetalPrim));
            for (si = 0; si < nbounded; si++)
            {
                prim_type *bp = bvh_bounded_prim(gb, si);
                input_type  inp;
                output_type out;
                int type, k;
                point_type c = {0,0,0};
                double tt[9];        /* 9 geometry slots packed into t0/t1/t2.x */
                memset(&inp, 0, sizeof(inp));
                memset(tt, 0, sizeof(tt));
                GetTexture(bp->inter.data[0].SurfAttrib, &inp, &out);
                ms[si].center[3] = 0.0f;

                if (bp->sec_func == sect_sphere)
                {
                    type = 0; c = bp->prim.sphere.c;
                    ms[si].center[3] = (float)bp->prim.sphere.r;
                }
                else if (bp->sec_func == sect_box)
                {
                    type = 1; c = bp->prim.box.c;
                    for (k = 0; k < 9; k++) tt[k] = bp->prim.box.t[k];
                }
                else if (bp->sec_func == sect_cylinder)
                {
                    type = 2; c = bp->prim.conic.c;
                    for (k = 0; k < 9; k++) tt[k] = bp->prim.conic.t[k];
                }
                else if (bp->sec_func == sect_cone)
                {
                    type = 3; c = bp->prim.conic.c;
                    for (k = 0; k < 9; k++) tt[k] = bp->prim.conic.t[k];
                }
                else if (bp->sec_func == sect_ellipsoid)
                {
                    type = 4; c = bp->prim.conic.c;
                    for (k = 0; k < 9; k++) tt[k] = bp->prim.conic.t[k];
                }
                else if (bp->sec_func == sect_board)
                {
                    type = 5;
                    tt[0]=bp->prim.board.l; tt[1]=bp->prim.board.r;
                    tt[2]=bp->prim.board.n; tt[3]=bp->prim.board.f;
                    tt[4]=bp->prim.board.y;
                }
                else /* sect_tri */
                {
                    type = 6;
                    tt[0]=bp->prim.tri.P[0].x; tt[1]=bp->prim.tri.P[0].y; tt[2]=bp->prim.tri.P[0].z;
                    tt[3]=bp->prim.tri.P[1].x; tt[4]=bp->prim.tri.P[1].y; tt[5]=bp->prim.tri.P[1].z;
                    tt[6]=bp->prim.tri.P[2].x; tt[7]=bp->prim.tri.P[2].y; tt[8]=bp->prim.tri.P[2].z;
                }

                ms[si].center[0] = (float)c.x;
                ms[si].center[1] = (float)c.y;
                ms[si].center[2] = (float)c.z;

                /* pack the 9 geometry slots across t0/t1/t2.x */
                ms[si].t0[0]=(float)tt[0]; ms[si].t0[1]=(float)tt[1]; ms[si].t0[2]=(float)tt[2]; ms[si].t0[3]=(float)tt[3];
                ms[si].t1[0]=(float)tt[4]; ms[si].t1[1]=(float)tt[5]; ms[si].t1[2]=(float)tt[6]; ms[si].t1[3]=(float)tt[7];
                ms[si].t2[0]=(float)tt[8];
                ms[si].t2[1] = (float)type;
                ms[si].t2[2] = (float)bp->inter.data[0].kdiff;
                ms[si].t2[3] = (float)bp->inter.data[0].kspec;
                ms[si].col[0] = (float)out.color.r;
                ms[si].col[1] = (float)out.color.g;
                ms[si].col[2] = (float)out.color.b;
                ms[si].col[3] = (float)out.highlight;

                /* per-channel reflectivity = attribute reflect tint * kspec,
                   matching the CPU's  rgb += reflected * reflect * kspec  */
                {
                    double ks = bp->inter.data[0].kspec;
                    ms[si].refl[0] = (float)(out.reflect.r * ks);
                    ms[si].refl[1] = (float)(out.reflect.g * ks);
                    ms[si].refl[2] = (float)(out.reflect.b * ks);
                    ms[si].refl[3] = 0.0f;
                }
            }

            /* Flatten BVH nodes for the GPU */
            MetalBVHNode *mn = (MetalBVHNode *) malloc((size_t)(nnodes > 0 ? nnodes : 1) * sizeof(MetalBVHNode));
            {
                int ni;
                for (ni = 0; ni < nnodes; ni++)
                {
                    float lo[3], hi[3];
                    bvh_get_node(gb, ni, lo, hi, &mn[ni].left, &mn[ni].first, &mn[ni].count);
                    mn[ni].lo[0]=lo[0]; mn[ni].lo[1]=lo[1]; mn[ni].lo[2]=lo[2]; mn[ni].lo[3]=0;
                    mn[ni].hi[0]=hi[0]; mn[ni].hi[1]=hi[1]; mn[ni].hi[2]=hi[2]; mn[ni].hi[3]=0;
                    mn[ni].pad = 0;
                }
            }

            /* Camera vectors */
            point_type M, H, V;
            GetCameraVectors(&M, &H, &V);
            float f_vp[3]  = {(float)(*GetViewpoint()).x,(float)(*GetViewpoint()).y,(float)(*GetViewpoint()).z};
            float f_M[3]   = {(float)M.x,(float)M.y,(float)M.z};
            float f_H[3]   = {(float)H.x,(float)H.y,(float)H.z};
            float f_V[3]   = {(float)V.x,(float)V.y,(float)V.z};
            float f_lpos[3]= {(float)lightsrc.x,(float)lightsrc.y,(float)lightsrc.z};
            float f_lcol[3]= {(float)lightcolor.r,(float)lightcolor.g,(float)lightcolor.b};
            float f_amb[3] = {(float)ambient.r,(float)ambient.g,(float)ambient.b};
            float f_ac     = (float)ambcoef;

            /* Background: sample one representative central sky ray, matching
               the CPU's p==NULL path (hitpoint = ray direction * bkgrndsize).
               The central pixel's target is M, so its direction is M - vp.
               The GPU uses this single flat sky color (procedural gradient
               backgrounds are approximated by their central value). */
            input_type  bki; output_type bko;
            point_type  vpt = *GetViewpoint();
            point_type  sdir;
            double      sdl;
            memset(&bki, 0, sizeof(bki));
            memset(&bko, 0, sizeof(bko));
            sdir.x = M.x - vpt.x; sdir.y = M.y - vpt.y; sdir.z = M.z - vpt.z;
            sdl = sqrt(sdir.x*sdir.x + sdir.y*sdir.y + sdir.z*sdir.z);
            if (sdl > 0.0) { sdir.x/=sdl; sdir.y/=sdl; sdir.z/=sdl; }
            bki.hitpoint.x = sdir.x * bkgrndsize;
            bki.hitpoint.y = sdir.y * bkgrndsize;
            bki.hitpoint.z = sdir.z * bkgrndsize;
            GetTexture(bkgrndmodule, &bki, &bko);
            float f_sky[3] = {(float)bko.color.r,(float)bko.color.g,(float)bko.color.b};

            unsigned char *gpupix = (unsigned char *) malloc((size_t)(Width*Height*4));

            printf("Rendering %s (%dx%d) on Metal GPU (%d prims, %d BVH nodes)...\n",
                   outputfile, Width, Height, nbounded, nnodes);
            gettimeofday(&t0, NULL);

            int gret = metal_render_spheres(
                ms, nbounded,
                mn, nnodes,
                f_vp, f_M, f_H, f_V,
                Width, Height,
                f_lpos, f_lcol, f_amb, f_ac,
                f_sky, maxdepth, gpupix);

            gettimeofday(&t1, NULL);
            render_ms = (t1.tv_sec-t0.tv_sec)*1000.0 + (t1.tv_usec-t0.tv_usec)/1000.0;

            if (gret == 0)
            {
                int x, y;
                for (y = 0; y < Height; y++)
                    for (x = 0; x < Width; x++)
                    {
                        rgb_type rgb;
                        int idx = (y * Width + x) * 4;
                        rgb.r = gpupix[idx]   / 255.0;
                        rgb.g = gpupix[idx+1] / 255.0;
                        rgb.b = gpupix[idx+2] / 255.0;
                        ImageSetPixel(x, y, rgb);
                    }
                free(gpupix); free(ms); free(mn); bvh_free(gb);
                ImageSave(outputfile, outputquality);
                ImageClose();
                printf("render time: %.1f ms  (%.0f primary rays/sec, Metal GPU)\n",
                       render_ms, (double)Width*Height / (render_ms/1000.0));
                return;
            }
            printf("GPU: dispatch failed — falling back to CPU\n");
            free(gpupix); free(ms); free(mn); bvh_free(gb);
        }
    }

cpu_path:
    n = num_threads;
    if (n < 1) n = 1;
    if (n > Height) n = Height; /* no point having idle threads */

    printf("Rendering %s (%dx%d) with %d thread%s...\n",
           outputfile,Width,Height,n,(n==1?"":"s"));

    /* time only the ray-tracing work, excluding image encode/write */
    gettimeofday(&t0,NULL);

    if (n == 1)
    {
        render_ctx c;
        c.thread_id = 0; c.nthreads = 1;
        c.width = Width; c.height = Height;
        c.viewpoint = *GetViewpoint(); c.lite = lightsrc;
        render_worker(&c);
    }
    else
    {
        tids = (pthread_t *) malloc(n * sizeof(pthread_t));
        ctx  = (render_ctx *) malloc(n * sizeof(render_ctx));
        for (t = 0; t < n; t++)
        {
            ctx[t].thread_id = t; ctx[t].nthreads = n;
            ctx[t].width = Width; ctx[t].height = Height;
            ctx[t].viewpoint = *GetViewpoint(); ctx[t].lite = lightsrc;
            pthread_create(&tids[t],NULL,render_worker,&ctx[t]);
        }
        for (t = 0; t < n; t++)
            pthread_join(tids[t],NULL);
        free(tids);
        free(ctx);
    }

    gettimeofday(&t1,NULL);
    render_ms = (t1.tv_sec - t0.tv_sec)*1000.0 + (t1.tv_usec - t0.tv_usec)/1000.0;
	ImageSave(outputfile,outputquality);
	ImageClose();
    printf("render time: %.1f ms  (%.0f primary rays/sec, %d threads)\n",
           render_ms, (double)Width*Height / (render_ms/1000.0), n);
    return;
}


/*
===============================================================
   Function: trace()
===============================================================
*/
/* per-ray state shared with the BVH visitor below */
typedef struct
{
    ray_type   *ray;
    prim_type  *skip;     /* object to ignore (NULL = none) */
    int         stest;    /* shadow test? */
    double      shadow;   /* shadow ray length */
    double      usec;     /* distance of closest hit so far */
    prim_type **p;        /* out: hit primitive (NULL if none) */
    point_type *hit;      /* out: hit point */
    point_type *nor;      /* out: surface normal */
} trace_ctx;

/* Test one candidate primitive. Returns 1 to stop traversal (shadow hit). */
static int trace_visit(prim_type *cur, void *vp)
{
    trace_ctx *c = (trace_ctx *)vp;
    double u;

    if (!((cur != c->skip || c->skip == NULL) && cur->isInCSG == 0x0))
        return 0;

    (*(cur->sec_func))(cur, c->ray);
    u = cur->inter.data[0].u;

    if (c->stest)
    {
        if (cur->hit > 0 && u < c->shadow) /* occluder between point and light */
        {
            c->usec = u;
            *c->p = cur;
            return 1; /* stop: one occluder is enough */
        }
    }
    else if (cur->hit > 0 && u < c->usec)  /* new closest hit */
    {
        c->usec = u;
        *c->p = cur;
        if (!cur->inter.data[0].nor_func)
        {
            *c->hit = cur->inter.data[0].hit;
            *c->nor = cur->inter.data[0].nor;
        }
    }
    return 0;
}

void trace(prim_type**p,prim_type *not,ray_type *ray,point_type *hit,point_type *nor,double shadow)
{
trace_ctx c;

    c.ray = ray;
    c.skip = not;
    c.stest = (shadow != 0.0);
    c.shadow = shadow;
    c.usec = 100000.0;
    c.p = p;
    c.hit = hit;
    c.nor = nor;

    *p = NULL;

    /* Shadow-ray coherence: the object that occluded the previous shadow ray
       is very likely to occlude this one too. Test it first and skip the full
       traversal on a hit (Haines/Greenberg shadow cache). */
    if (c.stest && not == NULL)
    {
        if (shadow_cache != NULL && trace_visit(shadow_cache, &c))
            return; /* cache hit: still in shadow, no traversal needed */
        c.skip = shadow_cache; /* already tested; don't retest in the scan */
    }

    /* Visit only primitives whose bounds the ray may cross (BVH), or fall
       back to a linear scan if no acceleration structure is present. */
    if (thread_bvh != NULL)
    {
        bvh_traverse(thread_bvh, ray, trace_visit, &c);
    }
    else
    {
        prim_type *cur;
        for (cur = database; cur != NULL; cur = cur->next)
            if (trace_visit(cur, &c)) break;
    }

    /* shadow rays: *p holds the occluder (or NULL); remember it and finish */
    if (c.stest)
    {
        if (not == NULL && *p != NULL) shadow_cache = *p;
        return;
    }

    if (c.usec > 99999.0)
    {
        *p = NULL;
        return;
    }

    /* finalize hit point / normal for the closest hit if it uses a nor_func */
    if ((*p)->inter.data[0].nor_func)
    {
        int hitnum = 0;
        double u = (*p)->inter.data[0].u;
        if ((*p)->hit > 0 && (*p)->hit < 3) hitnum = (*p)->hit - 1;
        hit->x = ray->s.x + u*ray->d.x;
        hit->y = ray->s.y + u*ray->d.y;
        hit->z = ray->s.z + u*ray->d.z;
        (*p)->inter.data[hitnum].hit = *hit;
        (*p)->inter.data[hitnum].nor_func(*p,hitnum);
        *nor = (*p)->inter.data[hitnum].nor;
        hit->x += 0.001*nor->x;
        hit->y += 0.001*nor->y;
        hit->z += 0.001*nor->z;
        (*p)->inter.data[hitnum].hit = *hit;
    }
    else
    {
        /* Primitives without a nor_func (board, triangle) set their hit point
           exactly on the surface. Apply the same shadow-bias offset the
           nor_func path uses, so the shadow/reflection ray doesn't instantly
           re-intersect the surface (self-shadowing flicker). */
        hit->x += 0.001*nor->x;
        hit->y += 0.001*nor->y;
        hit->z += 0.001*nor->z;
    }
}

/*
===============================================================
   Function: Illum()
===============================================================
*/
void Illum(prim_type *p,point_type hitv,point_type hitp,
    point_type nor,point_type lite,rgb_type *rgb,int level)
{
input_type  InputData;
output_type OutputData;
interD_type hit_data;
point_type  litdir;
point_type  dummy;
point_type  flec;
ray_type    lr;
double      dull,hlt,hcoef,hdot;
double      LightDist;
prim_type   *locp;


    /*
       If p == NULL then we did not hit any object. Since the ray hit
       nothing, this is a sky ray. We will call a background sail module
       to determine the surface attributes.
    */
    if(p == NULL)
    {
        /* surface properties ie: Color */
        InputData.hitpoint.x = hitv.x*bkgrndsize;
        InputData.hitpoint.y = hitv.y*bkgrndsize;
        InputData.hitpoint.z = hitv.z*bkgrndsize;
        GetTexture(bkgrndmodule,&InputData,&OutputData);
        *rgb = OutputData.color;
        return;
    }

    /*
       Calculate the direction for a light ray from the hit point to the
       light source.
    */
    litdir.x = lite.x - hitp.x;
    litdir.y = lite.y - hitp.y;
    litdir.z = lite.z - hitp.z;
    LightDist = sqrt(dot(litdir,litdir));
    normalize(&litdir);

    /*
        Now we call the ray tracer to see if there is an object between the
        hit point and the light source. If there is then the hit point is
        in the shadow of the light source.
    */

    lr.s = hitp; /* where ray originates */
    lr.d = litdir; /* the direction of the ray (towards the light source) */

    hit_data = p->inter.data[0];

    trace(&locp,NULL,&lr,&dummy,&dummy,LightDist); /* call the ray tracer */

    /* surface properties ie: Color */
    InputData.hitpoint = hitp;
    InputData.normal = nor;
    GetTexture(hit_data.SurfAttrib,&InputData,&OutputData);
    *rgb = OutputData.color;
    nor = OutputData.newnormal;
    hlt = OutputData.highlight;

    reflect(hitv,nor,&flec);

    /* if nothing is between the hit point and light source then no shadow */
    if(locp == NULL)
    {
        dull = dot(nor,litdir);
        if(dull < 0.0)
        {
            dull = 0.0;  /* lite does not reach hit point */
        }

        rgb->r *= (ambcoef + dull*(1.0-ambcoef));
        rgb->g *= (ambcoef + dull*(1.0-ambcoef));
        rgb->b *= (ambcoef + dull*(1.0-ambcoef));

        /* determine the highlight */
        if ((hdot = dot(litdir,flec)) > 0.0 && hlt > 0.0)
        {
            hcoef = pow(hdot,hlt);
            rgb->r += hcoef*(lightcolor.r-rgb->r);
            rgb->g += hcoef*(lightcolor.g-rgb->g);
            rgb->b += hcoef*(lightcolor.b-rgb->b);
        }
    }
    else   /* shadows */
    {
        rgb->r *= ambcoef;
        rgb->g *= ambcoef;
        rgb->b *= ambcoef;
    }

    /* weight diffuse color by specified coefficent (kdiff) */
    rgb->r *= hit_data.kdiff;
    rgb->g *= hit_data.kdiff;
    rgb->b *= hit_data.kdiff;

    /*
       If there is a specular coefficent, then we must calculate the
       reflected ray from our hit point, and return the color that this
       ray hits
    */

    if (hit_data.kspec > 0.001 && level++ < maxdepth)
    {
        ray_type lr;
        prim_type *lp;
        point_type h,n;
        rgb_type lrgb;

        lr.s = hitp;
        lr.d = flec;
        trace(&lp,NULL,&lr,&h,&n,0.0);
        Illum(lp,flec,h,n,lite,&lrgb,level);

        /*
            now that we have the color that the reflected ray hits, we must
            multiply this by the color of the specular value.
        */

        lrgb.r *= OutputData.reflect.r;
        lrgb.g *= OutputData.reflect.g;
        lrgb.b *= OutputData.reflect.b;

        rgb->r += lrgb.r*hit_data.kspec;
        rgb->g += lrgb.g*hit_data.kspec;
        rgb->b += lrgb.b*hit_data.kspec;
    }

    /* if rgb values are > 1.0, truncate (just in case). */
    if (rgb->r > 1.0) rgb->r = 1.0;
    if (rgb->g > 1.0) rgb->g = 1.0;
    if (rgb->b > 1.0) rgb->b = 1.0;
}
