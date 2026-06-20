/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | translat.c
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | This file consists interface to sail. All communication
              | to sail happens through here.
              |
==============================================================================
*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include "structs.h"
#include "mathtype.h"
#include "support.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "tokens.h"

/*
   NOTE: InitSail() has moved to jsonscene.c, which parses a JSON scene
   description instead of driving the old lex/yacc parser. The texture
   interface functions below are unchanged and still used by the ray tracer.
*/

void *InitTexture(char *sailname)
{
    return((void *) FindModule(sailname));
}


int GetTexture(void *module,input_type *input,output_type *output)
{
    sail_module_type *mod = (sail_module_type *)module;

    input->G_hitpoint = input->hitpoint;

    /* default to black for diffuse color */
    output->color.r = 0.0;
    output->color.g = 0.0;
    output->color.b = 0.0;

    /* default to white for reflective color */
    output->reflect.r = 1.0;
    output->reflect.g = 1.0;
    output->reflect.b = 1.0;

    /* default to black for transparent color */
    output->trans.r = 0.0;
    output->trans.g = 0.0;
    output->trans.b = 0.0;

    /* default highlight */

    output->highlight = 0.0;

    mod->funct(mod,input,output);

    output->newnormal = input->normal; /* until normal support is done */
    return(TRUE); /* successful */
}
