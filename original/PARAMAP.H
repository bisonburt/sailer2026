/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | paramap.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the paramap structures
              |
              |
==============================================================================
*/


typedef struct
{
    enum pm_type
    {
        OFF,
        PLANARX,PLANARY,PLANARZ,
        RADIALX,RADIALY,RADIALZ,
        ANGULARX,ANGULARY,ANGULARZ,
        DISTANCE,NOISE,FNOISE,CONSTANT
    } Type;
    double min; /* minimum value */
    double max; /* maximum value */
    double rep; /* reptition factor */
    point_type lattice; /* lattice units per bounding cube (in x,y,z) */
} map_type;

typedef struct Para
{
    Bound_type *BBox;       /* unit bounding box */
    map_type mapx,mapy,mapz;/* map_type for more info */
    struct sail_module_struct *module;
} Para_type;
