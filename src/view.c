/*
    view.c
*/
#include <stdio.h>
#include <ctype.h>
#include "structs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mathtype.h"
#include "mathfun.h"
#include "view.h"
#include "sailpub.h"
#include "tokens.h"
#include "parse_tr.h"
#include "sail.h"
#include "support.h"



static int Width,Height;
static point_type viewpoint,M,H,V;

static name_token_type values[] =
{
    {"viewpoint",1},{"gazedir",2},{"updir",3},{"angle",4},{"ratio",5},
    {"width",6},{"height",7},{"",-1}
};




/*
===============================================================
   Function: GetView()
===============================================================
*/
void GetView(x,y,p)
int x,y;
point_type *p;
{
double sx,sy;

    sx = (double)x/(double)Width;
    sy = (double)y/(double)Height;
    p->x = M.x + (1.0 - 2.0*sx)*H.x + (1.0 - 2.0*sy)*V.x;
    p->y = M.y + (1.0 - 2.0*sx)*H.y + (1.0 - 2.0*sy)*V.y;
    p->z = M.z + (1.0 - 2.0*sx)*H.z + (1.0 - 2.0*sy)*V.z;
}


/*
===============================================================
   Function: GetWidth()
===============================================================
*/
int GetWidth()
{
    return(Width);
}


/*===============================================================
   Function: GetHeight()
===============================================================
*/
int GetHeight()
{
    return(Height);
}


/*
===============================================================
   Function: GetViewpoint()
===============================================================
*/
point_type *GetViewpoint()
{
    return(&viewpoint);
}

/*
===============================================================
   Function: GetCameraVectors()
   Returns the M/H/V basis vectors needed to reproduce GetView().
===============================================================
*/
void GetCameraVectors(point_type *m, point_type *h, point_type *v)
{
    *m = M; *h = H; *v = V;
}


/*
===============================================================
   Function: InitView()
===============================================================
*/

void InitView(vp,gd,up,ang,rat,iwidth,iheight)
point_type *vp,*gd,*up;
double ang,rat;
{
point_type A,B;

    Width = iwidth;
    Height = iheight;
    printf("Width = %d, Height = %d\n",Width,Height);
    viewpoint = *vp;

    vcross(gd,up,&A);
    vcross(&A,gd,&B);
    M.x = vp->x+gd->x; M.y = vp->y+gd->y; M.z = vp->z+gd->z;
    vscale(rat*tan(ang)*vlen(gd)/vlen(&A),&A,&H);
    vscale(tan(ang)*vlen(gd)/vlen(&B),&B,&V);
}

/*
======================================
    BuildView()
======================================
*/
void BuildView(assmt_type *AssmtList)
{
assmt_type *t;
int token;

point_type viewpoint,gazedir,updir;
double angle,ratio;
int width,height;

    for (t = AssmtList; t != NULL; t = t->next)
    {
        switch(token = GetToken(t->lvalue,values))
        {
            case 1: /* viewpoint */
                if (CheckValue(t,EXPRLST,XYZ,"view, viewpoint"))
                viewpoint = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 2: /* gazedir */
                if (CheckValue(t,EXPRLST,XYZ,"view, gazedir"))
                gazedir = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 3: /* updir */
                if (CheckValue(t,EXPRLST,XYZ,"view, updir"))
                updir = t->rvalue.data.ExprLst->u.XYZVal;
            break;
            case 4: /* angle */
                if (CheckValue(t,EXPRLST,DOUBLE,"view, angle"))
                angle = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 5: /* ratio */
                if (CheckValue(t,EXPRLST,DOUBLE,"view, ratio"))
                ratio = t->rvalue.data.ExprLst->u.DoubleVal;
            break;
            case 6: /* width */
                if (CheckValue(t,EXPRLST,INT,"view, width"))
                width = t->rvalue.data.ExprLst->u.IntVal;
            break;
            case 7: /* height */
                if (CheckValue(t,EXPRLST,INT,"view, height"))
                height = t->rvalue.data.ExprLst->u.IntVal;
            break;
            default:
                printf("Unknown lvalue (%s) found in a view module\n",t->lvalue);
                SAIL_ERROR_FLAG = TRUE;
            break;
        }
    }
    /* tell raytracer about object */
    printf("About to call InitView()\n");
    InitView(&viewpoint,&gazedir,&updir,angle,ratio,width,height);
    return;
}
