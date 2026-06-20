/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | mandel.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the mandel structure
              |
              |
==============================================================================
*/


typedef struct
{
    double startx,starty;
    double endx,endy;
    double rep;
    int maxcount;
    struct sail_module_struct *rangemodule;
} Mandel_type;
