/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | sphere.c
    Version:  | 2.0 SAIL, 2.0 SAILER
              |
    Date:     | 20 Jul 1993
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | implements sphere related routines
              |
              |
==============================================================================
*/


/* includes go here */
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"
#include "init.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"

void sect_sphere(prim_type *,ray_type *);
static void shpere_nor(prim_type *p,int hitnum);
extern void *InitSphere(point_type,double,double,double,void *);

static name_token_type values[] =
{
    {"loc",1},{"size",2},{"diff",3},{"refl",4},{"surface",5},{"",-1}
};


/*
===============================================================
   Function: InitSphere()
===============================================================
*/
void *InitSphere(point_type center,double rad,double diff,double refl,void *surface)
{
prim_type *prim;

    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;
    prim->prim.sphere.c.x = center.x;
    prim->prim.sphere.c.y = center.y;
    prim->prim.sphere.c.z = center.z;

    prim->prim.sphere.r = rad;
    prim->inter.data[0].kdiff = diff;
    prim->inter.data[1].kdiff = diff;
    prim->inter.data[0].kspec = refl;
    prim->inter.data[1].kspec = refl;
    prim->sec_func = sect_sphere;
    prim->inter.data[0].SurfAttrib = surface;
    prim->inter.data[1].SurfAttrib = surface;
    prim->inter.data[0].nor_func = shpere_nor;
    prim->inter.data[1].nor_func = shpere_nor;
    prim->inter.data[0].orig_num = 0;
    prim->inter.data[1].orig_num = 1;
    prim->inter.data[0].origin = prim;
    prim->inter.data[1].origin = prim;

    /* a sphere is its own bounding sphere (used by the BVH) */
    prim->boundinfo.c = center;
    prim->boundinfo.r = rad;
    prim->bound_func = NULL;
    return((void *)prim);
}


/*
===========================================================
   Function: sect_sphere()
    this one works when ray starts at the spheres surface
===============================================================
*/
void sect_sphere(prim_type *p,ray_type *ray)
{
sphere_type *sph = &p->prim.sphere;
double a1,a0,dis,vin,vout;
point_type t = ray->s;
    p->hit = 0x0;

    t.x = ray->s.x - sph->c.x;              /* translate startpoint */
    t.y = ray->s.y - sph->c.y;
    t.z = ray->s.z - sph->c.z;

    a0 = dot(t,t) - sph->r*sph->r;
    a1 = dot(t,ray->d);
    if (a0 > 0.0 && a1 > 0.0) return;       /* start outside, point away */

    dis = a1*a1 - a0 ;
    if (dis < 0.00001) return;              /* check intersection */

    dis = sqrt(dis);
    vin = -a1 - dis;
    vout = -a1 + dis;
    if (vout < 0.0001) return;              /* sphere behind ray */

#if 0
    p->inter.data[0].hit.x = ray->s.x + vin*ray->d.x;
    p->inter.data[0].hit.y = ray->s.y + vin*ray->d.y;
    p->inter.data[0].hit.z = ray->s.z + vin*ray->d.z;

    p->inter.data[0].nor.x = (p->inter.data[0].hit.x - sph->c.x)/sph->r;
    p->inter.data[0].nor.y = (p->inter.data[0].hit.y - sph->c.y)/sph->r;
    p->inter.data[0].nor.z = (p->inter.data[0].hit.z - sph->c.z)/sph->r;

    p->inter.data[1].hit.x = ray->s.x + vout*ray->d.x;
    p->inter.data[1].hit.y = ray->s.y + vout*ray->d.y;
    p->inter.data[1].hit.z = ray->s.z + vout*ray->d.z;

    p->inter.data[1].nor.x = (p->inter.data[1].hit.x - sph->c.x)/sph->r;
    p->inter.data[1].nor.y = (p->inter.data[1].hit.y - sph->c.y)/sph->r;
    p->inter.data[1].nor.z = (p->inter.data[1].hit.z - sph->c.z)/sph->r;
#endif

    p->inter.data[0].u = vin;
    p->inter.data[1].u = vout;

    if (p->inter.data[0].u > 0.0001)
        p->hit = 1;
    else if (p->inter.data[1].u > 0.0001)
        p->hit = 2;
    else
        p->hit = -1;

    return;
}


static void shpere_nor(prim_type *p,int hitnum)
{
sphere_type *sph = &p->inter.data[0].origin->prim.sphere;

    p->inter.data[0].nor.x = (p->inter.data[0].hit.x - sph->c.x)/sph->r;
    p->inter.data[0].nor.y = (p->inter.data[0].hit.y - sph->c.y)/sph->r;
    p->inter.data[0].nor.z = (p->inter.data[0].hit.z - sph->c.z)/sph->r;

    /* if this instersection point originated as a out point */
    /* we need to flip the normal */
    if (p->inter.data[0].orig_num == 1)
    {
        p->inter.data[0].nor.x = -p->inter.data[0].nor.x;
        p->inter.data[0].nor.y = -p->inter.data[0].nor.y;
        p->inter.data[0].nor.z = -p->inter.data[0].nor.z;
    }
}

/*
======================================
    BuildSphere()
======================================
*/
void *BuildSphere(assmt_type *AssmtList)
{
assmt_type *t;
int token;

point_type loc;
double size,diff,refl;
struct sail_module_struct *surfacemodule=NULL;;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* loc */
                if (CheckValue(t,EXPRLST,XYZ,"sphere, loc"))
                loc = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* size */
                if (CheckValue(t,EXPRLST,DOUBLE,"sphere, size"))
                size = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 3: /* diff */
                if (CheckValue(t,EXPRLST,DOUBLE,"sphere, diff"))
                diff = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 4: /* refl */
                if (CheckValue(t,EXPRLST,DOUBLE,"sphere, refl"))
                refl = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* surface */
                if (t->rvalue.Type == MODULE)
                {
                    surfacemodule = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"sphere, surface"))
                {
                    surfacemodule =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            default:
                printf("Unknown lvalue (%s) found in a check module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    /* tell raytracer about object */
    return(InitSphere(loc,size,diff,refl,(void *)surfacemodule));
}



