/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | sail.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | This file is mainly the go-between for the parser and the
              | module creation & alocation. It also includes the bbox module
              | which can be used by many modules.
              |
==============================================================================
*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "structs.h"
#include "support.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "symtab.h"

int SAIL_ERROR_FLAG=FALSE;

/* private declarations */
static void InitBBoxMatrix(Bound_type *);

#define NEW_SYMBOLTAB

#ifdef NEW_SYMBOLTAB
static symtab_type *GlobalSymtab = NULL;
static symtabEntry tempEntry;
#else
static sail_module_type *ListHeader=NULL; /* pointer to all public sail modules */
#endif


/*
=====================================================
    NewSailModule()
    ADD_BELOW_FOR_MODULE
=====================================================
*/

sail_module_type *NewSailModule(int token,char *name,assmt_type *AssmtList)
{
sail_module_type *t;
    switch(token)
    {
        case YPARAMAP:
            t = BuildParaMap(AssmtList);
        break;
        case YBBOX:
            t = BuildBBox(AssmtList);
        break;
        case YCHECK:
            t = BuildCheck(AssmtList);
        break;
        case YRANGE:
            t = BuildRange(AssmtList);
        break;
        case YBITMAP:
            t = BuildBitmap(AssmtList);
        break;
        case YMANDEL:
            t = BuildMandel(AssmtList);
        break;
        case YATTRIBUTE:
            t = BuildAttrib(AssmtList);
        break;
/*
    the following type cast are a kluge
    the symbol table needs to have a class type added so
    it can distinguish surface modules from intersection modules.
*/

        case YSPHERE:
            t = (sail_module_type *)BuildSphere(AssmtList);
        break;
        case YCONIC:
            t = (sail_module_type *)BuildConic(AssmtList);
        break;
        case YBOX:
            t = (sail_module_type *)BuildBox(AssmtList);
        break;
        case YTRIANGLE:
            t = (sail_module_type *)BuildTriangle(AssmtList);
        break;
        case YBOARD:
            t = (sail_module_type *)BuildBoard(AssmtList);
        break;
        case YCSG:
            t = (sail_module_type *)BuildCSG(AssmtList);
        break;
        case YVIEW:
            BuildView(AssmtList);
        break;
        case YGLOBAL:
            BuildGlobal(AssmtList);
        break;
        default:
            return(NULL);
        break;
    }
    if (name != NULL)
    {

#ifdef NEW_SYMBOLTAB
        strncpy(tempEntry.name,name,31);
        tempEntry.info = (void *)t;
        tempEntry.freemem_funct = NULL;
        tempEntry.class = t->class;
        GlobalSymtab = AddEntry(GlobalSymtab,&tempEntry);
#else
        t->name = name;
        t->next = ListHeader;
        ListHeader = t;
#endif
        printf("module: %s %x\n",name,t);
    }
    return(t);
}


/*
=====================================================
    FindModule()
=====================================================
*/
sail_module_type *FindModule(char *name)
{
#ifdef NEW_SYMBOLTAB
symtabEntry *tempEntryPtr;
    if ((tempEntryPtr = FindEntryByName(GlobalSymtab,name)) != NULL)
    {
        return((sail_module_type *)tempEntryPtr->info);
    }
    else
    {
        printf("Error, module %s not found\n",name);
        SAIL_ERROR_FLAG = TRUE;
        return (NULL);
    }
#else
sail_module_type *temp=ListHeader;

    while (temp != NULL)
    {
        if (strcmp(temp->name,name) == 0)
        {
            return(temp);
        }
        temp = temp->next;
    }
    printf("Error, module %s not found\n",name);
    SAIL_ERROR_FLAG = TRUE;
    return(NULL);
#endif
}

/*
======================================
    BuildBBox()
======================================
*/

