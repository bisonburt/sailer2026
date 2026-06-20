/*
    file: mathfun.c
    for:  ray tracer
    by:   David Fletcher
    on:   april 17, 1991
*/

/* includes go here */
#include <math.h>
#include <stdlib.h>
#include "mathtype.h"
#include "structs.h"
#include "mathfun.h"

/*
===============================================================
    Function: RotScale()
    Note: Code adapted from CSc 456 ray tracer (Dr. Pokorny)
    IMPORTANT: Before the vector is tranformed by the resulting
    matrix that this function generates, you must translate your
    object to the origin. The matrix created only rotates and scales
    points.
================================================================
*/
void RotScale(rot,axis,mat)
point_type *rot,*axis;
double mat[9];
{
  double cx = cos(rot->x*DEG2RAD) ,
         sx = sin(rot->x*DEG2RAD) ,
         cy = cos(rot->y*DEG2RAD) ,
         sy = sin(rot->y*DEG2RAD) ,
         cz = cos(rot->z*DEG2RAD) ,
         sz = sin(rot->z*DEG2RAD) ;


    mat[0] = cy*cz/axis->x;
    mat[1] = cy*sz/axis->x;
    mat[2] = sy/axis->x;
    mat[3] = (-sx*sy*cz - cx*sz)/axis->y;
    mat[4] = (-sx*sy*sz + cx*cz)/axis->y;
    mat[5] = sx*cy/axis->y;
    mat[6] = (-cx*sy*cz + sx*sz)/axis->z;
    mat[7] = (-cx*sy*sz - sx*cz)/axis->z;
    mat[8] = cx*cy/axis->z;

/*
    mat[0] = cy*cz;
    mat[1] = cy*sz;
    mat[2] = sy;
    mat[3] = (-sx*sy*cz - cx*sz);
    mat[4] = (-sx*sy*sz + cx*cz);
    mat[5] = sx*cy;
    mat[6] = (-cx*sy*cz + sx*sz);
    mat[7] = (-cx*sy*sz - sx*cz);
    mat[8] = cx*cy;
*/

}


/*
===============================================================
   Function: vec_mat()
===============================================================
*/
void vec_mat(v,m,r)                       /* vector v times matrix m */
   point_type v  ;
   double m[9]    ;                                     /* 3x3 matrix */
   point_type *r ;                                  /* result vector */
{
   r->x = v.x*m[0] + v.y*m[3] + v.z*m[6] ,
   r->y = v.x*m[1] + v.y*m[4] + v.z*m[7] ,
   r->z = v.x*m[2] + v.y*m[5] + v.z*m[8] ;
}

/*
===============================================================
   Function: vec_t_mat()
===============================================================
*/
void vec_t_mat(v,m,r)          /* vector v times transposed matrix m */
   point_type v  ;
   double m[9]    ;                                     /* 3x3 matrix */
   point_type *r ;                                  /* result vector */
{
   r->x = v.x*m[0] + v.y*m[1] + v.z*m[2] ,
   r->y = v.x*m[3] + v.y*m[4] + v.z*m[5] ,
   r->z = v.x*m[6] + v.y*m[7] + v.z*m[8] ;
}

/*
===============================================================
   Function: mat_mat()
===============================================================
*/

void mat_mat(
   double a[3][3] ,
   double b[3][3] ,
   double c[3][3] )
{
   int i , j , k ;
   double s[3][3] ;

   for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
      {
	 s[i][j] = 0;
	 for (k = 0; k < 3; k++)
	    s[i][j] += a[i][k]*b[k][j];
      }

   for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
         c[i][j] = s[i][j];
}


/*
===============================================================
   Function: reflect()
===============================================================
*/
void reflect(hit,nor,ref)                        /* reflection vector */
   point_type  hit ,                   /* impinging vector facing in */
               nor ,                    /* surface normal facing out */
              *ref ;                  /* reflected vector facing out */
{
   double tcp = -2.0*dot(hit,nor) ;             /* two times cosine phi */

   ref->x = tcp*nor.x + hit.x;
   ref->y = tcp*nor.y + hit.y;
   ref->z = tcp*nor.z + hit.z;
}

/*
===============================================================
   Function: vcross()
===============================================================
*/
void vcross(a,b,c)
point_type *a,*b,*c;
{
c->x = (a->y*b->z) - (a->z*b->y);
c->y = (a->z*b->x) - (a->x*b->z);
c->z = (a->x*b->y) - (a->y*b->x);
}

/*
===============================================================
   Function: vscale()
===============================================================
*/
void vscale(f,v,V)
double f;
point_type *v,*V;
{
    V->x = f*v->x;
    V->y = f*v->y;
    V->z = f*v->z;
}

/*
===============================================================
   Function: vlen()
===============================================================
*/
double vlen(v)
point_type *v;
{
    return(sqrt(v->x*v->x + v->y*v->y + v->z*v->z));
}

/*
===============================================================
   Function: dist3d()
===============================================================
*/
double dist3d(point_type p1,point_type p2)
{
point_type v;

    v.x = p1.x-p2.x;
    v.y = p1.y-p2.y;
    v.z = p1.z-p2.z;
    return(sqrt(v.x*v.x + v.y*v.y + v.z*v.z));
}




#ifdef NEVER

/*
===============================================================
   Function: frac()
===============================================================
*/
double frac(r)
double r;
{
    return(r - floor(r));
}

double frand(s)			/* get random number from seed */
register int  s;
{
	s = s<<13 ^ s;
	return(1.0-((s*(s*s*15731+789221)+1376312589)&0x7fffffff)/1073741824.0);
}

#endif
