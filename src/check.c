/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | check.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The check module used in SAIL
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

static name_token_type values[] =
{
    {"rep",1},{"even",2},{"odd",3},{"flags",4},
    {"localodd",5},{"localeven",6},{"globalodd",7},{"globaleven",8},{"",-1}
};

extern int SAIL_LINE_NUMBER;
static void RunCheck(sail_module_type *,input_type *,output_type *);


/*
======================================
    BuildCheck()
======================================
*/
sail_module_type *BuildCheck(assmt_type *AssmtList)
{
sail_module_type *module;
assmt_type *t;
value_type *t1;
int token;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);

    /* defaults */
    module->module.Check.rep.x = 2.0;
    module->module.Check.rep.y = 2.0;
    module->module.Check.rep.z = 2.0;
    module->module.Check.flags = 0x0;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* rep */
                if (CheckValue(t,EXPRLST,XYZ,"check, rep"))
                module->module.Check.rep = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* even */
                if (t->rvalue.Type == MODULE)
                {
                    module->module.Check.even = t->rvalue.data.Module;
                    break;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"check, value"))
                {
                    module->module.Check.even =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                    break;
                }
                SAIL_ERROR_FLAG = TRUE;
                printf("Error in check, expecting a module type for even\n");
            break;
            case 3: /* odd */
                if (t->rvalue.Type == MODULE)
                {
                    module->module.Check.odd = t->rvalue.data.Module;
                    break;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"check, value"))
                {
                     module->module.Check.odd =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                    break;
                }
                SAIL_ERROR_FLAG = TRUE;
                printf("Error in check, expecting a module type for odd\n");
            break;
            case 4: /* flags */
                if (t->rvalue.Type == EXPRLST)
                {
                    for (t1 = t->rvalue.data.ExprLst; t1 != NULL; t1 = t1->next)
                    {
                        switch(token = GetToken(t1->u.CharPtrVal,values))
                        {
                            case 5: /* localodd */
                                module->module.Check.flags &= 0xfffffffe;
                            break;
                            case 6: /* localeven */
                                module->module.Check.flags &= 0xfffffffd;
                            break;
                            case 7: /* gloablodd */
                                module->module.Check.flags |= 0x1;
                            break;
                            case 8: /* globaleven */
                                module->module.Check.flags |= 0x2;
                            break;
                            default:
                                printf("Error in check, bad flag\n");
                                SAIL_ERROR_FLAG = TRUE;
                            break;
                        }
                    }
                    break;
                }
                printf("Error in check, Expected an EXPRLST type for flags\n");
                SAIL_ERROR_FLAG = TRUE;
            break;
            default:
                printf("[check, %d] bad lvalue: %s\n",SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }

    if (!module->module.Check.even) UnassignedErr("even in module check");
    if (!module->module.Check.odd) UnassignedErr("odd in module check");

    module->funct = RunCheck;
    return(module);
}


/*
======================================
    RunCheck()
======================================
*/
static void RunCheck(sail_module_type *module,input_type *in,output_type *out)
{
double rx,ry,rz;
int x,y,z;
Check_type *data;

    data = &module->module.Check;

    rx = frac(data->rep.x*in->hitpoint.x);
    ry = frac(data->rep.y*in->hitpoint.y);
    rz = frac(data->rep.z*in->hitpoint.z);

    x = (int)(floor(data->rep.x*in->hitpoint.x));
    y = (int)(floor(data->rep.y*in->hitpoint.y));
    z = (int)(floor(data->rep.z*in->hitpoint.z));

    if((x^y^z) & 0x1)
    {
        if(data->flags & 0x1) /* global */
        {
             data->odd->funct(data->odd,in,out);
             return;
        }
        else
        {
            input_type Nin = *in;
            Nin.hitpoint.x = rx;
            Nin.hitpoint.y = ry;
            Nin.hitpoint.z = rz;
            data->odd->funct(data->odd,&Nin,out);
            return;
        }
    }
    else
    {
        if(data->flags & 0x2)
        {
            data->even->funct(data->even,in,out);
            return;
        }
        else
        {
            input_type Nin = *in;
            Nin.hitpoint.x = rx;
            Nin.hitpoint.y = ry;
            Nin.hitpoint.z = rz;
            data->even->funct(data->even,&Nin,out);
            return;
        }
    }
    return;
}