sail_module_type *BuildBBox(assmt_type *AssmtList)
{
sail_module_type *module;
assmt_type *t;
value_type *t1;
int token;

    static name_token_type values[] =
    {
        {"loc",1},{"size",2},{"rot",3},{"flags",4},
        {"WIDEX",10},{"WIDEY",11},{"WIDEZ",12},{"WIDEALL",13},{"NONE",14},
        {"GLOBAL",20},
        {"",-1}
    };

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);

    module->module.bbox.flags |= 0x0;


    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* loc */
                if (CheckValue(t,EXPRLST,XYZ,"bbox, loc"))
                module->module.bbox.loc = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* size */
                if (CheckValue(t,EXPRLST,XYZ,"bbox, size"))
                module->module.bbox.size = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* rot */
                if (CheckValue(t,EXPRLST,XYZ,"bbox, rot"))
                module->module.bbox.rot = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* flags */
                if (t->rvalue.Type == EXPRLST)
                {
                    for (t1 = t->rvalue.data.ExprLst; t1 != NULL; t1 = t1->next)
                    {
                        switch(token = GetToken(t1->u.CharPtrVal,values))
                        {
                            case 10: /* WIDEX */
                                module->module.bbox.flags |= 0x1;
                            break;
                            case 11: /* WIDEY */
                                module->module.bbox.flags |= 0x2;
                            break;
                            case 12: /* WIDEZ */
                                module->module.bbox.flags |= 0x4;
                            break;
                            case 13: /* WIDEALL */
                                module->module.bbox.flags |= 0x7;
                            break;
                            case 14: /* NONE */
                                module->module.bbox.flags = 0x0;
                            break;
                            case 20: /* GLOBAL */
                                module->module.bbox.flags |= 0x8;
                            break;
                        }
                    }
                    break;
                }
                printf("Error in bbox, Expected an EXPRLST type for flags\n");
                SAIL_ERROR_FLAG = TRUE;
            break;

            default:
                printf("Unknown lvalue (%s) found in a bbox module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    InitBBoxMatrix(&module->module.bbox);
    return(module);
}

/*
======================================
    InitBBoxMatrix()
======================================
*/
static void InitBBoxMatrix(Bound_type *bbox)
{
    if (fabs(bbox->rot.x) >0.001 || fabs(bbox->rot.y) >0.001 || fabs(bbox->rot.z) >0.001 )
    {
        InitRotMat(&bbox->rot,bbox->mat);
        bbox->rotate = TRUE;
        return;
    }
    bbox->rotate = false;
}


/*
======================================
    RunBBox()
======================================
*/
void RunBBox(Bound_type *bbox,input_type *in,point_type *local)
{
point_type temp;


    if (bbox->flags & 0x8)  /* global value to be used */
    {
        temp.x = in->G_hitpoint.x-bbox->loc.x;
        temp.y = in->G_hitpoint.y-bbox->loc.y;
        temp.z = in->G_hitpoint.z-bbox->loc.z;
    }
    else                    /* local values to be used */
    {
        temp.x = in->hitpoint.x - bbox->loc.x;
        temp.y = in->hitpoint.y - bbox->loc.y;
        temp.z = in->hitpoint.z - bbox->loc.z;
    }


    if (bbox->rotate == TRUE)
    {
        vect_t_mat(temp,bbox->mat,local);
    }
    else
    {
        local->x = temp.x;
        local->y = temp.y;
        local->z = temp.z;
    }

    local->x /= bbox->size.x;
    local->y /= bbox->size.y;
    local->z /= bbox->size.z;

    if (bbox->flags & 0x1) local->x = (1.0+local->x)/2.0;
    if (bbox->flags & 0x2) local->y = (1.0+local->y)/2.0;
    if (bbox->flags & 0x4) local->z = (1.0+local->z)/2.0;

}


/*
======================================
    GetToken()
======================================
*/
int GetToken(char *name,name_token_type *tokens)
{
int i=0;

    while((tokens[i].token != -1) && (strcmp(name,tokens[i].name) != 0))
    {
        i++;
    }
    return(tokens[i].token);
}


/*
======================================
    UnassignedErr()
======================================
*/

int UnassignedErr(char *str)
{
    printf("Required value %s is not assigned\n",str);
    SAIL_ERROR_FLAG = TRUE;
    return(false);
}


/*
======================================
    CheckValue()
======================================
*/

CheckValue(assmt_type *t,enum r_type r,enum v_type v,char *EStr)
{
char *ValStr;
    if (t->rvalue.Type == r)
    {
        if (t->rvalue.data.ExprLst->Type == v)
        {
            return(TRUE);
        }
        switch(v)
        {
            case INT:
                ValStr = "integer";
            break;
            case CHAR:
                ValStr = "character";
            break;
            case CHARPTR:
                ValStr = "string";
            break;
            case DOUBLE:
                ValStr = "double";
            break;
            case XYZ:
                ValStr = "xyz()";
            break;
            case RGB:
                ValStr = "rgb()";
            break;
            default:
                ValStr = "unknown type";
        }
        printf("Type error in module %s expects %s\n",EStr,ValStr);
        SAIL_ERROR_FLAG = TRUE;
        return(false);
    }
    switch(r)
    {
        case EXPRLST:
            ValStr = "expression list";
        break;
        case MODULE:
            ValStr = "module";
        break;
        case ASSMTS:
            ValStr = "assignments";
        break;
        case EMPTY:
            ValStr = "empty";
        break;
        default:
            ValStr = "unknown Type";
        break;
    }
    printf("Wrong rvalue in module %s expects %s\n",EStr,ValStr);
    SAIL_ERROR_FLAG = TRUE;
    return(false);
}
