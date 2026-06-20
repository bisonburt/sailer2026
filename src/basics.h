/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     |
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | some datatypes and values used in sail.
              |
              |
==============================================================================
*/

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

typedef struct
{
    double r,g,b;
} rgb_type;

typedef struct
{
    double x,y,z;
} point_type;

typedef double matrix3_type[9];
