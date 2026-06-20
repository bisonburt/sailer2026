/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | pare_tr.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | This file is a support file for the parser. It does things
              | like math operations on arithmetic expressions. It also has
              | the coror table lookup functions, and some debugging routines
              | for examining expression trees.
              |
==============================================================================
*/

#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mathtype.h"
#include <stdlib.h>
#include "structs.h"
#include "support.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "tokens.h"

/*
================================================
    declration stuff for defined colors
================================================
*/

#define TABLESIZE 512
static struct
{
    char *name;
    rgb_type value;
} Table[TABLESIZE];

static int cur_table_size = 0;

/*
================================================
    functions for color table management
================================================
*/

void AddRGBConst(char *color,rgb_type rgb)
{

    if (cur_table_size < TABLESIZE)
    {
        Table[cur_table_size].name = stralloc(color);
        Table[cur_table_size++].value = rgb;
    }
    else
    {
        printf("[AddColor] ERROR, RGBTable full\n");
        SAIL_ERROR_FLAG = TRUE;
    }
}

rgb_type *GetRGBConst(char *name)
{
int i;
    for(i=0;i<cur_table_size;i++)
    {
        if (strcmp(Table[i].name,name+1) == 0)
        {
            return(&Table[i].value);
        }
    }
    printf("[GetRGBConst] ERROR, Color %s not in RGB table\n",name);
    SAIL_ERROR_FLAG = TRUE;
    return(NULL);
}

void DumpColorTable()  /* for debug use */
{
int i;
    printf("===== COLOR TABLE DUMP =====\n");
    printf("Number of items in table = %d\n",cur_table_size);
    for(i=0;i<cur_table_size;i++)
    {
        printf("Table[%d].name = %s, Table[%d].value = rgb(%f,%f,%f)\n",
        i,Table[i].name,i,Table[i].value.r,Table[i].value.g,Table[i].value.b);
    }
}


