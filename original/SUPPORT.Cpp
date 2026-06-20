/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | support.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | A grab bag of support routines for sail.
              |
              |
==============================================================================
*/

#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "mathtype.h"
#include "structs.h"
#include "support.h"


/* defines here */
#define MAXN 16
static int NoiseTab[MAXN+1][MAXN+1][MAXN+1][3];

/* Some 3d random number generators form graphics gems II (p. 615) */
#define  rand3a(x,y,z)	fr and(67*(x)+59*(y)+71*(z))
#define  rand3b(x,y,z)	frand(73*(x)+79*(y)+83*(z))
#define  rand3c(x,y,z)	frand(89*(x)+97*(y)+101*(z))
#define  rand3d(x,y,z)	frand(103*(x)+107*(y)+109*(z))


/*
===============================================================
   Function: stralloc(), used to allocate a string
===============================================================
*/
char *stralloc(char *S)
{
    char *ret;

    ret = (char *) malloc(strlen(S)+1);
    strcpy(ret,S);
    return(ret);
}


/*
===============================================================
   Function: normalize(), normalize a vector
===============================================================
*/
double normalize(point_type *p)
{
double l;

    l = sqrt((p->x*p->x) + (p->y*p->y) + (p->z*p->z));

    if (l > 0.0001)
    {
        p->x /= l;
        p->y /= l;
        p->z /= l;
    }
    return(l);
}

/*
===============================================================
   Function: dot(), compute the the dot product
===============================================================
*/
double dot(point_type a,point_type b)
{
    return(a.x*b.x + a.y*b.y + a.z*b.z);
}


/*
===============================================================
    Function: InitRotMat()
    Note: Code adapted from CSc 456 ray tracer (Dr. Pokorny)
    IMPORTANT: Before the vector is tranformed by the resulting
    matrix that this function generates, you must translate your
    object to the origin. The matrix created only rotates points.
    Scaling must be done after rotation.
================================================================
*/
void InitRotMat(point_type *rot, matrix3_type mat)
{
  double cx = cos(rot->x*DEG2RAD) ,
         sx = sin(rot->x*DEG2RAD) ,
         cy = cos(rot->y*DEG2RAD) ,
         sy = sin(rot->y*DEG2RAD) ,
         cz = cos(rot->z*DEG2RAD) ,
         sz = sin(rot->z*DEG2RAD) ;

    mat[0] = cy*cz;
    mat[1] = cy*sz;
    mat[2] = sy;
    mat[3] = (-sx*sy*cz - cx*sz);
    mat[4] = (-sx*sy*sz + cx*cz);
    mat[5] = sx*cy;
    mat[6] = (-cx*sy*cz + sx*sz);
    mat[7] = (-cx*sy*sz - sx*cz);
    mat[8] = cx*cy;
}


/*
===============================================================
   Function: vect_mat()
===============================================================
*/
void vect_mat(point_type v,matrix3_type m,point_type *r)
                       /* vector v times matrix m */
{
   r->x = v.x*m[0] + v.y*m[3] + v.z*m[6] ,
   r->y = v.x*m[1] + v.y*m[4] + v.z*m[7] ,
   r->z = v.x*m[2] + v.y*m[5] + v.z*m[8] ;
}

/*
===============================================================
   Function: vect_t_mat()
===============================================================
*/
void vect_t_mat(point_type v,matrix3_type m,point_type *r)
          /* vector v times transposed matrix m */
{
   r->x = v.x*m[0] + v.y*m[1] + v.z*m[2] ,
   r->y = v.x*m[3] + v.y*m[4] + v.z*m[5] ,
   r->z = v.x*m[6] + v.y*m[7] + v.z*m[8] ;
}


/*
===============================================================
   Function: frac()
===============================================================
*/
double frac(double r)
{
    return(r - floor(r));
}

/*
===============================================================
   Function: frand()
===============================================================
*/
double frand(int s)			/* get random number from seed */
{
	s = s<<13 ^ s;
	return(1.0-((s*(s*s*15731+789221)+1376312589)&0x7fffffff)/1073741824.0);
}


/*
===============================================================
   Function: InitNoise()
===============================================================
*/
void InitNoise()
{
int x,y,z,xx,yy,zz;
int i;
    for (i=0;i<3;i++)
        for(x=0;x<=MAXN;x++)
            for(y=0;y<=MAXN;y++)
                for(z=0;z<=MAXN;z++)
                {
                    NoiseTab[x][y][z][i] = (rand() % 10000);
                    if (x == MAXN) xx = 0; else xx = x;
                    if (y == MAXN) yy = 0; else yy = y;
                    if (z == MAXN) zz = 0; else zz = z;
                    NoiseTab[x][y][z][i] = NoiseTab[xx][yy][zz][i];
                }
}


/*
===============================================================
   Function: FNoise()
===============================================================
*/

double FNoise (double x,double y,double z,int i)
{
int j;
double scale = 2.0;
double sum;

    x /= 2.0;
    y /= 2.0;
    z /= 2.0;

    sum = 0.0;
    for (j=0;j<10;j++)
    {
        double temp;
        temp = Noise(x*2.0*scale,y*2.0*scale,z*2.0*scale,i);
        sum = sum + (temp/scale);
        scale *= 2.0;
    }
    return(sum);
}

/*
===============================================================
   Function: Noise()
===============================================================
*/

double Noise(double x,double y,double z,int i)
{
int ix,iy,iz;
point_type o;
int n;
double n00,n01,n10,n11,n0,n1;
double t;

    x += 150000.0;
    y += 150000.0;
    z += 150000.0;

    ix = (int)floor(x) % MAXN;
    iy = (int)floor(y) % MAXN;
    iz = (int)floor(z) % MAXN;

    o.x = frac(x);
    o.y = frac(y);
    o.z = frac(z);

    n = NoiseTab[ix][iy][iz][i];
    n00 = n + o.x*(double)(NoiseTab[ix+1][iy][iz][i]-n);
    n = NoiseTab[ix][iy][iz+1][i];
    n01 = n + o.x*(double)(NoiseTab[ix+1][iy][iz+1][i]-n);
    n = NoiseTab[ix][iy+1][iz][i];
    n10 = n + o.x*(double)(NoiseTab[ix+1][iy+1][iz][i]-n);
    n = NoiseTab[ix][iy+1][iz+1][i];
    n11 = n + o.x*(double)(NoiseTab[ix+1][iy+1][iz+1][i]-n);

    n0 = n00 + o.y*(n10-n00);
    n1 = n01 + o.y*(n11-n01);
    t =(n0+o.z*(n1-n0))*0.0001;
    return(t);
}

