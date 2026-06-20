/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | bitmap.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the bitmap structure
              |
              |
==============================================================================
*/


typedef struct
{
    int RetType; /* bit 0: color (0 off  1 on)
                    bit 1: refl
                    bit 2: trans */
    point_type rep;
    int x,y;
    char *bitmap;
} Bitmap_type;
