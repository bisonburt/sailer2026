/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | bitmap.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The bitmap module of SAIL
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
    {"picture",1},{"rep",2},
    {"flags",3},{"color",4},{"refl",5},{"trans",6},
    {"",-1}
};

void RunBitmap(sail_module_type *,input_type *,output_type *);
static int InitBitmap(char * ,Bitmap_type *);

/*
======================================
    BuildBitmap()
======================================
*/
sail_module_type *BuildBitmap(assmt_type *AssmtList)
{
sail_module_type *module;
Bitmap_type *this;
assmt_type *t;
value_type *t1;
int token;

    /* allocate a sail structure */
    module = (sail_module_type *) calloc(sizeof(sail_module_type),1);
    this = &module->module.bitmap;

    this->rep.x = 1.0;
    this->rep.y = 1.0;
    this->rep.z = 1.0;
    this->RetType = 0x1; /* return color only */

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* picure */
                if (CheckValue(t,EXPRLST,CHARPTR,"bitmap, picture"))
                InitBitmap(t->rvalue.data.ExprLst->u.CharPtrVal,&module->module.bitmap);
            break;
            case 2: /* rep */
                if (CheckValue(t,EXPRLST,XYZ,"bitmap, rep"))
                module->module.bitmap.rep = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* flags */
                if (t->rvalue.Type == EXPRLST)
                {
                    for (t1 = t->rvalue.data.ExprLst; t1 != NULL; t1 = t1->next)
                    {
                        switch(token = GetToken(t1->u.CharPtrVal,values))
                        {
                            case 4: /* color */
                                this->RetType |= 0x1;
                            break;
                            case 5: /* refl */
                                this->RetType |= 0x2;
                            break;
                            case 6: /* trans */
                                this->RetType |= 0x04;
                            break;
                            default:
                                printf("Error in check, bad flag\n");
                                SAIL_ERROR_FLAG = TRUE;
                            break;
                        }
                    }
                    break;
                }
                printf("Error in bitmap, Expected an EXPRLST type for flags\n");
                SAIL_ERROR_FLAG = TRUE;
            break;
            default:
                printf("[bitmap, %d] bad lvalue: %s\n",SAIL_LINE_NUMBER,t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    module->funct = RunBitmap;
    return(module);
}


/*
======================================
    RunBitmap()
======================================
*/
void RunBitmap(sail_module_type *module,input_type *in,output_type *out)
{
int x,y;
rgb_type value;
Bitmap_type *data;
int offset;
unsigned char *buff;
double u,v;
int bx,by;

    data = &module->module.bitmap;

    /* make code easier to read */
    u = frac(in->hitpoint.x*data->rep.x);
    v = frac(in->hitpoint.y*data->rep.y);
    x = data->x;
    y = data->y;
    bx = (int)floor(u*(double)x);
    by = (int)floor((1.0-v)*(double)y);

    /* compute location of rgb value */

    offset =  3*((by*x)+(bx));
    buff = data->bitmap;

    /* assign return value to pixel color */
    value.r = (double)buff[offset]/255.0;
    value.g = (double)buff[offset+1]/255.0;
    value.b = (double)buff[offset+2]/255.0;


    if(data->RetType & 0x1)
    {
        out->color = value;
    }
    if(data->RetType & 0x2)
    {
        out->reflect = value;
    }
    if(data->RetType & 0x4)
    {
        out->trans = value;
    }
    return;
}


static int InitBitmap(char * file,Bitmap_type *data)
{
int len,x,y;
int size;
static char buff[128];
FILE *fmap;

    if ((fmap = fopen(file,"r")) == NULL)
    {
        printf("Error opening bitmap file %s\n",file);
        return(0);
    }

    /* eat comment lines if any and get next useful line */
    for(fgets(buff,128,fmap);buff[0] == '#';fgets(buff,128,fmap));

    /* determine if file is a p6 ppm file */
    if (toupper(buff[0]) != 'P' || buff[1] != '6')
    {
        printf("bitmap file not ppm (p6) file\n");
        fclose(fmap);
        return(0);
    }

    /* eat comment lines if any and get next useful line*/
    for(fgets(buff,128,fmap);buff[0] == '#';fgets(buff,128,fmap));

    /* get size of file */
    sscanf(buff,"%d %d",&x,&y);

    printf("bitmap: %s [%d x %d]\n",file,x,y);

    data->x = x;
    data->y = y;

    /* eat comment lines if any and get next useful line*/
    for(fgets(buff,128,fmap);buff[0] == '#';fgets(buff,128,fmap));

    /* allocate memory to store bitmap */
    size = x*y*3*sizeof(char);
    if ((data->bitmap = (void *) malloc(size)) == NULL)
    {
        printf("bitmap size is to large to allocate\n");
        fclose(fmap);
        return(0);
    }

    /* read bitmap body into memory */
    len = fread(data->bitmap,size,1,fmap);

    /* determine if fread() above was successfull */
    if (len < 1)
    {
        printf("bitmap body was shorter than expected\n");
        free(data->bitmap); /* free allocated bitmap */
        fclose(fmap);
        return(0);
    }

    /* done */
    return(1);
}
