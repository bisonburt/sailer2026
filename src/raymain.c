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
static int maxdepth;
static int num_threads = 1;
static char buff[128];
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
int ProcessImage(const char *infile, const char *outfile, int quality, int threads)
{
    if (InitSail((char *)infile) == FALSE)
    {
        printf("Error reading scene file, aborting\n");
        return(1);
    }

    snprintf(outputfile, sizeof(outputfile), "%s", outfile);
    outputquality = quality;

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

    /* shadow rays: *p already holds the occluder (or NULL); done */
    if (c.stest) return;

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

    /* debug */
    if (dot(hitv,nor) > 0.0)
    {
         printf("N[%d]",level);
         rgb->r=1.0;
         rgb->g=0.0;
         rgb->b=0.0;
         return;
    }

    reflect(hitv,nor,&flec);

    if (dot(nor,flec) < 0.0)
        printf("W");
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
