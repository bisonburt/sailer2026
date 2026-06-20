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
#include "mathtype.h"
#include "structs.h"
#include "init.h"
#include "view.h"
#include "mathfun.h"
#include "image.h"
#include "sailpub.h"

/* global vars for stats */
static int G_NormalRays=0,G_ShadowRays=0,G_ShadowCacheHit=0;

/* protos for some sail functions */
void GetBackgroundInfo(double *,struct sail_module_struct **);
void GetLightingInfo(rgb_type *,double *,point_type *,rgb_type *);
void GetMaxDepth(int *);


static rgb_type ambient,lightcolor;
static double ambcoef,bkgrndsize;
static point_type lightsrc;
static struct sail_module_struct *bkgrndmodule;
static prim_type *database;
static int maxdepth;
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
int ProcessImage(const char *infile, const char *outfile, int quality)
{
    if (InitSail((char *)infile) == FALSE)
    {
        printf("Error reading scene file, aborting\n");
        return(1);
    }

    snprintf(outputfile, sizeof(outputfile), "%s", outfile);
    outputquality = quality;

    database = GetObjectDataBase();
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
    raytrace()
======================================
*/
void raytrace()
{
static prim_type *keyp;
static ray_type ray;
static point_type hit,nor,lite;
static rgb_type rayrgb;
static int x,y;
static int Width,Height;
time_t raytime;


    Width = GetWidth();
    Height = GetHeight();

    if (ImageOpen(Width,Height) != 0)
        return;

    lite = lightsrc;
    ray.s = *GetViewpoint();

    raytime = time(NULL);
    for (y=0; y < Height; y++)
    {
        printf("Rendering %s, line %d/%d\n",outputfile,y+1,Height);
        for (x=0; x < Width; x++)
        {
            GetView(x,y,&ray.d);
            ray.d.x -= ray.s.x;
            ray.d.y -= ray.s.y;
            ray.d.z -= ray.s.z;
            normalize(&ray.d); /* normalize ray */
            trace(&keyp,NULL,&ray,&hit,&nor,0.0);
            Illum(keyp,ray.d,hit,nor,lite,&rayrgb,0);
            ImagePutPixel(rayrgb);
        }
    }
    raytime -= time(NULL);
    raytime = -raytime;
	ImageSave(outputfile,outputquality);
	ImageClose();
    printf("time in seconds: %ld\n",raytime);
    printf("Normal Rays: %d\n",G_NormalRays);
    printf("Shadow Rays: %d\n",G_ShadowRays);
    printf("Shadow Cache Hits: %d\n",G_ShadowCacheHit);
    return;
}


/*
===============================================================
   Function: trace()
===============================================================
*/
void trace(prim_type**p,prim_type *not,ray_type *ray,point_type *hit,point_type *nor,double shadow)
{
double u,usec = 100000.0;
prim_type *cur;
int stest;

    if (shadow == 0.0)
        stest = 0;
    else
        stest = 1;

    for (cur = database; cur != NULL; cur = cur->next)
    {
        if((cur != not  || not == NULL) && cur->isInCSG == 0x0) /* don't intersect object not */
        {
            (*(cur->sec_func)) (cur,ray);
            u = cur->inter.data[0].u;
            if (stest)
            {
                G_ShadowRays++;
                if(cur->hit > 0 && (u < shadow))
                {
                    /*  we hit an object between the light source
                        and the surface
                    */
                    usec = u;
                    *p = cur;
                    return;
                }
            }
            else
            {
                G_NormalRays++;
                if((cur->hit > 0) && (u < usec))
                {
                    /* we hit an object */
                    usec = u;
                    *p = cur;
                    if (!cur->inter.data[0].nor_func)
                    {
                        *hit = cur->inter.data[0].hit;
                        *nor = cur->inter.data[0].nor;
                    }
                }
            }
        }
    }

    if (usec > 99999.0 || stest) *p = NULL;
    else
    {
        if ((*p)->inter.data[0].nor_func)
        {
            int hitnum=0;
            double u = (*p)->inter.data[0].u;
            if ((*p)->hit>0 && (*p)->hit<3) hitnum = (*p)->hit-1;
            hit->x = ray->s.x + u*ray->d.x;
            hit->y = ray->s.y + u*ray->d.y;
            hit->z = ray->s.z + u*ray->d.z;
            (*p)->inter.data[hitnum].hit = *hit;
            (*p)->inter.data[hitnum].nor_func(*p,hitnum);
             *nor = (*p)->inter.data[hitnum].nor;
            hit->x  += 0.001*nor->x;
            hit->y  += 0.001*nor->y;
            hit->z  += 0.001*nor->z;
            (*p)->inter.data[hitnum].hit = *hit;
        }
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
