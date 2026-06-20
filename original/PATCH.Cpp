/*
 *   File     : patch.c
 *   Author   : Sean Graves
 *
 *   Computer : Any 
 *   Date     : 6/21/90 
 *
 *   A set of routines for creating and raytracing bicubic surface
 *   patches.  The file patch.h contains function prototypes for
 *   the routines in this file.
 *
 *   User Routines:
 *     NewPatch - Allocates the patch data structures and returns a pointer.
 *     DefPatch - Defines a patch, given a patch pointer.
 *     IsectPatch - Intersects a ray and a patch. Returns u, v, t, pnt. 
 *     NormalPatch - Given a patch and a u, v position, returns the normal.
 *     FreePatch - Deallocates a patch.
 *     
 *   Additionally, there is one useful routine not specific to patches:
 *     BoxIntersect - Given a ray and a box, returns the intersection point.
 *
 * Copyright (c) 1990, by Sean Graves and Texas A&M University 
 *
 * Permission is hereby granted for non-commercial reproduction and use of
 * this program, provided that this notice is included in any material copied
 * from it. The author assumes no responsibility for damages resulting from
 * the use of this software, however caused.
 *
 */

#include "patch.h"
#include "vector.h"
#include "math.h"
#include <malloc.h>

#define NEWTON_TOL 0.01
#define MAX_ITER 3
#define OVERLAP 0.1
#define INF (1e99)
#define MAXDIV 16
#define TRUE 1
#define FALSE 0
#define NULL 0 

/*  Function Prototypes for local functions */

static void samplepatch();
static compute_overlap();
static TreePtr buildtree();
static ListPtr insert_node(); 
static void DumpList();          /* Diagnostic */
static void free_list();
static float eval_patch();
static void newton();   

static POINT samples[MAXDIV +1][MAXDIV +1]; 

/*************************************************************************/

PatchPtr NewPatch()
{
   return((PatchPtr) malloc(sizeof(Patch)));
}

/*************************************************************************/

void FreePatch(p)
PatchPtr p;
{
   free(p);
}

/*************************************************************************/

void DefPatch(p, M, geomx, geomy, geomz, div)
PatchPtr p;                      /* Pointer to patch struct */
MATRIX M, geomx, geomy, geomz;   /* Spline basis, geometry matrices */
int div;                         /* Number of divisions for tree */
{
   int i, j;

 
   /* Copy basic information into patch struct */
 
   for (i = 0; i < 4; i++) 
      for (j = 0; j < 4; j++) { 
         p->M[i][j] = M[i][j];
         p->geomx[i][j] = geomx[i][j];
         p->geomy[i][j] = geomy[i][j];
         p->geomz[i][j] = geomz[i][j];
      }

   /* Compute derivative of basis matrix */

   p->Md[0][0] = p->Md[0][1] = p->Md[0][2] = p->Md[0][3] = 0.0;
   for (i = 1; i < 4; i++) 
      for (j = 0; j < 4; j++) 
         p->Md[i][j] = p->M[i-1][j] * (4-i); 
       
   /* Fill in transpose fields of patch struct */

   transpose(p->M , p->MT );
   transpose(p->Md, p->MdT);

   /* Compute intermediate blending matrices */

   mmult4x4_4x4(p->Mx, p->M, p->geomx); /* Calculate blending matrix for x,y,z */
   mmult4x4_4x4(p->My, p->M, p->geomy);
   mmult4x4_4x4(p->Mz, p->M, p->geomz);
   mmult4x4_4x4(p->Mx, p->Mx, p->MT);
   mmult4x4_4x4(p->My, p->My, p->MT);
   mmult4x4_4x4(p->Mz, p->Mz, p->MT);

   mmult4x4_4x4(p->Mxdu, p->Md, p->geomx);  /* Calculate d/du matrix for x,y,z */
   mmult4x4_4x4(p->Mydu, p->Md, p->geomy);
   mmult4x4_4x4(p->Mzdu, p->Md, p->geomz);
   mmult4x4_4x4(p->Mxdu, p->Mxdu, p->MT);
   mmult4x4_4x4(p->Mydu, p->Mydu, p->MT);
   mmult4x4_4x4(p->Mzdu, p->Mzdu, p->MT);
 
   mmult4x4_4x4(p->Mxdv, p->M, p->geomx);  /* Calculate d/dv matrix for x,y,z */
   mmult4x4_4x4(p->Mydv, p->M, p->geomy);
   mmult4x4_4x4(p->Mzdv, p->M, p->geomz);
   mmult4x4_4x4(p->Mxdv, p->Mxdv, p->MdT);
   mmult4x4_4x4(p->Mydv, p->Mydv, p->MdT);
   mmult4x4_4x4(p->Mzdv, p->Mzdv, p->MdT);

   samplepatch(p, div);
   p->tree = buildtree(0, div, 0, div, div);

 }
	
