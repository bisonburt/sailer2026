/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | conic.c
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
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"
#include "conic.h"
#include "init.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"


static void elip_nor(prim_type *,int);
static void cyl_nor(prim_type *,int);
static void cone_nor(prim_type *,int);

void *InitConic(point_type,point_type,point_type,int,double,double,void *b);

static name_token_type values[] =
{
    {"loc",1},{"axis",2},{"rot",3},{"diff",4},{"refl",5},
    {"surface",6},{"type",7},
    {"cylinder",8},{"ellipsoid",9},{"cone",10},{"",-1}
};



/*
===============================================================
   Function: InitConic()
===============================================================
*/
void *InitConic(point_type ax,point_type rot,point_type cent,
    int func,double kdiff,double kspec,void *surface)
{
prim_type *prim;
conic_type *c;
double cx = cos(rot.x*DEG2RAD) ,
       sx = sin(rot.x*DEG2RAD) ,
       cy = cos(rot.y*DEG2RAD) ,
       sy = sin(rot.y*DEG2RAD) ,
       cz = cos(rot.z*DEG2RAD) ,
       sz = sin(rot.z*DEG2RAD) ;


    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;

    c = &prim->prim.conic;

    c->t[0] = cy*cz/ax.x;                  /* computation of T matrix */
    c->t[1] = cy*sz/ax.x;                  /*unit to arbitrary matrix */
    c->t[2] = sy/ax.x;                                /* inverse axes */
    c->t[3] = (-sx*sy*cz - cx*sz)/ax.y;
    c->t[4] = (-sx*sy*sz + cx*cz)/ax.y;
    c->t[5] = sx*cy/ax.y;
    c->t[6] = (-cx*sy*cz + sx*sz)/ax.z;
    c->t[7] = (-cx*sy*sz - sx*cz)/ax.z;
    c->t[8] = cx*cy/ax.z;

    c->c = cent;

    prim->inter.data[0].nor_func = NULL;
    prim->inter.data[1].nor_func = NULL;

    switch(func)
    {
        case 1:
            prim->sec_func = sect_cylinder;
            prim->inter.data[0].nor_func = cyl_nor;
            prim->inter.data[1].nor_func = cyl_nor;
        break;
        case 2:
            prim->sec_func = sect_ellipsoid;
            prim->inter.data[0].nor_func = elip_nor;
            prim->inter.data[1].nor_func = elip_nor;
        break;
        case 3:
            prim->sec_func = sect_cone;
            prim->inter.data[0].nor_func = cone_nor;
            prim->inter.data[1].nor_func = cone_nor;
        break;
    }
    prim->inter.data[0].SurfAttrib = surface;
    prim->inter.data[1].SurfAttrib = surface;

    prim->inter.data[0].kdiff = kdiff;
    prim->inter.data[0].kspec = kspec;
    prim->inter.data[1].kdiff = kdiff;
    prim->inter.data[1].kspec = kspec;
    prim->inter.data[0].orig_num = 0;
    prim->inter.data[1].orig_num = 1;
    prim->inter.data[0].origin = prim;
    prim->inter.data[1].origin = prim;
    prim->bound_func = NULL;
    return((void *)prim);
}


/*
===============================================================
   Function: sect_ellipsoid()
===============================================================
*/
void sect_ellipsoid(prim_type *p,ray_type *ray)
{
    conic_type *c ;
    point_type s , d ;
    double u1 , u2 ,
        a2 , a1 , a0 , dis ;

    p->hit = 0x0;


    c = &p->prim.conic;
    s.x = ray->s.x - c->c.x ;                /* translate startpoint */
    s.y = ray->s.y - c->c.y ;
    s.z = ray->s.z - c->c.z ;

    vec_t_mat(s,c->t,&s);                    /* transform startpoint */
    vec_t_mat(ray->d,c->t,&d);               /* transform direction vector */
    a2 = d.x*d.x + d.y*d.y + d.z*d.z ;
    a1 = s.x*d.x + s.y*d.y + s.z*d.z ;
    a0 = s.x*s.x + s.y*s.y + s.z*s.z - 1 ;
    dis = a1*a1 - a2*a0 ;

    if (dis < 0.000001)    /* check for intersection with unit shpere */
        return;                             /* no intersection */

    dis = sqrt(dis);
    u1 = (-a1 + dis)/a2;
    u2 = (-a1 - dis)/a2;

    if (u1 < 0.0001 && u2 < 0.0001)         /* both intersections negative */
        return;                             /* object behind ray */

    if (u2 < u1)                            /* make u1 < u2 */
        dis = u1, u1 = u2, u2 = dis;

    p->inter.data[0].u = u1;
    p->inter.data[1].u = u2;

    c->s = s;
    c->d = d;

    if (p->inter.data[0].u > 0.0001) p->hit = 1;
    else if (p->inter.data[1].u > 0.0001) p->hit = 2;
    else p->hit = -1;
    return;
}


