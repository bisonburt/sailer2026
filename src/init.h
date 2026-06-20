/* init.c function prototypes */
void InitRGB(rgb_type *,double,double,double);
void InitPoint(point_type *,double,double,double);
prim_type *GetObjectDataBase(void);
prim_type * AllocSectObject(void);
prim_type *CloneObjectDatabase(prim_type *src);
