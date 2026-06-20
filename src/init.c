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
