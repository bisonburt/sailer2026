/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | sailpublic.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | defines the public data structures used in sail.
              |
              |
==============================================================================
*/


typedef struct
{
    point_type normal;
    point_type hitpoint;    /* local hitpoint (can be modified by bbox) */
    point_type G_hitpoint;  /* global hitpoint */
    point_type gradu,gradv;
} input_type;

typedef struct
{
    rgb_type color;   /* diffuse color */
    rgb_type reflect; /* specular reflection */
    rgb_type trans;   /* transparency */
    double highlight; /* specular highlight */
    point_type newnormal; /* modified normal */
} output_type;

int InitSail(char *);
void *InitTexture(char *);
int GetTexture(void *,input_type *,output_type *);
