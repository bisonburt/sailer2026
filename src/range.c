/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | range.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The range module used in SAIL
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
#include "parse_tr.h"
#include "sail.h"
#include "support.h"
#include "tokens.h"

extern int SAIL_LINE_NUMBER;

static void RunRange(sail_module_type *,input_type *,output_type *);
static void DoModule(int, int, assmt_type *,sail_module_type *,int);
static void InterpSurface(double,output_type *,output_type *,output_type *);
static void InitOutput(output_type *);


static name_token_type values[] =
{
    {"solid",1},{"fade",2},{"rangevalue",3},{"rep",4},{"default",5},
    {"start",10},{"stop",11},
    {"value",20},{"stvalue",21},{"spvalue",22},
    {"flags",30},
    {"XVALUE",40},{"YVALUE",41},{"ZVALUE",42},{"local",43},{"global",44},
    {"",-1}
};


/*
======================================
    BuildRange()
======================================
*/
sail_module_type *BuildRange(assmt_type *AssmtList)
{
sail_module_type *module;
assmt_type *t;
int token;
int cnt=0;
range_type *crangel;
RangeP_type *crange;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);

    /* defaults for some values if not set */
    module->module.Range.rangevalue = 1;
    module->module.Range.rep = 0.0; /* default (off) */

    for (t = AssmtList; t != NULL; t = t->next)
    {
        /* to save space below in assignments */
        crangel = &module->module.Range.RangeList[cnt];
        crange = &module->module.Range;

        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* solid */
                crangel->Type = SOLID;
                DoModule(token,cnt++,t,module,0);
            break;
            case 2: /* fade */
                crangel->Type = FADE;
                DoModule(token,cnt++,t,module,1);
            break;
            case 3: /* rangevalue */
                if (t->rvalue.Type == EXPRLST)
                {
                    switch(GetToken(t->rvalue.data.ExprLst->u.CharPtrVal,values))
                    {
                        case 40: /* XVALUE */
                            crange->rangevalue = 1;
                        break;
                        case 41: /* YVALUE */
                            crange->rangevalue = 2;
                        break;
                        case 42: /* ZVALUE */
                            crange->rangevalue = 3;
                        break;
                        default:
                            printf("Bad flag for rangevalue in crange module\n");
                        break;
                    }
                }
                else
                {
                    printf("Error in crange, Expected an EXPRLST type for rangevalue\n");
                    SAIL_ERROR_FLAG = TRUE;
                }
            break;
            case 4: /* rep */
                if (CheckValue(t,EXPRLST,DOUBLE,"range, rep"))
                {
                    crange->rep = t->rvalue.data.ExprLst->u.DoubleVal;
                }
            break;
            case 20: /* value */
                if (t->rvalue.Type == MODULE)
                {
                    crange->defaultv = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"range, value"))
                {
                    crange->defaultv =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            default:
                printf("[range, %d] bad lvalue: %s\n",SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    module->module.Range.cnt = cnt;
    module->funct = RunRange;
    return(module);
}


static void DoModule(int tag,int count,assmt_type *t,sail_module_type *m, int type)
{
assmt_type *t1;
range_type *crangel;

    crangel = &m->module.Range.RangeList[count];

    if (t->rvalue.Type == ASSMTS)
    {
        for (t1 = t->rvalue.data.Assmts; t1 != NULL; t1 = t1->next)
        {
            switch(GetToken(t1->lvalue,values))
            {
                case 10: /* start */
                    if (CheckValue(t1,EXPRLST,DOUBLE,"range, start"))
                    {
                        crangel->start = t1->rvalue.data.ExprLst->u.DoubleVal;
                    }
                break;
                case 11: /* stop */
                    if (CheckValue(t1,EXPRLST,DOUBLE,"range, stop"))
                    {
                        crangel->stop = t1->rvalue.data.ExprLst->u.DoubleVal;
                    }
                break;
                case 20: /* value */
                    if (t1->rvalue.Type == MODULE)
                    {
                        crangel->startv = t1->rvalue.data.Module;
                    }
                    else if (CheckValue(t1,EXPRLST,CHARPTR,"range, value"))
                    {
                        crangel->startv =
                            InitTexture(t1->rvalue.data.ExprLst->u.CharPtrVal);
                    }
                break;
                case 21: /* stvalue */
                    if (t1->rvalue.Type == MODULE)
                    {
                        crangel->startv = t1->rvalue.data.Module;
                    }
                    else if (CheckValue(t1,EXPRLST,CHARPTR,"range, stvalue"))
                    {
                        crangel->startv =
                            InitTexture(t1->rvalue.data.ExprLst->u.CharPtrVal);
                    }
                break;
                case 22: /* spvalue */
                    if (t1->rvalue.Type == MODULE)
                    {
                        crangel->stopv = t1->rvalue.data.Module;
                    }
                    else if (CheckValue(t1,EXPRLST,CHARPTR,"range, spvalue"))
                    {
                        crangel->stopv =
                            InitTexture(t1->rvalue.data.ExprLst->u.CharPtrVal);
                    }
                break;
                case 30: /* flags */
                    if (t1->rvalue.Type == EXPRLST)
                    {
                        int v;
                        v = GetToken(t1->rvalue.data.ExprLst->u.CharPtrVal,values);
                        if (v == 43)
                        {
                            crangel->flags |= 0x1;
                        }
                        else
                        {
                            printf("[range %d] invalid flag, %s, used in out \n",
                                SAIL_LINE_NUMBER,t1->rvalue.data.ExprLst->u.CharPtrVal);
                            SAIL_ERROR_FLAG = TRUE;
                        }
                    }
                    else
                    {
                        printf("[crange %d] Expected an EXPRLST type for flags\n",SAIL_LINE_NUMBER);
                        SAIL_ERROR_FLAG = TRUE;
                    }
                break;
                default:
                    printf("[range.%s, %d] bad lvalue: %s\n",
                        (type ? "solid" : "fade"),SAIL_LINE_NUMBER,t1->lvalue);
                    SAIL_ERROR_FLAG = TRUE;
                break;
            }
        }
    }
}

/*
======================================
    RunRange()
======================================
*/
static void RunRange(sail_module_type *module,input_type *in,output_type *out)
{
double range;
RangeP_type *data;
int found=0;
int i;

    data = &module->module.Range;

    switch(data->rangevalue) /* get a local copy of rangevalue */
    {
        case 1: /* XVALUE */
            range = in->hitpoint.x;
        break;
        case 2: /* YVALUE */
            range = in->hitpoint.y;
        break;
        case 3: /* ZVALUE */
            range = in->hitpoint.z;
        break;
    }

    if (data->rep > 0.0)
    {
        range = frac((range * data->rep));
    }

    for(i=0;i < data->cnt;i++)
    {
        range_type *crangel;
        crangel = &data->RangeList[i];

        if (crangel->start <= range && range < crangel->stop)
        {
            double coef;

            found = 1;
            coef = (range - crangel->start)/(crangel->stop - crangel->start);

            switch(crangel->Type)
            {
                case SOLID:
                {
                    input_type Nin = *in;
                    /* if local bit set, localize proper input value */
                    if (crangel->flags & 0x1)
                    {
                        switch(data->rangevalue)
                        {
                            case 1: /* XVALUE */
                                Nin.hitpoint.x = coef;
                            break;
                            case 2: /* YVALUE */
                                Nin.hitpoint.y = coef;
                            break;
                            case 3: /* ZVALUE */
                                Nin.hitpoint.z = coef;
                            break;
                        }
                    }
                    crangel->startv->funct
                    (crangel->startv,&Nin,out);
                }
                break;
                case FADE:
                {
                    input_type Nin; /* temp input */
                    output_type StartOut,StopOut;

                    Nin = *in;

                    /* if local bit set, localize proper input value */
                    if (crangel->flags & 0x1)
                    {
                        switch(data->rangevalue)
                        {
                            case 1: /* XVALUE */
                                Nin.hitpoint.x = coef;
                            break;
                            case 2: /* YVALUE */
                                Nin.hitpoint.y = coef;
                            break;
                            case 3: /* ZVALUE */
                                Nin.hitpoint.z = coef;
                            break;
                        }
                    }

                    InitOutput(&StartOut);
                    crangel->startv->funct(crangel->startv,&Nin,&StartOut);

                    InitOutput(&StopOut);
                    crangel->stopv->funct(crangel->stopv,&Nin,&StopOut);

                    InterpSurface(coef,&StartOut,&StopOut,out);
                }
                break;
            } /* switch */
        } /* if */
    } /* for */

    if (found == 0)
    {
        /* Some ranges (e.g. a pure fade gradient) define no default value.
           Only invoke the default when one exists; otherwise leave 'out' at
           its initialized value rather than dereferencing a NULL module. */
        if (data->defaultv != NULL)
        {
            input_type Nin = *in;
            data->defaultv->funct(data->defaultv,&Nin,out);
        }
    }
    return;
}

static void InterpSurface(double coef,
    output_type *start, output_type *stop, output_type *out)
{
    /* interp between start and stop */
    /* note: OO design would move the following code to attribute object */

    out->color.r = start->color.r +(coef)*(stop->color.r - start->color.r);
    out->color.g = start->color.g +(coef)*(stop->color.g - start->color.g);
    out->color.b = start->color.b +(coef)*(stop->color.b - start->color.b);

    out->reflect.r = start->reflect.r +
        (coef)*(stop->reflect.r - start->reflect.r);
    out->reflect.g = start->reflect.g +
        (coef)*(stop->reflect.g - start->reflect.g);
    out->reflect.b = start->reflect.b +
        (coef)*(stop->reflect.b - start->reflect.b);

    out->trans.r = start->trans.r +(coef)*(stop->trans.r - start->trans.r);
    out->trans.g = start->trans.g +(coef)*(stop->trans.g - start->trans.g);
    out->trans.b = start->trans.b +(coef)*(stop->trans.b - start->trans.b);

    out->highlight = start->highlight+(coef)*(stop->highlight-start->highlight);
}

static void InitOutput(output_type *o)
{
    o->color.r = 1.0;
    o->color.g = 1.0;
    o->color.b = 1.0;

    o->reflect.r = 1.0;
    o->reflect.g = 1.0;
    o->reflect.b = 1.0;

    o->trans.r = 0.0;
    o->trans.g = 0.0;
    o->trans.b = 0.0;

    o->newnormal.x = 0.0;
    o->newnormal.y = 0.0;
    o->newnormal.z = 1.0;

    o->highlight = 0.0;
}