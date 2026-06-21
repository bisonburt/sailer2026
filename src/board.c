/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | board.c
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
#include "board.h"
#include "init.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"



static name_token_type values[] =
{
    {"left",1},{"right",2},{"near",3},{"far",4},{"height",5},
    {"diff",6},{"refl",7},{"surface",8},{"",-1}
};


/*
===============================================================
   Function: InitBoard()
===============================================================
*/
void *InitBoard(double h,double l,double r, double n,double f,
    double kdiff, double kspec, void *surface)
{
prim_type *prim;

    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;

    prim->prim.board.l = l;
    prim->prim.board.r = r;
    prim->prim.board.n = n;
    prim->prim.board.f = f;
    prim->prim.board.y = h;

    prim->sec_func = sect_board;

    prim->inter.data[0].SurfAttrib = surface;
    prim->inter.data[1].SurfAttrib = surface;
    prim->inter.data[0].kdiff = kdiff;
    prim->inter.data[0].kspec = kspec;
    prim->inter.data[0].nor_func = NULL;
    prim->inter.data[1].nor_func = NULL;

    /* enclosing bounding sphere over the finite board rectangle (for the BVH) */
    {
        double cx = (l + r) / 2.0, cz = (n + f) / 2.0;
        double dx = r - cx, dz = f - cz;
        prim->boundinfo.c.x = cx;
        prim->boundinfo.c.y = h;
        prim->boundinfo.c.z = cz;
        prim->boundinfo.r = sqrt(dx*dx + dz*dz) + 1e-4;
    }
    prim->bound_func = NULL;

    return((void *)prim);
}

/*
===============================================================
   Function: sect_board()
===============================================================
*/
void sect_board(prim_type *p,ray_type *ray)
{
board_type brd = p->prim.board;

    p->hit = 0x0;

    if (fabs(ray->d.y) < 0.0001) return;

    if((p->inter.data[0].u = (brd.y - ray->s.y)/ray->d.y) < 0.0)
    {
        return;
    }

    p->inter.data[0].hit.x = ray->s.x + p->inter.data[0].u*ray->d.x;
    p->inter.data[0].hit.y = ray->s.y + p->inter.data[0].u*ray->d.y;
    p->inter.data[0].hit.z = ray->s.z + p->inter.data[0].u*ray->d.z;

    if ((p->inter.data[0].hit.x > brd.l) && (p->inter.data[0].hit.x < brd.r) &&
        (p->inter.data[0].hit.z > brd.n) && (p->inter.data[0].hit.z < brd.f))
    {
        /* hit board! */
        p->inter.data[0].nor.x = 0.0;
        p->inter.data[0].nor.y = 1.0;
        p->inter.data[0].nor.z = 0.0;
        p->hit= 0x1;
        return;
    }
    return; /* failed to hit board */
}

/*
======================================
    BuildBoard()
======================================
*/
void *BuildBoard(assmt_type *AssmtList)
{
assmt_type *t;
int token;
double left,right,nnear,ffar,height; /* near and far are keywords in c */
double diff,refl;
struct sail_module_struct *surfacemodule=NULL;
prim_type *prim;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* left */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, left"))
                left = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 2: /* right */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, right"))
                right = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 3: /* near */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, near"))
                nnear = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 4: /* far */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, far"))
                ffar = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* height */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, height"))
                height = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* diff */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, diff"))
                diff = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 7: /* refl */
                if (CheckValue(t,EXPRLST,DOUBLE,"board, refl"))
                refl = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 8: /* surface */
                if (t->rvalue.Type == MODULE)
                {
                    surfacemodule = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"board, surface"))
                {
                    surfacemodule =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            default:
                printf("Unknown lvalue (%s) found in a board module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }

    /* allocate a node */
    prim = AllocSectObject();

    prim->inter.next = NULL;

    prim->prim.board.l = left;
    prim->prim.board.r = right;
    prim->prim.board.n = nnear;
    prim->prim.board.f = ffar;
    prim->prim.board.y = height;

    prim->sec_func = sect_board;

    prim->inter.data[0].SurfAttrib = surfacemodule;
    prim->inter.data[1].SurfAttrib = surfacemodule;
    prim->inter.data[0].kdiff = diff;
    prim->inter.data[0].kspec = refl;
    prim->inter.data[0].nor_func = NULL;
    prim->inter.data[1].nor_func = NULL;

    /* enclosing bounding sphere over the finite board rectangle (for the BVH) */
    {
        double cx = (left + right) / 2.0, cz = (nnear + ffar) / 2.0;
        double dx = right - cx, dz = ffar - cz;
        prim->boundinfo.c.x = cx;
        prim->boundinfo.c.y = height;
        prim->boundinfo.c.z = cz;
        prim->boundinfo.r = sqrt(dx*dx + dz*dz) + 1e-4;
    }
    prim->bound_func = NULL;

    return((void *)prim);
}

