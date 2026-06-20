/*
   WritePPM.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mathtype.h"
#include "structs.h"
#include "writeppm.h"
#include "mathfun.h"

#define BUFFSIZE 4096              /* size of buffer */
static char buff[BUFFSIZE][3];    /* the actual file block buffer */
static int size = 0;              /* nothing in buffer yet */
FILE *fp;                         /* name of file pointer */

int OpenPPM(name,w,h)
char *name;
int w,h;
{
    if((fp = fopen(name,"wb")) == NULL)
    {
        printf("Error opening file: %s\n",name);
        return(-1);
    }
    fprintf(fp,"P6\n%d %d\n255\n",w,h);
    return(0);
}

void WritePPM(rgb)
rgb_type rgb;
{

    buff[size][0] = rgb.r*255;
    buff[size][1] = rgb.g*255;
    buff[size][2] = rgb.b*255;
    size++;

    if (size == BUFFSIZE)
    {
        fwrite(buff,BUFFSIZE,3,fp); /* flush buffer */
        size = 0;
    }
}

void ClosePPM()
{
    fwrite(buff,size,3,fp); /* flush buffer */
    size = 0;
    fclose(fp);
}

