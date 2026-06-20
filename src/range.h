/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | range.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the range structures
              |
              |
==============================================================================
*/


typedef struct
{
    int flags;
    double start;
    double stop;
    enum {SOLID,FADE} Type;
    struct sail_module_struct *startv;
    struct sail_module_struct *stopv;
} range_type;

typedef struct RangeP_type
{
    double rep;
    int cnt; /* total number of used RangeList slots */
    int rangevalue; /* 1 = XVALUE, 2 = YVALUE, 3 = ZVALUE */
    struct sail_module_struct *defaultv;
    range_type RangeList[30];
} RangeP_type;


