/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | box.c
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
#include "bound.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"

static void box_nor(prim_type *p,int hitnum);
void sect_box(prim_type *p,ray_type *ray);



static name_token_type values[] =
{
    {"loc",1},{"axis",2},{"rot",3},{"diff",4},{"refl",5},
    {"surface",6},{"",-1}
};




/*
===============================================================
   Function: sect_box()
===============================================================
*/
void sect_box(prim_type *p,ray_type *ray)     /* intersect ray with box */
{
box_type *b ;
point_type s , d ;
int i;
int p_numI, p_numO;
double u ,
   uen = -1000000.0 ,
   uex =  1000000.0 ,
   st[6] , di[6] ;

    p->hit = 0x0;

    b = &p->prim.box;
    s.x = ray->s.x - b->c.x ;         /* translate startpoint */
    s.y = ray->s.y - b->c.y ;
    s.z = ray->s.z - b->c.z ;
    vec_t_mat(s,b->t,&s);             /* transform startpoint */
    vec_t_mat(ray->d,b->t,&d);        /* transform direction */

    st[0] = -s.x - 1 , di[0] =  d.x ,
    st[1] =  s.x - 1 , di[1] = -d.x ,
    st[2] = -s.y - 1 , di[2] =  d.y ,
    st[3] =  s.y - 1 , di[3] = -d.y ,
    st[4] = -s.z - 1 , di[4] =  d.z ,
    st[5] =  s.z - 1 , di[5] = -d.z ;

    for (i = 0; i < 6; i++)
    {
        if (fabs(di[i]) < 0.001)        /* parallel */
        {
            if (0.0 < st[i]) return;    /* ray passes outside box */
        }
        else                            /* not parallel */
        {
            u = st[i]/di[i];
            if (0.0 < di[i])            /* u is entry value */
            {
                if (uen < u)            /* new entry value */
                {
                    uen = u; p_numI = i;
                }
            }
            else                        /* u is exit value */
            {
                if (u < uex)
                {
                    uex = u; p_numO = i; /* new exit value */
                }
            }
        }
    }

    if (uen < uex && uex > 0.0)
    {
        p->inter.data[0].u = uen;
        p->inter.data[1].u = uex;
        b->nornum[0] = p_numI;
        b->nornum[1] = p_numO;
        if (uen > 0.0001) p->hit = 1;
        else if (uex > 0.0001) p->hit = 2;
        else p->hit = -1;
        return;             /* hit u-value */
    }
    else
    {
        return;
    }
}


static void box_nor(prim_type *p,int hitnum)
{
box_type *box = &p->inter.data[0].origin->prim.box;

    p->inter.data[0].nor = box->n[box->nornum[p->inter.data[0].orig_num]];
    if (p->inter.data[0].orig_num == 1)
    {
        p->inter.data[0].nor.x = -p->inter.data[0].nor.x;
        p->inter.data[0].nor.y = -p->inter.data[0].nor.y;
        p->inter.data[0].nor.z = -p->inter.data[0].nor.z;
    }
}


/*
======================================
    BuildBox()
======================================
*/
void *BuildBox(assmt_type *AssmtList)
{
assmt_type *t;
int token;

point_type loc,axis,rot;
double diff,refl;
prim_type *prim;
box_type *b;
double cx,sx,cy,sy,cz,sz;
int i;
struct sail_module_struct *surfacemodule=NULL;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* loc */
                if (CheckValue(t,EXPRLST,XYZ,"box, loc"))
                loc = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* axis */
                if (CheckValue(t,EXPRLST,XYZ,"box, axis"))
                axis = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* rot */
                if (CheckValue(t,EXPRLST,XYZ,"box, rot"))
                rot = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* diff */
                if (CheckValue(t,EXPRLST,DOUBLE,"box, diff"))
                diff = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* refl */
                if (CheckValue(t,EXPRLST,DOUBLE,"box, refl"))
                refl = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* surface */
                if (t->rvalue.Type == MODULE)
                {
                    surfacemodule = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"box, surface"))
                {
                    surfacemodule =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            default:
                printf("Unknown lvalue (%s) found in a box module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    /* tell raytracer about object */

    cx = cos(rot.x*DEG2RAD) ,
    sx = sin(rot.x*DEG2RAD) ,
    cy = cos(rot.y*DEG2RAD) ,
    sy = sin(rot.y*DEG2RAD) ,
    cz = cos(rot.z*DEG2RAD) ,
    sz = sin(rot.z*DEG2RAD) ;

    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;

    b =  &prim->prim.box;

    b->t[0] = cy*cz/axis.x;                  /* computation of T matrix */
    b->t[1] = cy*sz/axis.x;                  /*unit to arbitrary matrix */
    b->t[2] = sy/axis.x;                                /* inverse axes */
    b->t[3] = (-sx*sy*cz - cx*sz)/axis.y;
    b->t[4] = (-sx*sy*sz + cx*cz)/axis.y;
    b->t[5] = sx*cy/axis.y;
    b->t[6] = (-cx*sy*cz + sx*sz)/axis.z;
    b->t[7] = (-cx*sy*sz - sx*cz)/axis.z;
    b->t[8] = cx*cy/axis.z;

                             /* precomputation of six surface normals */
    b->n[0].x = -b->t[0]; b->n[0].y = -b->t[1]; b->n[0].z = -b->t[2];
    b->n[1].x =  b->t[0]; b->n[1].y =  b->t[1]; b->n[1].z =  b->t[2];
    b->n[2].x = -b->t[3]; b->n[2].y = -b->t[4]; b->n[2].z = -b->t[5];
    b->n[3].x =  b->t[3]; b->n[3].y =  b->t[4]; b->n[3].z =  b->t[5];
    b->n[4].x = -b->t[6]; b->n[4].y = -b->t[7]; b->n[4].z = -b->t[8];
    b->n[5].x =  b->t[6]; b->n[5].y =  b->t[7]; b->n[5].z =  b->t[8];

    for (i = 0; i < 6; i++)              /* normalize surface normals */
       normalize(&b->n[i]);

    b->c = loc;
    prim->sec_func = sect_box;
    prim->inter.data[0].SurfAttrib = surfacemodule;
    prim->inter.data[0].kdiff = diff;
    prim->inter.data[0].kspec = refl;
    prim->inter.data[1].SurfAttrib = surfacemodule;
    prim->inter.data[1].kdiff = diff;
    prim->inter.data[1].kspec = refl;
    prim->inter.data[0].nor_func = box_nor;
    prim->inter.data[1].nor_func = box_nor;
    prim->inter.data[0].orig_num = 0;
    prim->inter.data[1].orig_num = 1;
    prim->inter.data[0].origin = prim;
    prim->inter.data[1].origin = prim;

    /* create a bounding sphere for this object */
    prim->boundinfo.c = b->c;
    prim->boundinfo.r = vlen(&axis);
    prim->bound_func = sect_bound;

    return((void *)prim);
}


