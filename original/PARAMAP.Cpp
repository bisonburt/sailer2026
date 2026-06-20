/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | paramap.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The paramap module used by SAIL
              |
              |
==============================================================================
*/


/*
===============================================================
   Declarations for: Para_type.
===============================================================
*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "mathtype.h"
#include "structs.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"
#include "tokens.h"

extern int SAIL_LINE_NUMBER;

/* local decls */
static void BuildMap(assmt_type *data,map_type *map);
void RunParaMap(sail_module_type *,input_type *,output_type *);

static name_token_type values[] =
{
    {"axis",1},{"mapx",2},{"mapy",3},{"mapz",4},{"value",5},
    {"type",10},{"min",11},{"max",12},{"rep",13},{"lattice",14},
    {"OFF",20},
    {"PLANARX",21},{"PLANARY",22},{"PLANARZ",23},
    {"RADIALX",24},{"RADIALY",25},{"RADIALZ",26},
    {"ANGULARX",27},{"ANGULARY",28},{"ANGULARZ",29},
    {"DISTANCE",30},{"NOISE",31},{"CONSTANT",32},{"FNOISE",33},
    {"",-1}
};


/*
======================================
    BuildParaMap()
======================================
*/

sail_module_type *BuildParaMap(assmt_type *AssmtList)
{
sail_module_type *module;
Para_type *this;
assmt_type *t;
int token;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);
    this = &module->module.ParaMap;

    /* defaults */
    this->BBox = NULL;
    this->module = NULL;

    this->mapx.Type = PLANARX;
    this->mapx.min = 0.0;
    this->mapx.max = 1.0;
    this->mapx.rep = 1.0;
    this->mapx.lattice.x = 1.0;
    this->mapx.lattice.y = 1.0;
    this->mapx.lattice.z = 1.0;

    this->mapx.Type = PLANARY;
    this->mapy.min = 0.0;
    this->mapy.max = 1.0;
    this->mapy.rep = 1.0;
    this->mapy.lattice.x = 1.0;
    this->mapy.lattice.y = 1.0;
    this->mapy.lattice.z = 1.0;

    this->mapx.Type = PLANARZ;
    this->mapz.min = 0.0;
    this->mapz.max = 1.0;
    this->mapz.rep = 1.0;
    this->mapz.lattice.x = 1.0;
    this->mapz.lattice.y = 1.0;
    this->mapz.lattice.z = 1.0;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* axis */
                if (t->rvalue.Type == MODULE)
                {
                    this->BBox = &t->rvalue.data.Module->module.bbox;
                    break;
                }
                printf("[paramap %d], Expected a MODULE type for axis\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            case 2: /* mapx */
                if (t->rvalue.Type == ASSMTS)
                {
                    BuildMap(t->rvalue.data.Assmts,&this->mapx);
                    break;
                }
                printf("[paramap %d], Expected an ASSMTS type for mapx\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            case 3: /* mapy */
                if (t->rvalue.Type == ASSMTS)
                {
                    BuildMap(t->rvalue.data.Assmts,&this->mapy);
                    break;
                }
                printf("[paramap %d], Expected an ASSMTS type for mapy\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            case 4: /* mapz */
                if (t->rvalue.Type == ASSMTS)
                {
                    BuildMap(t->rvalue.data.Assmts,&this->mapz);
                    break;
                }
                printf("[paramap %d], Expected an ASSMTS type for mapz\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            case 5: /* value */
                if (t->rvalue.Type == MODULE)
                {
                    this->module = t->rvalue.data.Module;
                    break;
                }
                else if (CheckValue(t,EXPRLST,CHARPTR,"paramap, value"))
                {
                    this->module =
                        InitTexture(t->rvalue.data.ExprLst->u.CharPtrVal);
                    break;
                }
                printf("[paramap %d], Expected a MODULE type for value\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            default:
                /* error */
                printf("[paramap, %d] bad lvalue: %s\n",
                SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    module->funct = RunParaMap;
    return(module);
}

static void BuildMap(assmt_type *data,map_type *map)
{
assmt_type *t;

    for (t = data; t != NULL; t = t->next)
    {
        switch(GetToken(t->lvalue,values))
        {
            case 10: /* type */
                if (t->rvalue.Type == EXPRLST)
                {
                    if (t->rvalue.data.ExprLst->Type == CHARPTR)
                    {
                        switch(GetToken(t->rvalue.data.ExprLst->u.CharPtrVal,values))
                        {
                            case 20: /* OFF */
                                map->Type = OFF;
                            break;
                            case 21: /* PLANARX */
                                map->Type = PLANARX;
                            break;
                            case 22: /* PLANARY */
                                map->Type = PLANARY;
                            break;
                            case 23: /* PLANERZ */
                                map->Type = PLANARZ;
                            break;
                            case 24: /* RADIALX */
                                map->Type = RADIALX;
                            break;
                            case 25: /* RADIALY */
                                map->Type = RADIALY;
                            break;
                            case 26: /* RADIALZ */
                                map->Type = RADIALZ;
                            break;
                            case 27: /* ANGULARX */
                                map->Type = ANGULARX;
                            break;
                            case 28: /* ANGULARY */
                                map->Type = ANGULARY;
                            break;
                            case 29: /* ANGULARZ */
                                map->Type = ANGULARZ;
                            break;
                            case 30: /* DISTANCE */
                                map->Type = DISTANCE;
                            break;
                            case 31: /* NOISE */
                                map->Type = NOISE;
                            break;
                            case 32: /* CONSTANT */
                                map->Type = CONSTANT;
                            break;
                            case 33: /* FNOISE */
                                map->Type = FNOISE;
                            break;
                            default:
                                printf("[paramap %d], unknown flag assigned to type\n",
                                SAIL_LINE_NUMBER);
                                SAIL_ERROR_FLAG = TRUE;
                            break;
                        }
                        break;
                    }
                    printf("[paramap %d], Expected an DOUBLE value for min\n",
                    SAIL_LINE_NUMBER);
                    SAIL_ERROR_FLAG = TRUE;
                    break;
                }
                printf("[paramap %d], Expected an EXPRLST type for min\n",
                SAIL_LINE_NUMBER);
                SAIL_ERROR_FLAG = TRUE;
            break;
            case 11: /* min */
                if (CheckValue(t,EXPRLST,DOUBLE,"paramap, rep"))
                map->min = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 12: /* max */
                if (CheckValue(t,EXPRLST,DOUBLE,"paramap, rep"))
                map->max = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 13: /* rep */
                if (CheckValue(t,EXPRLST,DOUBLE,"paramap, rep"))
                map->rep = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 14: /* lattice */
                if (CheckValue(t,EXPRLST,XYZ,"paramap, lattice"))
                map->lattice = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            default:
                printf("[paramap %d], invalid lvalue (%s)\n",
                SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
}


/*
======================================
    RunParaMap()
======================================
*/

void RunParaMap(sail_module_type *module,input_type *in,output_type *out)
{
rgb_type value;
Para_type *data;
point_type local;
double temp;
int i;
enum pm_type pm;
double angle;


    data = &module->module.ParaMap;

    /* void RunBBox(Bound_type *box,input_type *in,point_type *local) */
    if (data->BBox)
        RunBBox(data->BBox,in,&local);
    else
        local = in->hitpoint;

    for (i=0; i<3; i++)
    {
        switch(i)
        {
            case 0: pm = data->mapx.Type; break;
            case 1: pm = data->mapy.Type; break;
            case 2: pm = data->mapz.Type; break;
        }
        switch (pm)
        {
            case OFF: /* none */
                temp = 0;
            break;
            case PLANARX:
                temp = local.x;
            break;
            case PLANARY:
                temp = local.y;
            break;
            case PLANARZ:
                temp = local.z;
            break;
            case RADIALX:
                temp = sqrt(local.y*local.y+local.z*local.z) /* /SQRT2 */;
            break;
            case RADIALY:
                temp = sqrt(local.x*local.x+local.z*local.z)/* /SQRT2 */;
            break;
            case RADIALZ:
                temp = sqrt(local.x*local.x+local.z*local.z)/* /SQRT2 */;
            break;
            case ANGULARX:
                if (fabs(local.y) > 0.0000001)
                {
                    angle = atan(local.z/local.y);
                }
                else
                {
                    angle = PIOVER2;
                }
                if (local.y < 0.0) angle += PI;
                if (angle < 0.0) angle += PITIMES2;
                temp = angle/PITIMES2;
            break;
            case ANGULARY:
                if (fabs(local.z) > 0.0000001)
                {
                    angle = atan(local.x/local.z);
                }
                else
                {
                    angle = PIOVER2;
                }
                if (local.z < 0.0) angle += PI;
                if (angle < 0.0) angle += PITIMES2;
                temp = angle/PITIMES2;
            break;
            case ANGULARZ:
                if (fabs(local.y) > 0.0000001)
                {
                    angle = atan(local.x/local.y);
                }
                else
                {
                    angle = PIOVER2;
                }
                if (local.y < 0.0) angle += PI;
                if (angle < 0.0) angle += PITIMES2;
                temp = angle/PITIMES2;
            break;
            case DISTANCE:
                temp = sqrt(local.x*local.x+local.y*local.y+local.z*local.z)/SQRT3;
            break;
            case NOISE:
                {
                    point_type t;
                    switch(i)
                    {
                        case 0: t = data->mapx.lattice; break;
                        case 1: t = data->mapy.lattice; break;
                        case 2: t = data->mapz.lattice; break;
                    }
                    temp = Noise(local.x*t.x,local.y*t.y,local.z*t.z,i);
                }
            break;
            case CONSTANT: /* constant */
                switch(i)
                {
                    case 0: temp = data->mapx.max; break;
                    case 1: temp = data->mapy.max; break;
                    case 2: temp = data->mapz.max; break;
                }
            break;
            case FNOISE:
                {
                    point_type t;
                    switch(i)
                    {
                        case 0: t = data->mapx.lattice; break;
                        case 1: t = data->mapy.lattice; break;
                        case 2: t = data->mapz.lattice; break;
                    }
                    temp = FNoise(local.x*t.x,local.y*t.y,local.z*t.z,i);
                }
            break;
            default: /* this should not happen */
                printf("DEBUG: RunParaMap() default case in switch, something wrong\n");
                temp = 0.0;
            break;
        } /* switch */

        switch(i)
        {
            case 0: value.r = temp; break;
            case 1: value.g = temp; break;
            case 2: value.b = temp; break;
        }
    } /* for */


    /* min/max/rep done here */
    /* to add later */

    if(data->module == NULL)
    {
        out->color = value;
        out->reflect = value;
    }
    else
    {
        input_type t_in = *in;

        /* copy info into temp input data */
        t_in.hitpoint.x = value.r;
        t_in.hitpoint.y = value.g;
        t_in.hitpoint.z = value.b;

        t_in.normal = out->newnormal;

        /* call the module here */
        data->module->funct(data->module,&t_in,out);
    }
    return;
}