static void elip_nor(prim_type *p,int hitnum)
{
conic_type *con = &p->inter.data[0].origin->prim.conic;

    p->inter.data[0].nor.x = con->s.x + p->inter.data[0].u*con->d.x;
    p->inter.data[0].nor.y = con->s.y + p->inter.data[0].u*con->d.y;
    p->inter.data[0].nor.z = con->s.z + p->inter.data[0].u*con->d.z;

    vec_mat(p->inter.data[0].nor,con->t,&p->inter.data[0].nor);
    normalize(&p->inter.data[0].nor);

    if (p->inter.data[0].orig_num == 1)
    {
        p->inter.data[0].nor.x = -p->inter.data[0].nor.x;
        p->inter.data[0].nor.y = -p->inter.data[0].nor.y;
        p->inter.data[0].nor.z = -p->inter.data[0].nor.z;
    }
}


/*
===============================================================
   Function: sect_cylinder()
===============================================================
*/
void sect_cylinder(prim_type *p,ray_type *ray)
{
    conic_type c;
    point_type s,d;
    double capu1,capu2,cylu1,cylu2;
    double u1,u2,a2,a1,a0,dis;
    int capin,capout;
    int swaped_cap=FALSE,swaped_cyl=FALSE;



    p->hit = 0x0;

    c = p->prim.conic;
    s.x = ray->s.x - c.c.x;         /* translate startpoint */
    s.y = ray->s.y - c.c.y;
    s.z = ray->s.z - c.c.z;

    vec_t_mat(s,c.t,&s);            /* transform startpoint */
    vec_t_mat(ray->d,c.t,&d);       /* transform direction vector */

    a0 = s.x*s.x + s.z*s.z - 1;
    a1 = s.x*d.x + s.z*d.z;
    if (a0 > 0.0 && a1 > 0.0) return;    /* start outside, point away */
    a2 = d.x*d.x + d.z*d.z;

    dis = a1*a1 - a2*a0;

    if (dis < 0.0)         /* check for intersection with unit cyl */
        return;                 /* no intersection */

    dis = sqrt(dis);
    cylu1 = (-a1 + dis)/a2;
    cylu2 = (-a1 - dis)/a2;


    if (cylu1 < 0.0 && cylu2 < 0.0)   /* both intersections negative */
        return;                             /* object behind ray */

    if (cylu2 < cylu1)
    {
        dis = cylu1;
        cylu1 = cylu2;
        cylu2 = dis;
        swaped_cyl = TRUE;
    };

    capu1 = (1-s.y)/d.y ;        /* top */
    capu2 = (-1-s.y)/d.y ;       /*  bottom */

    if (capu2 < capu1)
    {
        dis = capu1;
        capu1 = capu2;
        capu2 = dis;
        swaped_cap = TRUE;
    }

    /* find largest in */
    u1 = capu1;
    capin = TRUE;
    if (capu1 < cylu1)
    {
        u1 = cylu1;
        capin = FALSE;
    }

    u2 = capu2;
    capout = TRUE;
    /* find smallest out */
    if (capu2 > cylu2)
    {
        u2 = cylu2;
        capout = FALSE;
    }

    if (u1>u2) return;

    if (cylu2 < capu1) return;
    if (capu2 < cylu1) return;

    p->inter.data[0].u = u1;
    p->inter.data[1].u = u2;

    p->prim.conic.capin = capin;
    p->prim.conic.capout = capout;
    p->prim.conic.swaped_cap = swaped_cap;

    if (p->inter.data[0].u > 0.0001) p->hit = 1;
    else if (p->inter.data[1].u > 0.0001) p->hit = 2;
    else p->hit = -1;
    return;
}


