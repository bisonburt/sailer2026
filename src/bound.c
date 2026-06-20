/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | bound.c
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
#include <stdlib.h>
#include <string.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"
#include "bound.h"
#include "init.h"


/*
===============================================================
   Function:sect_bound()
    just the sphere intersection code
===============================================================
*/
void sect_bound(prim_type *p,ray_type *ray)
{
sphere_type *sph = &p->boundinfo;
point_type EO;
double EO2,v,disc,d,vin,vout;

    p->hit = 0x0;

    EO.x = sph->c.x - ray->s.x;
    EO.y = sph->c.y - ray->s.y;
    EO.z = sph->c.z - ray->s.z;

    EO2 = dot(EO,EO);
    v = dot(EO,ray->d);

    disc = (sph->r*sph->r) - (EO2 - v*v);
    if (disc < 0.0001) return;

    d = sqrt(disc);
    vin = v-d;
    vout = v+d;

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

    p->inter.data[0].u = vin;
    p->inter.data[1].u = vout;

    if (p->inter.data[0].u > 0.0001) p->hit = 1;
    else if (p->inter.data[1].u > 0.0001) p->hit = 2;
    else p->hit = -1;

    return;
}
