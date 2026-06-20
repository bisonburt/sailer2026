/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | parse-tree.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Defines the data structures for the parse/expression tree.
              |
              |
==============================================================================
*/


/* typedefs go here */

typedef struct value
{
    struct value *next; /* for lists */
    enum v_type {INT,CHAR,CHARPTR,DOUBLE,XYZ,RGB,BAD}  Type;
    union
    {
        int IntVal;
        char CharVal;
        char *CharPtrVal;
        double DoubleVal;
        rgb_type RGBVal;
        point_type XYZVal;
    } u;
} value_type;

typedef struct rvalue
{
    enum r_type {EXPRLST,MODULE,ASSMTS,EMPTY} Type; /* flag for union */
    union
    {
        value_type *ExprLst;    /* for EXPR  */
        struct sail_module_struct *Module;       /* for MODULE */
        struct assmt *Assmts; /* for ASSMTS */
    } data;
} rvalue_type;

typedef struct assmt
{
    struct assmt *next; /* for lists */
    char *lvalue;
    rvalue_type rvalue;
} assmt_type;


/* enumerated list of operator structures. We id on this */
typedef enum
{
    SAIL_MODULE,
    ASSMT_STRUCT,
    RVALUE_STRUCT,
    VALUE_STRUCT,

} OperatorID;

typedef struct node *nodep;

/* externs go here */
extern void AddRGBConst(char *,rgb_type);
extern rgb_type *GetRGBConst(char *);
extern void DumpColorTable();
extern void DoOperator(char,value_type *,value_type *,value_type *);
extern int DoInt(char,int,int);
extern double DoDouble(char,double,double);
extern void DoXYZ(char,point_type,point_type,point_type *);
extern void DoRGB(char,rgb_type,rgb_type,rgb_type *);
extern void DoScalarXYZ(char,double,point_type,point_type *);
extern void DoScalarRGB(char,double,rgb_type,rgb_type *);
extern void DumpExpValue(value_type *);
extern void DumpExpList(value_type *);
void DumpAssmtList(assmt_type *);