/*******************************************************************/

static void samplepatch(p, div)
PatchPtr p;
int div;
{
   float U[1][4], V[4][1];         /* u, v vectors */
   MATRIX C;                       /* sol matrix (only [0][0] used) */
   MATRIX M2x, M2y, M2z;           /* Intermediate results */
   float u, v;                     /* Parameters */
   int i,j;

   for (i = 0; i <= div; i++) {
      u = (float) i / div;
      U[0][3] = 1;
      U[0][2] = u;
      U[0][1] = u * u;
      U[0][0] = U[0][1] * u;
      mmult1x4_4x4 (M2x, U, p->Mx);
      mmult1x4_4x4 (M2y, U, p->My);
      mmult1x4_4x4 (M2z, U, p->Mz);
      for (j = 0; j <= div; j++) {
        v = (float) j / div;
        V[3][0] = 1;
        V[2][0] = v;
        V[1][0] = v * v;
        V[0][0] = V[1][0] * v;
        mmult1x4_4x1 (C, M2x, V);
        samples[i][j].x = C[0][0];
        mmult1x4_4x1 (C, M2y, V);
        samples[i][j].y = C[0][0];
        mmult1x4_4x1 (C, M2z, V);
        samples[i][j].z = C[0][0];
      }
   }
}

/***************************************************************************/

static compute_overlap(curr_node)
TreePtr curr_node;
{
   int i;
   float incrs;
/*
   for (i=0; i<3; i++) {
      incrs = fabs((curr_node->box_max[i] - curr_node->box_min[i]) * OVERLAP);
      curr_node->box_min[i] -= incrs;
      curr_node->box_max[i] += incrs;
   } 
*/
}

/***************************************************************************/

