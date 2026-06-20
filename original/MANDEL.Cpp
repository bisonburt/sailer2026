/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | mandel.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The Mandelbrot generator module
              |
              |
==============================================================================
*/


#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include "mathtype.h"
#include "structs.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"

extern int SAIL_LINE_NUMBER;

static name_token_type values[] =
{
    {"startx",1},{"starty",2},{"endx",3},{"endy",4}, /* old names */
    {"left",1},{"bottom",2},{"right",3},{"top",4},  /* new names (> sail 1.0) */
    {"rep",5},{"maxcount",6},{"rangemodule",7},
    {"",-1}
};

void RunMandel(sail_module_type *,input_type *,output_type *);

/*
======================================
    BuildMandel()
======================================
*/
sail_module_type *BuildMandel(assmt_type *AssmtList)
{
sail_module_type *module;
assmt_type *t;
int token;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);

    /* default vaules for module */
    module->module.Mandel.startx    =   -2.1;
    module->module.Mandel.endx      =    0.9;
    module->module.Mandel.starty    =    1.3;
    module->module.Mandel.endy      =   -1.3;
    module->module.Mandel.rep       =    1.0;
    module->module.Mandel.maxcount  =     20;
    module->module.Mandel.rangemodule = NULL;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1:
                if (CheckValue(t,EXPRLST,DOUBLE,"mandel, startx"))
                    module->module.Mandel.startx =
                    t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 2:
                if (CheckValue(t,EXPRLST,DOUBLE,"mandel, starty"))
                    module->module.Mandel.starty =
                    t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 3:
                if (CheckValue(t,EXPRLST,DOUBLE,"mandel, endx"))
                    module->module.Mandel.endx =
                    t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 4:
                if (CheckValue(t,EXPRLST,DOUBLE,"mandel, endy"))
                    module->module.Mandel.endy =
                    t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5:
                if (CheckValue(t,EXPRLST,DOUBLE,"mandel, rep"))
                    module->module.Mandel.rep =
                    t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6:
                if (CheckValue(t,EXPRLST,INT,"mandel, maxcount"))
                    module->module.Mandel.maxcount =
                    t->rvalue.data.ExprLst->u.IntVal;
            break;
            case 7:
                if (t->rvalue.Type == MODULE)
                {
                    module->module.Mandel.rangemodule = t->rvalue.data.Module;
                }
                else
                {
                    printf("Type error in module mandel, ");
                    printf("rangemodule expects a module\n");;
                }
            break;
            default:
                printf("[mandel, %d] bad lvalue: %s\n",SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    module->funct = RunMandel;
    return(module);
}


/*
======================================
    RunMandel()
======================================
*/
void RunMandel(sail_module_type *module,input_type *in,output_type *out)
{
double x,y;
double cx,cy; /* current working value of mandelbrot we are computing */
double zx,zy; /* z value in mandelbrot calculation */
double size;
int count;
double tx,ty;
double rangep;
int maxcnt;
Mandel_type *data;

    data = &module->module.Mandel;

    /* normalized values of area to map mandelbrot function */
    x = frac(in->hitpoint.x);   /* 0 < x < 1 */
    y = frac(in->hitpoint.y);   /* 0 < y < 1 */

    cx =  data->startx + x*(data->endx - data->startx);
    cy =  data->starty + y*(data->endy - data->starty);

    zx = 0.0;
    zy = 0.0;
    size = 0.0;
    count = 0;
    maxcnt = data->maxcount;
    tx = 0.0;
    ty = 0.0;

    /* compute the count of iterations for mandelbrot set  */

    while ((size < 4.0) && (count < maxcnt))
    {
        double t;
        /* z = z*z + c */
        t = zx;
        zx= tx - ty;        /* z*z (real) */
        zy= 2.0*(t*zy);     /* z*z (imag) */
        zx+=cx;             /* +c (real) */
        zy+=cy;             /* +c (imag) */
        count++;
        tx = zx*zx;
        ty = zy*zy;
        size = tx+ty;
    }

    if (count == maxcnt)
    {
        rangep = 0.0;
    }
    else
    {
        rangep = frac(data->rep*(double)count/(double)(maxcnt-1)); /* 0<x<1 */
    }

    {
        input_type Nin;
        Nin.hitpoint.x = rangep;
        Nin.hitpoint.y = rangep;
        Nin.hitpoint.z = rangep;
        data->rangemodule->funct(data->rangemodule,&Nin,out);
    }
    return;
}