static void cyl_nor(prim_type *p,int hitnum)
{
conic_type *con = &p->inter.data[hitnum].origin->prim.conic;
point_type nor;

    if (p->inter.data[hitnum].orig_num == 0)
    {
        if (con->capin)
        {
            if (con->swaped_cap)
            {
                /* bottom */
                nor.x = -con->t[3];
                nor.y = -con->t[4];
                nor.z = -con->t[5];
            }
            else
            {
                /* top */
                nor.x = con->t[3];
                nor.y = con->t[4];
                nor.z = con->t[5];
            }
        }
        else
        {
            nor.x = (p->inter.data[hitnum].hit.x - con->c.x);
            nor.y = (p->inter.data[hitnum].hit.y - con->c.y);
            nor.z = (p->inter.data[hitnum].hit.z - con->c.z);
            vec_t_mat(nor,con->t,&nor);
            nor.y = 0.0;
            vec_mat(nor,con->t,&nor);
        }
    }
    else
    {
        if (con->capout)
        {
            if (con->swaped_cap)
            {
                /* top */
                nor.x = -con->t[3];
                nor.y = -con->t[4];
                nor.z = -con->t[5];
            }
            else
            {
                /* bottom */
                nor.x = con->t[3];
                nor.y = con->t[4];
                nor.z = con->t[5];
            }
        }
        else
        {
            nor.x = -(p->inter.data[hitnum].hit.x - con->c.x);
            nor.y = -(p->inter.data[hitnum].hit.y - con->c.y);
            nor.z = -(p->inter.data[hitnum].hit.z - con->c.z);
            vec_t_mat(nor,con->t,&nor);
            nor.y = 0.0;
            vec_mat(nor,con->t,&nor);
        }
    }
    normalize(&nor);
    p->inter.data[hitnum].nor = nor;
}


/*
===============================================================
   Function: sect_cone()
===============================================================
*/
void sect_cone(prim_type *p,ray_type *ray)
{
conic_type c;
point_type s,d;
double u1,u2,a2,a1,a0,dis;
int capin,capout;
int swaped_cap=FALSE;
double cylu1,cylu2,capu1,capu2;

    p->hit = 0x0;

    c = p->prim.conic;
    s.x = ray->s.x - c.c.x ;
    s.y = ray->s.y - c.c.y ;
    s.z = ray->s.z - c.c.z ;
    vec_t_mat(s,c.t,&s);                /* transform startpoint */
    vec_t_mat(ray->d,c.t,&d);           /* transform direction vector */

    if (   s.y > -0.00001 && d.y > -0.000001        /* outside planes */
            || s.y < -0.99999 && d.y <  0.000001)   /* pointing away  */
        return;

    a2 = d.x*d.x - d.y*d.y + d.z*d.z ,
    a1 = s.x*d.x - s.y*d.y + s.z*d.z ,
    a0 = s.x*s.x - s.y*s.y + s.z*s.z ,
    dis = a1*a1 - a2*a0 ;

    if (dis < 0.0)          /* check for intersection with unit cone */
        return;              /* no intersection */

    dis = sqrt(dis);
    cylu1 = (-a1 + dis)/a2;
    cylu2 = (-a1 - dis)/a2;

    if (cylu1 < 0.0001 && cylu2 < 0.0001)   /* both intersections negative */
        return;                             /* object behind ray */

    if (cylu2 < cylu1)    /* make u1 < u2 */
    {
        dis  = cylu1;
        cylu1 = cylu2;
        cylu2 = dis;
    }

    capu1 = -s.y/d.y ;             /* top */
    capu2 = (-1-s.y)/d.y ;       /*  bottom */

    if (capu2 < capu1)
    {
        dis = capu1;
        capu1 = capu2;
        capu2 = dis;
        swaped_cap = TRUE;
    }

    /* find largest in */
    u1 = capu1;
    capin = TRUE;
    if (capu1 < cylu1)
    {
        u1 = cylu1;
        capin = FALSE;
    }

    u2 = capu2;
    capout = TRUE;
    /* find smallest out */
    if (capu2 > cylu2)
    {
        u2 = cylu2;
        capout = FALSE;
    }

    if (u1>u2) return;

    if (cylu2 < capu1) return;
    if (capu2 < cylu1) return;

    p->inter.data[0].u = u1;
    p->inter.data[1].u = u2;

    p->prim.conic.capin = capin;
    p->prim.conic.capout = capout;
    p->prim.conic.swaped_cap = swaped_cap;

    if (p->inter.data[0].u > 0.0001) p->hit = 1;
    else if (p->inter.data[1].u > 0.0001) p->hit = 2;
    else p->hit = -1;
    return;
}