static TreePtr buildtree(u_low, u_hi, v_low, v_hi, div)
int u_low, u_hi, v_low, v_hi, div;
{
   int u_mid, v_mid, i, j;
   TreePtr curr_node;
   
   curr_node = (TreePtr) malloc(sizeof(TreeNode));
   if (!curr_node) { printf("Error - malloc failed\n"); exit(0); }
   if ((u_low+1 == u_hi) || (v_low+1 == v_hi)) {         /* Found leaf */
                                                       /* Compute midpoint */
      curr_node->u_mid = ((((float) (u_hi - u_low)) / 2.0) + u_low) / div ; 
      curr_node->v_mid = ((((float) (v_hi - v_low)) / 2.0) + v_low) / div ; 
                                                       /* Compute box */
      if (length(samples[u_low][v_low], samples[u_hi][v_hi]) >
          length(samples[u_hi][v_low], samples[u_low][v_hi])) {
         curr_node->box_min.x = samples[u_low][v_low].x;
         curr_node->box_max.x = samples[u_hi ][v_hi ].x;
         curr_node->box_min.y = samples[u_low][v_low].y;
         curr_node->box_max.y = samples[u_hi ][v_hi ].y;
         curr_node->box_min.z = samples[u_low][v_low].z;
         curr_node->box_max.z = samples[u_hi ][v_hi ].z;
      } else {
         curr_node->box_min.x = samples[u_hi ][v_low].x;
         curr_node->box_max.x = samples[u_low][v_hi ].x;
         curr_node->box_min.y = samples[u_hi ][v_low].y;
         curr_node->box_max.y = samples[u_low][v_hi ].y;
         curr_node->box_min.z = samples[u_hi ][v_low].z;
         curr_node->box_max.z = samples[u_low][v_hi ].z;
      }
      for (j = 0; j < 4; j++) curr_node->child[j] = NULL;
 
   } else {                                           /* Not leaf node */ 
      u_mid = (u_hi - u_low) / 2 + u_low; 
      v_mid = (v_hi - v_low) / 2 + v_low;

      curr_node->child[0] = buildtree(u_low, u_mid, v_low, v_mid, div); 
      curr_node->child[1] = buildtree(u_mid, u_hi, v_low, v_mid, div); 
      curr_node->child[2] = buildtree(u_low, u_mid, v_mid, v_hi, div);
      curr_node->child[3] = buildtree(u_mid, u_hi, v_mid, v_hi, div);
                                                      /* Compute box */
      curr_node->box_min.x = 1e99;
      curr_node->box_max.x = -1e99;
      for (j = 0; j < 4; j++) {;
         if (curr_node->child[j]->box_min.x < curr_node->box_min.x)
            curr_node->box_min.x = curr_node->child[j]->box_min.x;
         if (curr_node->child[j]->box_min.x > curr_node->box_max.x)
            curr_node->box_max.x = curr_node->child[j]->box_min.x;
         if (curr_node->child[j]->box_max.x < curr_node->box_min.x)
            curr_node->box_min.x = curr_node->child[j]->box_max.x;
         if (curr_node->child[j]->box_max.x > curr_node->box_max.x)
            curr_node->box_max.x = curr_node->child[j]->box_max.x;
      }
      curr_node->box_min.y = 1e99;
      curr_node->box_max.y = -1e99;
      for (j = 0; j < 4; j++) {;
         if (curr_node->child[j]->box_min.y < curr_node->box_min.y)
            curr_node->box_min.y = curr_node->child[j]->box_min.y;
         if (curr_node->child[j]->box_min.y > curr_node->box_max.y)
            curr_node->box_max.y = curr_node->child[j]->box_min.y;
         if (curr_node->child[j]->box_max.y < curr_node->box_min.y)
            curr_node->box_min.y = curr_node->child[j]->box_max.y;
         if (curr_node->child[j]->box_max.y > curr_node->box_max.y)
            curr_node->box_max.y = curr_node->child[j]->box_max.y;
      }
      curr_node->box_min.z = 1e99;
      curr_node->box_max.z = -1e99;
      for (j = 0; j < 4; j++) {;
         if (curr_node->child[j]->box_min.z < curr_node->box_min.z)
            curr_node->box_min.z = curr_node->child[j]->box_min.z;
         if (curr_node->child[j]->box_min.z > curr_node->box_max.z)
            curr_node->box_max.z = curr_node->child[j]->box_min.z;
         if (curr_node->child[j]->box_max.z < curr_node->box_min.z)
            curr_node->box_min.z = curr_node->child[j]->box_max.z;
         if (curr_node->child[j]->box_max.z > curr_node->box_max.z)
            curr_node->box_max.z = curr_node->child[j]->box_max.z;
      }
   }
/*   compute_overlap(curr_node); */
   return curr_node;
}







/*************************************************************************/

