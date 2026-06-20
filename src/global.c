/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | global.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The global module that is used in the ray tracer
              |
              |
==============================================================================
*/


#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "mathtype.h"
#include "structs.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"

static rgb_type ambient,lightcolor;
static double ambcoef,bkgrndsize;
static point_type lightsrc;
static struct sail_module_struct *module;
static int maxdepth;

static name_token_type values[] =
{
    {"ambient",1},{"ambcoef",2},{"lightsrc",3},{"lightcolor",4},
    {"bkgrndsize",5},{"bkgrndsurface",6},{"maxdepth",7},{"",-1}
};

/*
======================================
    BuildGlobal()
======================================
*/
void BuildGlobal(assmt_type *AssmtList)
{
assmt_type *t;
int token;

    maxdepth = 5;
    bkgrndsize = 100.0;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* ambient */
                if (CheckValue(t,EXPRLST,RGB,"global, ambient"))
                    ambient = t->rvalue.data.ExprLst->u.RGBVal;
            break;
            case 2: /* ambcoef */
                if (CheckValue(t,EXPRLST,DOUBLE,"global, ambcoef"))
                    ambcoef = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 3: /* lightsrc */
                if (CheckValue(t,EXPRLST,XYZ,"global, lightsrc"))
                    lightsrc = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* lightcolor */
                if (CheckValue(t,EXPRLST,RGB,"global, lightcolor"))
                    lightcolor = t->rvalue.data.ExprLst->u.RGBVal;
            break;
            case 5: /* bkgrndsize */
                if (CheckValue(t,EXPRLST,DOUBLE,"global, bkgrndsize"))
                    bkgrndsize = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* bkgrndsurface */
                if (t->rvalue.Type == MODULE)
                {
                    module = t->rvalue.data.Module;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"global, bkgrndsurface"))
                {
                    module =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                }
            break;
            case 7: /* maxdepth */
                if (CheckValue(t,EXPRLST,INT,"global, maxdepth"))
                    maxdepth = t->rvalue.data.ExprLst->u.IntVal;
            break;
            default:
                printf("Unknown lvalue (%s) found in a global module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    return;
}

void GetLightingInfo(rgb_type * amb,double *ambc,point_type *light,rgb_type *color)
{
    *amb = ambient;
    *ambc = ambcoef;
    *light = lightsrc;
    *color = lightcolor;
}

void GetBackgroundInfo(double *size,struct sail_module_struct **mod)
{
    *size = bkgrndsize;
    *mod = module;
}

void GetMaxDepth(int *md)
{
    *md = maxdepth;
}