/*
================================================
    DoOperator()
    This function will preform operations on
    expressions like add, subtract, etc.
================================================
*/
void DoOperator(char OpToken,value_type *Left ,value_type *Right,value_type *out)
{

/*
    if ((Left->Type == BAD) || (Right->Type == BAD))
        out->Type = BAD;
    else
*/
    if (Left == NULL)
    {
        switch(Right->Type)
        {
            case INT:
                out->Type = INT;
                if (OpToken == '-')
                    out->u.IntVal = - Right->u.IntVal;
                else
                    out->u.IntVal = Right->u.IntVal;
            break;
            case CHAR:
                out->Type = CHAR;
                if (OpToken == '-')
                    out->u.CharVal = - Right->u.CharVal;
                else
                    out->u.IntVal = Right->u.IntVal;
            break;
            case CHARPTR:
                printf("Error: Bad operation (%c CHARPTR)\n",OpToken);
                SAIL_ERROR_FLAG = TRUE;
                out->Type = BAD;
            break;
            case DOUBLE:
                out->Type = DOUBLE;
                if (OpToken == '-')
                    out->u.DoubleVal = - Right->u.DoubleVal;
                else
                    out->u.DoubleVal = Right->u.DoubleVal;
            break;
            case XYZ:
                out->Type = XYZ;
                if (OpToken == '-')
                {
                    out->u.XYZVal.x = - Right->u.XYZVal.x;
                    out->u.XYZVal.y = - Right->u.XYZVal.y;
                    out->u.XYZVal.z = - Right->u.XYZVal.z;
                }
                else
                {
                    out->u.XYZVal.x = Right->u.XYZVal.x;
                    out->u.XYZVal.y = Right->u.XYZVal.y;
                    out->u.XYZVal.z = Right->u.XYZVal.z;
                }
            break;
            case RGB:
                out->Type = RGB;
                if (OpToken == '-')
                {
                    out->u.RGBVal.r = - Right->u.RGBVal.r;
                    out->u.RGBVal.g = - Right->u.RGBVal.g;
                    out->u.RGBVal.b = - Right->u.RGBVal.b;
                }
                else
                {
                    out->u.RGBVal.r = Right->u.RGBVal.r;
                    out->u.RGBVal.g = Right->u.RGBVal.g;
                    out->u.RGBVal.b = Right->u.RGBVal.b;
                }
            break;
        }
    }
    else
    switch(Left->Type)
    {
        case INT:
            switch (Right->Type)
            {
                case INT:
                    out->u.IntVal = DoInt(OpToken,Left->u.IntVal,Right->u.IntVal);
                    out->Type = INT;
                break;
                case CHAR:
                    out->u.IntVal = DoInt(OpToken,Left->u.IntVal,Right->u.CharVal);
                    out->Type = CHAR;
                break;
                case CHARPTR:  /* BAD CASE */
                    printf("Error: Type clash (INT operator CHARPTR)\n");
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case DOUBLE:
                    out->u.DoubleVal = DoDouble(OpToken,(double)Left->u.IntVal,Right->u.DoubleVal);
                    out->Type = DOUBLE;
                break;
                case XYZ:
                    DoScalarXYZ(OpToken,(double)Left->u.IntVal,Right->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case RGB:
                    DoScalarRGB(OpToken,(double)Left->u.IntVal,Right->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
            }
        break;
        case CHAR:
            switch (Right->Type)
            {
                case INT:
                    out->u.IntVal = DoInt(OpToken,(int)Left->u.CharVal,Right->u.IntVal);
                    out->Type = INT;
                break;
                case CHAR:
                    out->u.CharVal = (char) DoInt(OpToken,(int)Left->u.CharVal,(int)Right->u.CharVal);
                    out->Type = CHAR;
                break;
                case CHARPTR: /* BAD CASE */
                    printf("Error: Type clash (CHAR operator CHARPTR)\n");
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case DOUBLE:
                    out->u.DoubleVal = DoDouble(OpToken,(double)Left->u.CharVal,Right->u.DoubleVal);
                    out->Type = DOUBLE;
                break;
                case XYZ:
                    out->Type = XYZ;
                    DoScalarXYZ(OpToken,(double)Left->u.CharVal,Right->u.XYZVal,&out->u.XYZVal);
                break;
                case RGB:
                    out->Type = RGB;
                    DoScalarRGB(OpToken,(double)Left->u.CharVal,Right->u.RGBVal,&out->u.RGBVal);
                break;
            }
        break;
        case CHARPTR:
            switch (Right->Type)
            {
                case INT:
                    printf("Error: Type clash (CHARPTR %c INT)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case CHAR:
                    printf("Error: Type clash (CHARPTR %c CHAR)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case CHARPTR:
                        /* ADD STUFF */
                break;
                case DOUBLE:
                    printf("Error: Type clash (CHARPTR %c DOUBLE)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case XYZ:
                    printf("Error: Type clash (CHARPTR %c XYZ)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case RGB:
                    printf("Error: Type clash (CHARPTR %c RGB)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
            }
        break;
        case DOUBLE:
            switch (Right->Type)
            {
                case INT:
                    out->u.DoubleVal = DoDouble(OpToken,Left->u.DoubleVal,(double)Right->u.IntVal);
                    out->Type = DOUBLE;
                break;
                case CHAR:
                    out->u.DoubleVal = DoDouble(OpToken,Left->u.DoubleVal,(double)Right->u.CharVal);
                    out->Type = DOUBLE;
                break;
                case CHARPTR:
                    printf("Error: Type clash (DOUBLE %c CHARPTR)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case DOUBLE:
                    out->u.DoubleVal = DoDouble(OpToken,Left->u.DoubleVal,Right->u.DoubleVal);
                    out->Type = DOUBLE;
                break;
                case XYZ:
                    DoScalarXYZ(OpToken,Left->u.DoubleVal,Right->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case RGB:
                    DoScalarRGB(OpToken,Left->u.DoubleVal,Right->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
            }
        break;
        case XYZ:
            switch (Right->Type)
            {
                case INT:
                    DoScalarXYZ(OpToken,(double)Right->u.IntVal,Left->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case CHAR:
                    DoScalarXYZ(OpToken,(double)Right->u.CharVal,Left->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case CHARPTR:
                    printf("Error: Type clash (XYZ %c CHARPTR)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case DOUBLE:
                    DoScalarXYZ(OpToken,Right->u.DoubleVal,Left->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case XYZ:
                    DoXYZ(OpToken,Left->u.XYZVal,Right->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case RGB:
                    DoXYZ(OpToken,Left->u.XYZVal,Right->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
            }
        break;
        case RGB:
            switch (Right->Type)
            {
                case INT:
                    DoScalarRGB(OpToken,(double)Right->u.IntVal,Left->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
                case CHAR:
                    DoScalarRGB(OpToken,(double)Right->u.CharVal,Left->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
                case CHARPTR:
                    printf("Error: Type clash (RGB %c CHARPTR)\n",OpToken);
                    SAIL_ERROR_FLAG = TRUE;
                    out->Type = BAD;
                break;
                case DOUBLE:
                    DoScalarRGB(OpToken,(double)Right->u.DoubleVal,Left->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
                case XYZ:
                    DoXYZ(OpToken,Left->u.XYZVal,Right->u.XYZVal,&out->u.XYZVal);
                    out->Type = XYZ;
                break;
                case RGB:
                    DoRGB(OpToken,Left->u.RGBVal,Right->u.RGBVal,&out->u.RGBVal);
                    out->Type = RGB;
                break;
            }
        break;
    }
}


/*
================================================
    DoInt()
================================================
*/
int DoInt(char Op,int left,int right)
{
    switch(Op)
    {
        case '+':
            return(left + right);
        case '-':
            return(left - right);
        case '*':
            return(left * right);
        case '/':
            return(left / right);
        case '%':
            return(left % right);
        case '|':
            return(left | right);
        case '&':
            return(left & right);
        default:
            printf("Error: Operation (DOUBLE %c DOUBLE) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            return(0); /* KLUGE (should report back an error) */
        break;
    }
}


/*
================================================
    DoDouble()
================================================
*/
double DoDouble(char Op,double left,double right)
{
    switch(Op)
    {
        case '+':
            return(left + right);
        case '-':
            return(left - right);
        case '*':
            return(left * right);
        case '/':
            return(left / right);
        default:
            printf("Error: Operation (DOUBLE %c DOUBLE) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            return(0.0); /* KLUGE (should report back an error) */
        break;
    }
}


/*
================================================
    DoXYZ()
================================================
*/
void DoXYZ(char Op,point_type left,point_type right,point_type *t)
{
    switch(Op)
    {
        case '+':
            t->x = left.x + right.x;
            t->y = left.y + right.y;
            t->z = left.z + right.z;
        break;
        case '-':
            t->x = left.x - right.x;
            t->y = left.y - right.y;
            t->z = left.z - right.z;
        break;
        default:
            printf("Error: Operation (XYZ %c XYZ) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            /* KLUGE (should report back an error) */
            t->x = 0.0;
            t->y = 0.0;
            t->z = 0.0;
        break;
    }
}


/*
================================================
    DoRGB()
================================================
*/
void DoRGB(char Op,rgb_type left,rgb_type right,rgb_type *t)
{
    switch(Op)
    {
        case '+':
            t->r = left.r + right.r;
            t->g = left.g + right.g;
            t->b = left.b + right.b;
        break;
        case '-':
            t->r = left.r - right.r;
            t->g = left.g - right.g;
            t->b = left.b - right.b;
        break;
        default:
            printf("Error: Operation (RGB %c RGB) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            /* KLUGE (should report back an error) */
            t->r = 0.0;
            t->g = 0.0;
            t->b = 0.0;
        break;
    }
}


/*
================================================
    DoScalarXYZ()
================================================
*/
void DoScalarXYZ(char Op,double scalar,point_type vector,point_type *t)
{
    switch(Op)
    {
        case '+':
            t->x = scalar + vector.x;
            t->y = scalar + vector.y;
            t->z = scalar + vector.z;
        break;
        case '-':
            t->x = scalar - vector.x;
            t->y = scalar - vector.y;
            t->z = scalar - vector.z;
        break;
        case '*':
            t->x = scalar * vector.x;
            t->y = scalar * vector.y;
            t->z = scalar * vector.z;
        break;
        case '/':
            t->x = scalar / vector.x;
            t->y = scalar / vector.y;
            t->z = scalar / vector.z;
        break;
        default:
            printf("Error: Operation (SCALAR %c XYZ) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            /* KLUGE (should report back an error) */
            t->x = 0.0;
            t->y = 0.0;
            t->z = 0.0;
        break;
    }
}


/*
================================================
    DoScalarRGB()
================================================
*/
void DoScalarRGB(char Op,double scalar,rgb_type vector,rgb_type *t)
{
    switch(Op)
    {
        case '+':
            t->r = scalar + vector.r;
            t->g = scalar + vector.g;
            t->b = scalar + vector.b;
        break;
        case '-':
            t->r = scalar - vector.r;
            t->g = scalar - vector.g;
            t->b = scalar - vector.b;
        break;
        case '*':
            t->r = scalar * vector.r;
            t->g = scalar * vector.g;
            t->b = scalar * vector.b;
        break;
        case '/':
            t->r = scalar / vector.r;
            t->g = scalar / vector.g;
            t->b = scalar / vector.b;
        break;
        default:
            printf("Error: Operation (SCALAR %c RGB) not allowed\n",Op);
            SAIL_ERROR_FLAG = TRUE;
            /* KLUGE (should return an error) */
            t->r = 0.0;
            t->g = 0.0;
            t->b = 0.0;
        break;
    }
}


/*
================================================
    DumpModuleList()  DEBUG ROUTINE
================================================
*/
void DumpModuleList(rvalue_type *data)
{

    switch(data->Type)
    {
        case EXPRLST:
            printf("EXPRLST\n");
            DumpExpList(data->data.ExprLst);
        break;
        case MODULE:
            printf("MODULE\n");
            printf("Evaluated module found\n");
        break;
        case ASSMTS:
            printf("ASSMTS\n");
            DumpAssmtList(data->data.Assmts);
        break;
        case EMPTY:
        break;
        default:
            printf("ERROR IN DumpModuleList(), unknown type\n");
        break;
    }
}

/*
================================================
    DumpAssmtList()  DEBUG ROUTINE
================================================
*/
void DumpAssmtList(assmt_type *data)
{

    printf("lvalue = %s\n",data->lvalue);
    printf("rvalue :\n");
    DumpModuleList(&data->rvalue);
    if (data->next != NULL)
    {
        DumpAssmtList(data->next);
    }
}

/*
================================================
    DumpExpList()  DEBUG ROUTINE
================================================
*/
void DumpExpList(value_type *expr)
{
value_type *t;

    for (t = expr; t != NULL; t = t->next)
    {
        DumpExpValue(t);
    }
}


/*
================================================
    DumpExpValue()  DEBUG ROUTINE
================================================
*/

void DumpExpValue(value_type *expr)
{
    if (expr == NULL)
    {
        printf("Type: NULL\n");
    }
    else
    {
        switch(expr->Type)
        {
            case INT:
                printf("Type: INT, value = %d\n",expr->u.IntVal);
            break;
            case CHAR:
                printf("Type: CHAR, value = %c\n",expr->u.CharVal);
            break;
            case CHARPTR:
                printf("Type: CHARPTR, value = %s\n",expr->u.CharPtrVal);
            break;
            case DOUBLE:
                printf("Type: DOUBLE, value = %f\n",expr->u.DoubleVal);
            break;
            case XYZ:
                printf("Type: XYZ, value = xyz(%f,%f,%f)\n",
                   expr->u.XYZVal.x,expr->u.XYZVal.y,expr->u.XYZVal.z);
            break;
            case RGB:
                printf("Type: RGB, value = rgb(%f,%f,%f)\n",
                   expr->u.RGBVal.r,expr->u.RGBVal.g,expr->u.RGBVal.b);
            break;
            case BAD:
                printf("Type: BAD\n");
            break;
            default:
                printf("ERROR: Type Not Found\n");
            break;
        }
    }
}

