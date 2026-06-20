/*
    file: init.c
    for:  SAILER
    by:   David Fletcher
    on:   14 Jul 1992

    This file contains the code for initializing instances of our
    primitive objects, and other assorted initializing tasks
*/

/* includes */
#include <ctype.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"
#include "init.h"

#include "sailpub.h"

static prim_type *primlist = NULL;

/*
===============================================================
   Function: GetObjectDataBase()
===============================================================
*/
prim_type *GetObjectDataBase(void)
{
    return(primlist);
}

/*
===============================================================
   Function: AllocSectObject()
===============================================================
*/
prim_type * AllocSectObject(void)
{
prim_type *prim;

    prim = (prim_type *) calloc(sizeof(prim_type),1);
    if (prim == NULL)
    {
        printf("malloc filed in alocating a prim_type (InitCSG)\n");
        exit(2);
    }
    prim->next=primlist;
    primlist = prim;
    return(prim);
}

/*
===============================================================
   Function: CloneObjectDatabase()

   Produce an independent deep copy of the primitive database so that
   each render thread owns its own per-object intersection scratch
   (prim_type.inter / .hit), which trace() and the sect_*() functions
   write into. Read-only data is shared:
     - surface modules (interD_type.SurfAttrib) point to SAIL modules
       that are only read during shading, so they are NOT cloned.
     - intersect/bound/normal function pointers are copied as-is.
   Pointers that reference other primitives are remapped to the clones:
     - prim_type.next
     - CSG children (prim.CSG.left / .right)
     - interD_type.origin (owner back-pointer used by box/conic normals)
===============================================================
*/
extern void sect_CSG(prim_type *, ray_type *);

static prim_type *remap(prim_type *old, prim_type **olds, prim_type **news, int n)
{
    int i;
    if (old == NULL) return NULL;
    for (i = 0; i < n; i++)
        if (olds[i] == old) return news[i];
    return old; /* not in database (should not happen); leave unchanged */
}

prim_type *CloneObjectDatabase(prim_type *src)
{
    int n = 0, i;
    prim_type *p, **olds, **news;

    for (p = src; p != NULL; p = p->next) n++;
    if (n == 0) return NULL;

    olds = (prim_type **) malloc(n * sizeof(prim_type *));
    news = (prim_type **) malloc(n * sizeof(prim_type *));

    for (i = 0, p = src; p != NULL; p = p->next, i++)
    {
        olds[i] = p;
        news[i] = (prim_type *) malloc(sizeof(prim_type));
        *news[i] = *p; /* value copy: scratch, funcs, SurfAttrib, prim union */
    }

    for (i = 0; i < n; i++)
    {
        prim_type *np = news[i];
        np->next = remap(np->next, olds, news, n);
        np->inter.data[0].origin = remap(np->inter.data[0].origin, olds, news, n);
        np->inter.data[1].origin = remap(np->inter.data[1].origin, olds, news, n);
        if (np->sec_func == sect_CSG)
        {
            np->prim.CSG.left  = remap(np->prim.CSG.left,  olds, news, n);
            np->prim.CSG.right = remap(np->prim.CSG.right, olds, news, n);
        }
    }

    p = news[0]; /* src was olds[0], so its clone heads the list */
    free(olds);
    free(news);
    return p;
}

/*
===============================================================
   Function: InitRGB()
===============================================================
*/
void InitRGB(RGB,r,g,b)
rgb_type *RGB;
double r,g,b;
{
    RGB->r = r;
    RGB->g = g;
    RGB->b = b;
}


/*
===============================================================
   Function: InitPoint()
===============================================================
*/
void InitPoint(Point,x,y,z)
point_type *Point;
double x,y,z;
{
    Point->x = x;
    Point->y = y;
    Point->z = z;
}