float BoxIntersect(r, b1, b2) 
RAY r;
POINT b1, b2;
{
   float t1, t2, tswap, tnear, tfar, dinv;

   /*  Check X slabs */
   if (fcmp (r.dir.x, 0.0)) {  /* Ray is parallel to X planes of box */
      if (!(((b1.x <= r.org.x ) && (r.org.x <= b2.x)) ||
            ((b2.x <= r.org.x ) && (r.org.x <= b1.x))))/* But not between */
         return -1.0;
      tfar = INF;
      tnear = -INF;
   } else {
      dinv = 1.0/r.dir.x;         /* Calculate inverse for speed */
      t1 = (b1.x - r.org.x) * dinv;
      t2 = (b2.x - r.org.x) * dinv;

      if (t1 > t2) { tfar = t1; tnear = t2; } 
      else         { tfar = t2; tnear = t1; } 
      if (tfar < 0.0)              /* Box is behind ray */
         return -1.0;
   }

   /* Check Y slabs */

   if (fcmp (r.dir.y, 0.0)) {   /* Ray is parallel to Y planes of box */
      if (!(((b1.y <= r.org.y ) && (r.org.y <= b2.y)) ||
            ((b2.y <= r.org.y ) && (r.org.y <= b1.y))))/* But not between */
         return -1.0;
   } else {
      dinv = 1.0/r.dir.y;         /* Calculate inverse for speed */
      t1 = (b1.y - r.org.y) * dinv;
      t2 = (b2.y - r.org.y) * dinv;

      if (t1 > t2) { tswap = t1; t1 = t2; t2 = tswap; }

      if (t1 > tnear) tnear = t1;
      if (t2 < tfar)  tfar = t2;

      if (tnear > tfar)            /* Box is missed */
         return -1.0;

      if (tfar < 0.0)              /* Box is behind ray */
      return -1.0;     
   }

   /* Check Z slabs */

   if (fcmp (r.dir.z, 0.0)) {   /* Ray is parallel to Z planes of box */
      if (!(((b1.z <= r.org.z ) && (r.org.z <= b2.z)) ||
            ((b2.z <= r.org.z ) && (r.org.z <= b1.z))))/* But not between */
         return -1.0;
   } else {
      dinv = 1.0/r.dir.z;         /* Calculate inverse for speed */
      t1 = (b1.z - r.org.z) * dinv;
      t2 = (b2.z - r.org.z) * dinv;

      if (t1 > t2) { tswap = t1; t1 = t2; t2 = tswap; }

      if (t1 > tnear) tnear = t1;
      if (t2 < tfar)  tfar = t2;

      if (tnear > tfar)            /* Box is missed */
         return -1.0;

      if (tfar < 0.0)              /* Box is behind ray */
         return -1.0;     
   } 
   return tnear;
} 
  
/*************************************************************************/

/*************************************************************************/
/* Create a intersection list node.  Put it in the list based on the
/* parameter t.  Return the new head of the list
/*  Pass: list ptr, node to insert, t
/************************************************************************/

static ListPtr insert_node(list, node, t)
ListPtr list;
TreePtr node;
float t;
{
   ListPtr new, this, back;

   new = (ListPtr) malloc(sizeof(ListNode));
   if (!new) { printf("Error - malloc failed\n"); exit(0); }
   new->t = t;
   new->node = node;
   new->next = NULL;

   if (!list) 
      return new;

   back = NULL;
   this = list;
   while (this && (this->t < new->t)) {
      back = this;
      this = this->next;
   }
   if (!back) {                       /* First item */
     new->next = list;
     return new;
   } else {                             /* Second through last item */
     back->next = new;
     new->next  = this;
     return list;
   }
}   
   
/************************************************************************/

static void dump_list(list) 
ListPtr list;
{
   ListPtr this;

   this = list;
   while (this) {
      printf("%4.2f  ",this->t);
      this = this->next;
   }
   printf("\n");
}

/************************************************************************/

static void free_list(list) 
ListPtr list;
{
   ListPtr this;

   this = list;
   while (this) {
      list = this->next;
      free(this);
      this = list;
   }
}
 
/************************************************************************/

static float eval_patch(u, M, G, MT, v)
float u, v;
MATRIX M, G, MT;
{
   float U[1][4], V[4][1];              /* u, v vectors */
   MATRIX M2;                           /* Intermediate results */
   int i,j;

   U[0][3] = 1;
   U[0][2] = u;
   U[0][1] = u * u;
   U[0][0] = U[0][1] * u;
   V[3][0] = 1;
   V[2][0] = v;
   V[1][0] = v * v;
   V[0][0] = V[1][0] * v;

   mmult1x4_4x4 (M2, U, M);
   mmult1x4_4x4 (M2, M2, G);
   mmult1x4_4x4 (M2, M2, MT);
   mmult1x4_4x1 (M2, M2, V); 
   return M2[0][0];
}

/************************************************************************/

