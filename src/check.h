/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | check.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the check structure
              |
              |
==============================================================================
*/


typedef struct
{
    point_type rep; /* amount of checkers in x,y,z */
    int flags;      /* bit 0: localx = 0, globalx = 1
                       bit 1: localy = 0, globaly = 1 */
    struct sail_module_struct *odd;
    struct sail_module_struct *even;
} Check_type;