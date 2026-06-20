/* function prototypes mathfun.c routines */
extern double normalize(point_type *);
extern double dot(point_type,point_type);
extern void RotScale(point_type *,point_type *,double []);
extern void vec_mat(point_type,double *,point_type *);
extern void vec_t_mat(point_type,double *,point_type *);
extern void reflect(point_type,point_type,point_type *);
extern void vcross(point_type *,point_type *,point_type *);
extern void vscale(double,point_type *,point_type *);
extern double vlen(point_type *);
extern double frac(double);
extern double frand(int);
double dist3d(point_type,point_type);
void mat_mat(double [3][3],double [3][3],double [3][3]);
