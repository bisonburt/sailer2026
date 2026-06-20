/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | attribute.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The attribute module use in SAIL
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

static void RunAttrib(sail_module_type *,input_type *,output_type *);

static name_token_type values[] =
{
    {"color",1},{"refl",2},{"trans",3},{"normal",4},{"highlt",5},
    {"",-1}
};


/*
======================================
    BuildAttrib()
======================================
*/
sail_module_type *BuildAttrib(assmt_type *AssmtList)
{
sail_module_type *module;
assmt_type *t;
int token;
Attrib_type *attrib;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);
    /* to save space below in assignments */
    attrib = &module->module.Attrib;

    /* defaults for some values if not set */
    attrib->value.color.r = 1.0;
    attrib->value.color.g = 1.0;
    attrib->value.color.b = 1.0;

    attrib->value.reflect.r = 1.0;
    attrib->value.reflect.g = 1.0;
    attrib->value.reflect.b = 1.0;

    attrib->value.trans.r = 0.0;
    attrib->value.trans.g = 0.0;
    attrib->value.trans.b = 0.0;

    attrib->value.newnormal.x = 0.0;
    attrib->value.newnormal.y = 0.0;
    attrib->value.newnormal.z = 1.0;

    attrib->value.highlight = 0.0;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            /* default attribute values */
            case 1: /* color */
                if (CheckValue(t,EXPRLST,RGB,"attribute, color"))
                {
                    attrib->value.color = t->rvalue.data.ExprLst->u.RGBVal;
                }
            break;
            case 2: /* refl */
                if (CheckValue(t,EXPRLST,RGB,"attribute, refl"))
                {
                    attrib->value.reflect = t->rvalue.data.ExprLst->u.RGBVal;
                }
            break;
            case 3: /* trans */
                if (CheckValue(t,EXPRLST,RGB,"attribute, trans"))
                {
                    attrib->value.trans = t->rvalue.data.ExprLst->u.RGBVal;
                }
            break;
            case 4: /* normal */
                if (CheckValue(t,EXPRLST,XYZ,"attribute, normal"))
                {
                    attrib->value.newnormal = t->rvalue.data.ExprLst->u.XYZVal;
                }
            break;
            case 5: /* highlt */

                if (CheckValue(t,EXPRLST,DOUBLE,"attribute, highlt"))
                {
                    attrib->value.highlight = t->rvalue.data.ExprLst->u.DoubleVal;

                }
            break;
            default:
                printf("[attribute, %d] bad lvalue: %s\n",SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    module->funct = RunAttrib;
    return(module);
}


/*
======================================
    RunAttrib()
======================================
*/
static void RunAttrib(sail_module_type *module,input_type *in,output_type *out)
{
    *out = module->module.Attrib.value;
    return;
}
