/*
========================================================================
    WHAT:   sail.h
    FOR:    Surface Attributes Interpeted Language (SAIL)
    BY:     David Fletcher
    WHEN:   28 Feb 1992
    WHY:
========================================================================
*/

/*
===================================================================
    STRUCTURE:
    USE:
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
===================================================================
    STRUCTURE:  typedef out_type
    USE: When a value is determined in a module, we must pass it
    back to the caller, or call another module that will use the
    computed value. This structure depending on the "Type" will
    supply the module to call or simply return a value back.
===================================================================
*/
typedef struct
{
    enum o_value {SAILMODULE,RETURN} Type;
    struct sail_module_struct *module;
    value_type Value;
} out_type;

/*
===========================================
    .h files for modules are included here
===========================================
*/
#include "SailMod.h"


typedef struct sail_module_struct
{
    struct sail_module_struct *next;
    char *name;
    int RetType; /* return type ie: diffuse color, reflective color, ect */
    void (*funct)(struct sail_module_struct *,input_type *,output_type *);
    enum {PARAMAP,CRANGE,BBOX} Type;
    union
    {
        /* ADD_BELOW_FOR_MODULE */
        Bound_type bbox;    /* bounding box */
        Para_type ParaMap;  /* parametric mapper module */
        RangeP_type CRange;  /* color range module */
        Check_type Check;   /* checkerboard pattern */
        Bitmap_type bitmap; /* bitmap module */
        Mandel_type Mandel; /* mandelbrot module */
        Attrib_type Attrib;     /* attribute module */
    } module;
} sail_module_type;

/* external functions */
extern sail_module_type * NewSailModule(int, char *, assmt_type *);
extern sail_module_type * BuildBBox(assmt_type *);
void RunBBox(Bound_type *,point_type *,point_type *);
int GetToken(char *,name_token_type *);
int CheckValue(assmt_type *,enum r_type,enum v_type,char *);
int UnassignedErr(char *);

/* ADD_BELOW_FOR_MODULE */
extern sail_module_type * BuildParaMap();
extern sail_module_type * BuildCRange();
extern sail_module_type * BuildCheck();
extern sail_module_type * BuildNewCheck();
extern sail_module_type * BuildBitmap();
extern sail_module_type * BuildMandel();
extern void *BuildSphere();
extern void *BuildConic();
extern void *BuildBox();