/*
    file: structs.h
    for:  ray tracer
    by:   David Fletcher
    on:   05 Apr 1993
*/

#define PI       3.1415
#define PITIMES2 6.283185
#define PIOVER2  1.570796
#define SQRT2    1.414214
#define SQRT3    1.732051
#define DEG2RAD 0.01745329

#define TRUE 1
#define FALSE 0


#define true 1
#define false 0

#define null 0

extern int SAIL_ERROR_FLAG; /* GLOBAL */

typedef double matrix3_type[9];

typedef struct
{
    double r,g,b;       /* a rgb value */
} rgb_type;

typedef struct
{
    double x,y,z;       /* a point in space or a vector */
} point_type;

typedef struct
{
    point_type s,d;     /* source (point), destination (vector) */
} ray_type;

typedef struct
{
    point_type c;
    double r;
    double t[9];
    point_type s,d;
    int capin,capout,swaped_cap;
} conic_type;

typedef struct
{
    point_type c;
    double r;
    double t[9];
    point_type n[6];
    int nornum[2]; /* index to normal # */
} box_type;

typedef struct
{
    point_type c;
    double r;
    point_type P[3];
    point_type N;   /* to be computed once */
    double d;       /* to be computed once */
    int i0;
} tri_type;

typedef struct
{
    point_type c;
    double r;
} patch_type;

typedef struct
{
    point_type c;
    double r;
} sphere_type;

typedef struct
{
    point_type c;
    double y;
    double l,r,n,f;
} board_type;

typedef struct
{
    struct prim_type *left,*right;          /* CSG childeren */
    char op;                                /* + - * */
} CSG_type;

typedef struct
{
    double u;                               /* distance to intersection */
    point_type nor;                         /* normal */
    point_type hit;                         /* hit point */
    void *SurfAttrib;                       /* SAIL surface data */
    void (*nor_func)(struct prim_type *,int);   /* normal function */
    struct prim_type *origin;               /* points back to struct where */
    int orig_num;                           /* is this a in (0) or a out(1) */
                                            /* this data came from */
    double kdiff,kspec;                     /* diffuse and specular */
} interD_type;

typedef struct data
{
    interD_type data[2];
    struct data *next;
} inter_type;

typedef struct prim_type
{
    void (*sec_func)(struct prim_type *,ray_type *);    /* intersect function */
    void (*bound_func)(struct prim_type *,ray_type *);  /* bounding function */
    sphere_type boundinfo;                              /* bounding info */

    struct prim_type *next;                 /* link to next obj */
    int isInCSG :1;                         /* is this node in a CSG tree T/F */
    int hit;                                /* object hit
                                               - if 0 then no hit
                                               - if > 0 then hit corresponds
                                                 to the FIRST positive u val
                                                 (base 1)
                                               - if < 0 (neg) then no positive
                                                 hit points
                                            */
    inter_type inter;                       /* intersections points */
    union                                   /* union of all primitives */
    {
        board_type board;
        sphere_type sphere;
        conic_type conic;
        box_type box;
        tri_type tri;
        patch_type patch;
        CSG_type CSG;
    } prim;
} prim_type;
