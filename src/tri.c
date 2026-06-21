/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | tri.c
    Version:  | 2.0 SAIL, 2.0 SAILER
              |
    Date:     | 20 Jul 1993
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
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"
#include "tri.h"
#include "init.h"
#include "bound.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"

void *InitTri(point_type *,double,double,void *);

static name_token_type values[] =
{
    {"pt1",1},{"pt2",2},{"pt3",3},{"diff",4},{"refl",5},{"surface",6},{"",-1}
};


/*
===============================================================
   Function: InitTri()
===============================================================
*/
void *InitTri(point_type *vert,double diff,double refl,void *surface)
{
tri_type *t;
prim_type *prim;

    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;

    t = &prim->prim.tri;

    t->P[0] = vert[0];
    t->P[1] = vert[1];
    t->P[2] = vert[2];

    t->N.x = (t->P[1].y - t->P[0].y)*(t->P[2].z - t->P[0].z)-
             (t->P[2].y - t->P[0].y)*(t->P[1].z - t->P[0].z);
    t->N.y = (t->P[1].z - t->P[0].z)*(t->P[2].x - t->P[0].x)-
             (t->P[2].z - t->P[0].z)*(t->P[1].x - t->P[0].x);
    t->N.z = (t->P[1].x - t->P[0].x)*(t->P[2].y - t->P[0].y)-
             (t->P[2].x - t->P[0].x)*(t->P[1].y - t->P[0].y);

    normalize(&t->N);

    t->d = -(t->N.x*t->P[0].x + t->N.y*t->P[0].y + t->N.z*t->P[0].z);

    t->i0 = ((fabs(t->N.x)>fabs(t->N.y))?((fabs(t->N.x)>fabs(t->N.z))?0:2):
            ((fabs(t->N.y)>fabs(t->N.z))?1:2));

    prim->sec_func = sect_tri;
    prim->inter.data[0].nor_func = NULL;
    prim->inter.data[1].nor_func = NULL;

    prim->inter.data[0].SurfAttrib = surface;
    prim->inter.data[1].SurfAttrib = surface;

    prim->inter.data[0].kdiff = diff;
    prim->inter.data[0].kspec = refl;

#if 0
    /* create a bounding sphere for this object */
    prim->boundinfo.c.x = (vert[0].x+vert[1].x+vert[2].x)/3.0;
    prim->boundinfo.c.y = (vert[0].y+vert[1].y+vert[2].y)/3.0;
    prim->boundinfo.c.z = (vert[0].z+vert[1].z+vert[2].z)/3.0;

    if ((tmp[0] = dist3d(prim->boundinfo.c,vert[0]))>
        (tmp[1] = dist3d(prim->boundinfo.c,vert[1])))
    {
        prim->boundinfo.r = tmp[0];
        if ((tmp[2] = dist3d(prim->boundinfo.c,vert[2]))>tmp[0])
        {
            prim->boundinfo.r = tmp[2];
        }
    }
    else
    {
        prim->boundinfo.r = tmp[1];
        if ((tmp[2] = dist3d(prim->boundinfo.c,vert[2]))>tmp[1])
        {
            prim->boundinfo.r = tmp[2];
        }
    }
    prim->bound_func = sect_bound;
#endif

    return((void *)prim);
}



/*
===============================================================
   Function:sect_tri()
   adapted from "Graphics Gems", Academic Press, 1990
   by Didier Badouel
===============================================================
*/
void sect_tri(prim_type *p,ray_type *ray)
{
tri_type *tri = &p->prim.tri;
double t;
double num,den;
double u0,v0,u1,v1,u2,v2,alpha,beta,inter;

    p->hit = 0x0;

    den = dot(tri->N,ray->d);
    if (fabs(den) < 0.00001) return; /* plane is parallel to ray */
    num = tri->d + dot(tri->N,ray->s);
    t = -num/den;
    if (t < 0.0) return;  /* plane is behind the origin of the ray */

    p->inter.data[0].hit.x = ray->s.x + ray->d.x*t;
    p->inter.data[0].hit.y = ray->s.y + ray->d.y*t;
    p->inter.data[0].hit.z = ray->s.z + ray->d.z*t;

    switch (tri->i0)
    {
        case 0:
            u0 = p->inter.data[0].hit.y - tri->P[0].y;
            v0 = p->inter.data[0].hit.z - tri->P[0].z;
            u1 = tri->P[1].y - tri->P[0].y;
            v1 = tri->P[1].z - tri->P[0].z;
            u2 = tri->P[2].y - tri->P[0].y;
            v2 = tri->P[2].z - tri->P[0].z;
        break;
        case 1:
            u0 = p->inter.data[0].hit.x - tri->P[0].x;
            v0 = p->inter.data[0].hit.z - tri->P[0].z;
            u1 = tri->P[1].x - tri->P[0].x;
            v1 = tri->P[1].z - tri->P[0].z;
            u2 = tri->P[2].x - tri->P[0].x;
            v2 = tri->P[2].z - tri->P[0].z;
        break;
        case 2:
            u0 = p->inter.data[0].hit.x - tri->P[0].x;
            v0 = p->inter.data[0].hit.y - tri->P[0].y;
            u1 = tri->P[1].x - tri->P[0].x;
            v1 = tri->P[1].y - tri->P[0].y;
            u2 = tri->P[2].x - tri->P[0].x;
            v2 = tri->P[2].y - tri->P[0].y;
        break;
    }

    inter = 0;

    if (u1 == 0.0)
    {
        beta = u0/u2;
        if ((beta >= 0.0)&&(beta <= 1.0))
        {
            alpha = (v0 - beta*v2)/v1;
            inter = ((alpha >= 0.0)&&((alpha+beta) <= 1.0));
        }
    }
    else
    {
        beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
        if ((beta >= 0.0)&&(beta <= 1.0))
        {
            alpha = (u0 - beta*u2)/u1;
            inter = ((alpha >= 0.0)&&((alpha+beta) <= 1.0));
        }
    }

    if (inter)
    {
        p->inter.data[0].nor.x = tri->N.x;
        p->inter.data[0].nor.y = tri->N.y;
        p->inter.data[0].nor.z = tri->N.z;

        p->inter.data[0].u = t;
        p->hit= 0x1;
        return;
    }
    else
    {
        /* The ray crossed the triangle's plane but landed outside the
           triangle itself: this is a clean miss. (The original code here
           resurrected a stale p->inter.data[0].u from a previous ray that
           did hit, reporting a false hit and smearing the triangle's color
           across its whole infinite plane.) */
        p->hit = 0x0;
        return;
    }
}


/*
======================================
    BuildSphere()
======================================
*/
void *BuildTriangle(assmt_type *AssmtList)
{
assmt_type *t;
int token;

point_type points[3];
double diff,refl;
struct sail_module_struct *surfacemodule;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* pt1 */
                if (CheckValue(t,EXPRLST,XYZ,"sphere, pt1"))
                points[0] = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* pt2 */
                if (CheckValue(t,EXPRLST,XYZ,"sphere, pt2"))
                points[1] = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* pt3 */
                if (CheckValue(t,EXPRLST,XYZ,"sphere, pt3"))
                points[2] = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* diff */
                if (CheckValue(t,EXPRLST,DOUBLE,"sphere, diff"))
                diff = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* refl */
                if (CheckValue(t,EXPRLST,DOUBLE,"sphere, refl"))
                refl = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* surface */
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
				assert("Unknown lvalue (%s) found in a check module\n");
//                printf("Unknown lvalue (%s) found in a check module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    /* tell raytracer about object */
    return(InitTri(points,diff,refl,(void *)surfacemodule));
}


