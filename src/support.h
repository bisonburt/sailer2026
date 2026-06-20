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
    Summery:  | the prototypes for the support routines
              |
              |
==============================================================================
*/

char *stralloc(char *);
extern double normalize(point_type *);
extern double dot(point_type, point_type);
extern void vect_mat(point_type,matrix3_type , point_type *);
extern void vect_t_mat(point_type,matrix3_type , point_type *);
extern double frac(double);
extern double frand(int);
extern void InitNoise(void);
extern double Noise(double,double,double,int);
extern double FNoise(double,double,double,int);
void InitRotMat(point_type *, matrix3_type );