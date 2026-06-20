/*
 *   File     : vector.c
 *   Author   : Sean Graves
 *
 *   Computer : Any 
 *   Date     : 6/21/90 
 *
 *   Basic linear algebra stuff used by the patch.c routines
 *
 *
 * Copyright (c) 1990, by Sean Graves and Texas A&M University
 *
 * Permission is hereby granted for non-commercial reproduction and use of
 * this program, provided that this notice is included in any material copied
 * from it. The author assumes no responsibility for damages resulting from
 * the use of this software, however caused.
 *
 */

#include "math.h"
#include "patch.h"
#define TOL 0.0001

/************************************************************/

void mmult4x4_4x4(a, b, c)
MATRIX a, b, c;
{
   int i, j, k;
   MATRIX temp;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         temp[i][j] = 0;
         for (k = 0; k < 4; k++) {
            temp[i][j] += (b[i][k] * c[k][j]);
	 }
      }
   }
   for (i = 0; i < 4; i++) 
      for (j = 0; j < 4; j++)
         a[i][j] = temp[i][j];
}

/************************************************************/

void mmult1x4_4x1(a, b, c)
float a[1][1], b[1][4], c[4][1];
{
   int k;
   float temp;
   
   temp = 0;
   for (k = 0; k < 4; k++) {
      temp += (b[0][k] * c[k][0]);
   }

   a[0][0] = temp;
}

/************************************************************/

void mmult4x4_4x1(a, b, c)
float a[4][1], b[4][4], c[4][1];
{
   int i, k;
   float temp[4][1];

   for (i = 0; i < 4; i++) {
      temp[i][0] = 0;
      for (k = 0; k < 4; k++) {
         temp[i][0] += (b[i][k] * c[k][0]);
      }
   }
   for (i = 0; i < 4; i++) 
      a[i][0] = temp[i][0];
}

/************************************************************/

void mmult1x4_4x4(a, b, c)
float a[1][4], b[1][4], c[4][4];
{
   int j, k;
   float temp[1][4];

   for (j = 0; j < 4; j++) {
      temp[0][j] = 0;
      for (k = 0; k < 4; k++) {
         temp[0][j] += (b[0][k] * c[k][j]);
      }
   }
   for (j = 0; j < 4; j++)
      a[0][j] = temp[0][j];
}

/************************************************************/

void transpose(a,b)
MATRIX a, b;
{
   int i, j;

   for (i = 0; i < 4 ; i++) 
      for (j = 0; j < 4; j++)
         b[j][i] = a[i][j];
}
 
/************************************************************/

int fcmp(a,b)
float a,b;
{
   if (fabs(a - b) < TOL) return 1; 
   return 0;
}

/************************************************************/

float length(v1, v2)
POINT v1, v2;
{
   float a, b, c;

   a = v1.x - v2.x;
   b = v1.y - v2.y;
   c = v1.z - v2.z;

   return sqrt(a*a + b*b + c*c);
}


/**********************************************************/

void normalize(vec)
VECTOR *vec;
{
   float mag;
 
   mag = sqrt(vec->x*vec->x + vec->y*vec->y + vec->z*vec->z);
   vec->x /= mag;
   vec->y /= mag;
   vec->z /= mag;
}

/************************************************************/

void cross_product(c, a, b)    /* c = a X b */
VECTOR a, b, *c;
{
   c->x = a.y*b.z - a.z*b.y;
   c->y = a.z*b.x - a.x*b.z;
   c->z = a.x*b.y - a.y*b.x;
}

/************************************************************/

float dot_product(a, b)
VECTOR a, b;
{
   return a.x*b.x + a.y*b.y + a.z*b.z;
}

/*********************DIAGNOSTIC**********************/

float aref(arr, cols, r, c)
float *arr;
int cols, r, c;
{
    return (float) *(arr + (cols * r) + c);
}

/*********************DIAGNOSTIC*********************/

dump(a, rs, cs)
float *a;
int rs, cs;
{
   int i,j;

   for (i = 0; i < rs; i++) {
	for (j = 0; j < cs; j++)
		printf("%6.2f  ",aref(a, cs, i, j));
        printf("\n");
   }
}