static void cone_nor(prim_type *p,int hitnum)
{
conic_type *con = &p->inter.data[hitnum].origin->prim.conic;
point_type nor;

    if (p->inter.data[hitnum].orig_num == 0)
    {
        if (con->capin)
        {
            if (con->swaped_cap)
            {
                /* bottom */
                nor.x = -con->t[3];
                nor.y = -con->t[4];
                nor.z = -con->t[5];
            }
            else
            {
                /* top */
                nor.x = con->t[3];
                nor.y = con->t[4];
                nor.z = con->t[5];
            }
        }
        else
        {
            nor.x = (p->inter.data[hitnum].hit.x - con->c.x);
            nor.y = (p->inter.data[hitnum].hit.y - con->c.y);
            nor.z = (p->inter.data[hitnum].hit.z - con->c.z);
            vec_t_mat(nor,con->t,&nor);
            nor.y = -nor.y;
            vec_mat(nor,con->t,&nor);
        }
    }
    else
    {
        if (con->capout)
        {
            if (con->swaped_cap)
            {
                /* top */
                nor.x = -con->t[3];
                nor.y = -con->t[4];
                nor.z = -con->t[5];
            }
            else
            {
                /* bottom */
                nor.x = con->t[3];
                nor.y = con->t[4];
                nor.z = con->t[5];
            }
        }
        else
        {
            nor.x = (p->inter.data[hitnum].hit.x - con->c.x);
            nor.y = (p->inter.data[hitnum].hit.y - con->c.y);
            nor.z = (p->inter.data[hitnum].hit.z - con->c.z);
            vec_t_mat(nor,con->t,&nor);
            nor.y = -nor.y;
            vec_mat(nor,con->t,&nor);
            nor.x = -nor.x;
            nor.y = -nor.y;
            nor.z = -nor.z;
        }
    }
    normalize(&nor);
    p->inter.data[hitnum].nor = nor;
}


/*
======================================
    BuildConic()
======================================
*/
void *BuildConic(assmt_type *AssmtList)
{
assmt_type *t;
int token;

point_type loc,axis,rot;
double diff,refl;
int funct;

struct sail_module_struct *surfacemodule=NULL;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* loc */
                if (CheckValue(t,EXPRLST,XYZ,"conic, loc"))
                loc = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* axis */
                if (CheckValue(t,EXPRLST,XYZ,"conic, axis"))
                axis = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* rot */
                if (CheckValue(t,EXPRLST,XYZ,"conic, rot"))
                rot = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* diff */
                if (CheckValue(t,EXPRLST,DOUBLE,"conic, diff"))
                diff = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* refl */
                if (CheckValue(t,EXPRLST,DOUBLE,"conic, refl"))
                refl = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* surface */
                if (t->rvalue.Type == MODULE)
                {
                    surfacemodule = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"conic, surface"))
                {
                    surfacemodule =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            case 7: /* type */
                if (CheckValue(t,EXPRLST,CHARPTR,"conic, type"))
                {
                    switch(GetToken(t->rvalue.data.ExprLst->u.CharPtrVal,values))
                    {
                        case 8: /* cylinder */
                            funct = 1; /* sect_cylinder */
                        break;
                        case 9: /* ellipsoid */
                            funct = 2; /* sect_elipsiod */
                        break;
                        case 10: /* cone */
                            funct = 3; /* sect_cone */
                        break;
                        default:
                            printf("Error in conic, bad flag for type in flag assignment\n");
                            SAIL_ERROR_FLAG = TRUE;
                        break;
                    }
                }
            break;
            default:
                printf("Unknown lvalue (%s) found in a check module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    /* tell raytracer about object */
    return(InitConic(axis,rot,loc,funct,diff,refl,(void *)surfacemodule));
}