static void newton(surface, node, r, irec)
PatchPtr surface;
TreePtr node;
RAY r;
Isectrec *irec;
{
   int iterations, success, failure, i, j;
   float u, v, error, last_error;
   float plane1d, plane2d, E1, E2;
   VECTOR plane1abc, plane2abc, temp; 
   MATRIX H1, H2;
   float dfdu, dfdv, dgdu, dgdv;

   /* get initial values for u and v from tree node */
   u = node->u_mid;
   v = node->v_mid;
 
   /* calculate plane1, plane2 */
   cross_product(&plane1abc, r.org, r.dir);   
   normalize(&plane1abc);
   cross_product(&plane2abc, plane1abc, r.dir);
   normalize(&plane2abc);

   plane1d = dot_product(plane1abc, r.org);
   plane2d = dot_product(plane2abc, r.org); 

   /* calculate the H matrices */
   for (i=0; i<4; i++) {
      for (j=0; j<4; j++) {
         temp.x = surface->geomx[i][j];
         temp.y = surface->geomy[i][j];
         temp.z = surface->geomz[i][j];
         H1[i][j] = dot_product(plane1abc, temp);
         H2[i][j] = dot_product(plane2abc, temp); 
      }
   }

   /* begin iteration */

   error = 0;
   iterations = 0;
   success = failure = FALSE;
   while (!success && !failure) {
      E1 = eval_patch(u, surface->M, H1, surface->MT, v) - plane1d; 
      E2 = eval_patch(u, surface->M, H2, surface->MT, v) - plane2d; 
      last_error = error;
      error = fabs(E1) + fabs(E2);
      if (error < NEWTON_TOL) 
         success = TRUE;
      else if ((iterations >= MAX_ITER) && (error >= last_error)) 
         failure = TRUE;
      else {                /****** calc newton step ***  f = E1, g = E2 **/ 

         dfdu = eval_patch(u, surface->Md, H1, surface->MT, v);
         dfdv = eval_patch(u, surface->M, H1, surface->MdT, v);
         dgdu = eval_patch(u, surface->Md, H2, surface->MT, v);
         dgdv = eval_patch(u, surface->M, H2, surface->MdT, v);

         u += ((E2*dfdv-E1*dgdv)/(dfdu*dgdv-dfdv*dgdu));  /* Delta u */
         v += ((E2*dfdu-E1*dgdu)/(dfdv*dgdu-dfdu*dgdv));  /* Delta v */

         iterations++; 
      }
   }
  
   if (success) {
      irec->isect.x = eval_patch(u, surface->M, surface->geomx, surface->MT, v);
      irec->isect.y = eval_patch(u, surface->M, surface->geomy, surface->MT, v);
      irec->isect.z = eval_patch(u, surface->M, surface->geomz, surface->MT, v);
      irec->t = length (r.org, irec->isect);
      irec->u = u;
      irec->v = v;
   }
   else irec->t = -1.0;
}

/************************************************************************/

Isectrec IsectPatch (surface, r)
PatchPtr surface;
RAY r;
{
   int i;
   float tmin, t;
   ListPtr this, head; 
   ListNode thisnode;
   Isectrec isect, isectmin;

   head = insert_node(NULL, surface->tree, 0); 
   tmin = INF;
   isectmin.t = -1.0;              /* Use -1.0 to indicate no intersection */

   while (head) {
      this = head;
      head = this->next;
      thisnode = *this;
      free(this);
      if (!thisnode.node->child[0]) {     /* if node is a leaf node */
         newton(surface, thisnode.node, r, &isect); 
         if ((isect.t != -1.0) && (isect.t < tmin)) {
            tmin = isect.t;    
            isectmin = isect;
         }
      } else {
         for (i = 0; i < 4; i++) {                  /* for each child of node */
            t = BoxIntersect(r, thisnode.node->child[i]->box_min,
                                thisnode.node->child[i]->box_max);
            if ( t != -1.0)                         /* if ray intersects box */
               head = insert_node(head, thisnode.node->child[i], t);
         }
      }
      if ((head) && (tmin < head->t)) { free_list(head); head = NULL; }
   }
   return isectmin;
}   
      
/********************************************************************/


VECTOR NormalPatch (p, u, v)
PatchPtr p;
float u, v;
{
   VECTOR ddu, ddv, norm;

   ddu.x = eval_patch(u, p->Md, p->geomx, p->MT, v);
   ddu.y = eval_patch(u, p->Md, p->geomy, p->MT, v);
   ddu.z = eval_patch(u, p->Md, p->geomz, p->MT, v);

   ddv.x = eval_patch(u, p->M, p->geomx, p->MdT, v);
   ddv.y = eval_patch(u, p->M, p->geomy, p->MdT, v);
   ddv.z = eval_patch(u, p->M, p->geomz, p->MdT, v);

   cross_product(&norm, ddu, ddv);
   normalize(&norm); 
   return norm;
}
