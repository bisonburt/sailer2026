/*
 *   File     : patch.h
 *   Author   : Sean Graves
 *
 *   Computer : Any 
 *   Date     : 6/21/90 
 *
 *   Contains function prototypes for the routines in patch.c.
 *   Also defines data structures used in the definition of patches and
 *   the computations used to calculate intersections.
 *
 *   Important data structures:
 *     Patch, PatchPtr - Contains the definition of a patch, and precomputed
 *                       intermediate values.
 *     TreeNode        - A node in the hierarchical bounding box tree for
 *                       a patch.
 *     ListNode, ListPtr - An entry in the intersection list for a patch.
 *                         This intersection list is only used internally
 *                         within the intersection algorithm.
 *     Isectrec          - A record returned by the intersection algorithm
 *                         containing t, u and v, and the actual point.
 *                         If there was no intersection, t = -1.0.
 *
 * Copyright (c) 1990, by Sean Graves and Texas A&M University
 *
 * Permission is hereby granted for non-commercial reproduction and use of
 * this program, provided that this notice is included in any material copied
 * from it. The author assumes no responsibility for damages resulting from
 * the use of this software, however caused.
 *
 */

typedef struct { float x,y,z;           } POINT,VECTOR;
typedef struct { POINT org; VECTOR dir; } RAY;
typedef float MATRIX[4][4];

/*********************************************************************/

typedef struct tnode *TreePtr;   /* Pointer to a tree node */

typedef struct {
   MATRIX geomx, geomy, geomz;   /* Geometry matrices */
   MATRIX M, MT;                 /* Spline basis, transpose */
   MATRIX Md, MdT;               /* Deriv. spline basis, transpose */
   MATRIX Mx, My, Mz;            /* Blending matrices (MGMT) */
   MATRIX Mxdu, Mxdv;            /* For calculation of surface normals */
   MATRIX Mydu, Mydv;
   MATRIX Mzdu, Mzdv;
   TreePtr tree;                 /* Points to bounding-box hierarchy */
} Patch, *PatchPtr;

/*********************************************************************/

typedef struct tnode {           /* A node in the bnding-box hierarchy */
   float u_mid, v_mid;           /* Approximate guess for u, v */
   POINT box_min, box_max;       /* Extent of box */
   TreePtr child[4];             /* Pointers to four children of node */
} TreeNode;

/*******************************************************************/

typedef struct lnode *ListPtr;     /* Pointer to intersection list elt */

typedef struct lnode {             /* Intersection list elt */
   float t;                        /* Distance from ray origin to int */
   TreePtr node;                   /* Pointer to box node */
   ListPtr next;                   /* Next element in list */
} ListNode; 

/*********************************************************************/

typedef struct {                   /* Intersection record returned */
   POINT isect;
   float t, u, v;
} Isectrec;

/******************* Function Prototypes ***************************/

PatchPtr NewPatch();
void DefPatch();
void FreePatch();
float BoxIntersect();     /* Probably should be moved */
VECTOR NormalPatch();
Isectrec IsectPatch ();  

