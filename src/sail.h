/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | sail.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | This file will defines many of the structures that are needed
              | to define a module type. Modify this file if you are adding
              | or deleting modules to the language.
              |
==============================================================================
*/


/*
===================================================================
    STRUCTURE: name_token_type
    USE: to create a list of lvalues for use in parsing modules.
    See a module file like paramap.c for its use.
===================================================================
*/
typedef struct
{
    char name[16];
    int token;
} name_token_type;


/*
===================================================================
    STRUCTURE:  typedef Bound_type
    USE: This structure stores the information for a bounding box.
===================================================================
*/
typedef struct  /* bounding box data structure */
{
    point_type loc; /* location of center point of box */
    point_type size; /* the size of the bounding box */
    point_type rot; /* rotation of the bounding box */
    int flags; /* bit 0 = WIDEX, bit 1 = WIDEY, bit2 = WIDEZ */
    int rotate;  /* true/false flag */
    matrix3_type mat; /* calculated rotation matrix */
} Bound_type;


/*
==============================================
    .h files for modules are included here
    add the .h file for a module in this file.
==============================================
*/
#include "sailmod.h"



/*
===================================================================
    STRUCTURE: typedef sail_module_type
    USE: defines the module type that gets used everywhere
===================================================================
*/
typedef struct sail_module_struct
{
    struct sail_module_struct *next;
    char *name;
    int class;
    void (*funct)(struct sail_module_struct *,input_type *,output_type *);
    union
    {
        /* ADD_BELOW_FOR_MODULE */
        Bound_type bbox;        /* bounding box */
        Para_type ParaMap;      /* parametric mapper module */
        Check_type Check;       /* checkerboard pattern */
        RangeP_type Range;   /* range */
        Bitmap_type bitmap;     /* bitmap module */
        Mandel_type Mandel;     /* mandelbrot module */
        Attrib_type Attrib;     /* attribute module */
    } module;
} sail_module_type;


/* external functions */
sail_module_type * NewSailModule(int, char *, assmt_type *);
sail_module_type * BuildBBox(assmt_type *);
void RunBBox(Bound_type *,input_type *,point_type *);
int GetToken(char *,name_token_type *);
int CheckValue(assmt_type *,enum r_type,enum v_type,char *);
int UnassignedErr(char *str);
sail_module_type *FindModule(char *);



/* ADD_BELOW_FOR_MODULE */
sail_module_type * BuildParaMap(assmt_type *);
sail_module_type * BuildCheck(assmt_type *);
sail_module_type * BuildRange(assmt_type *);
sail_module_type * BuildBitmap(assmt_type *);
sail_module_type * BuildMandel(assmt_type *);
sail_module_type * BuildAttrib(assmt_type *);


/* these are for the raytracer and not the sail language */
extern void * BuildSphere(assmt_type *);
extern void * BuildConic(assmt_type *);
extern void * BuildBox(assmt_type *);
extern void * BuildBoard(assmt_type *);
extern void * BuildTriangle(assmt_type *);
extern void * BuildCSG(assmt_type *);
extern void  BuildView(assmt_type *);
extern void  BuildGlobal(assmt_type *);
