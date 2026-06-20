/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     |
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | This file parses the input file. It uses lex.l for getting
              | tokens and calls functions in sail.c. This file defines the
              | SAIL language.
              |
==============================================================================
*/


/*
=====================================================
    Includes for definition files
=====================================================
*/
%{
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "mathtype.h"
#include "structs.h"
#include "support.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "parser.h"
extern int SAIL_LINE_NUMBER;
extern char SAIL_CurrentFile[128];
void yyerror(char *s);
%}

/*
=====================================================
    Start Symbol
=====================================================
*/
%start Top

/*
=====================================================
    Terminal Symbols
=====================================================
*/
%token <keyword> YDIFFUSE
%token <keyword> YREFLECT
%token <keyword> YTRANS
%token <keyword> YNORMAL
%token <keyword> YHIGHLIGHT

/* ADD_BELOW_FOR_MODULE */
%token <keyword> YBBOX
%token <keyword> YPARAMAP
%token <keyword> YATTRIBUTE
%token <keyword> YCOLORTABLE
%token <keyword> YCHECK
%token <keyword> YRANGE
%token <keyword> YBITMAP
%token <keyword> YMANDEL
%token <keyword> YSPHERE
%token <keyword> YCONIC
%token <keyword> YBOX
%token <keyword> YBOARD
%token <keyword> YTRIANGLE
%token <keyword> YVIEW
%token <keyword> YGLOBAL
%token <keyword> YCSG


%token <keyword> YAND
%token <keyword> YOR
%token <keyword> YEND
%token <keyword> YDIV
%token <keyword> YRGB
%token <keyword> YXYZ
%token <keyword> '%'
%token <keyword> '|'
%token <keyword> '&'
%token <keyword> ';'
%token <keyword> '='
%token <keyword> '+'
%token <keyword> '-'
%token <keyword> '*'
%token <keyword> '/'
%token <keyword> '('
%token <keyword> ')'
%token <keyword> '{'
%token <keyword> '}'
%token <keyword> '['
%token <keyword> ']'
%token <keyword> ','

/*
=======================================================================
    Non-Terminal cum Terminal Symbols
    -- these are typically non-terminals in a standard grammar that are
    -- taken care of by lex, thereby becoming terminals here
=======================================================================
*/

%token <YYSidentifier> YIdentifier
%token <YYSinteger> YInteger
%token <YYSreal> YReal
%token <YYSchar> YChar
%token <YYSstring> YString
%token <YYSkeyword> YKeyword
%token <YYSRGBString> YRGBString
/*
=====================================================
    Operator Precedences
=====================================================
*/
%left '+' '-' '|'               /* AddOperator */
%left '*' '/' '%' '&'     /* MultOperator */
%left UMINUS
%right YASSMNT_OP			    /* = */
/*
=====================================================
    Non-Terminal Symbols
=====================================================
*/

%type <AssmtsPtr> Assignment
%type <AssmtsPtr> AssignmentList
%type <Value> Expr
%type <ValuePtr> ExpressionList
%type <Value> Vector3D
%type <Value> XYZValue
%type <Value> RGBValue
%type <RValue> Module
%type <ivalue> YOperatorID

%%
/*************************/
/******    RULES    ******/
/*************************/
Top     : Modules YEND
        ;

Modules : Module ';'
        {
        }
        | Modules Module ';'
        {
        }
        ;

Module  : ExpressionList
        {
            $$.Type = EXPRLST;
            $$.data.ExprLst = $1;
        }
        | YOperatorID YIdentifier '{' AssignmentList '}'
        {
            $$.Type = MODULE;
            $$.data.Module = NewSailModule($1,$2.text,$4);
        }
        | YOperatorID '{' AssignmentList '}'
        {
            $$.Type = MODULE;
            $$.data.Module = NewSailModule($1,NULL,$3);
        }
        | '{' AssignmentList '}'
        {
            $$.Type = ASSMTS;
            $$.data.Assmts = $2;
        }
        | YCOLORTABLE '{' ColorList '}'
        {
            printf("Color Table built\n");
            /* does not pass up anything */
            $$.Type = EMPTY;
        }
        ;


/* ADD_BELOW_FOR_MODULE */

YOperatorID : YPARAMAP
            {
                $$ = YPARAMAP;
            }
            | YBBOX
            {
                $$ = YBBOX;
            }
            |  YCHECK
            {
                $$ = YCHECK;
            }
            |  YRANGE
            {
                $$ = YRANGE;
            }
            |  YBITMAP
            {
                $$ = YBITMAP;
            }
            |  YMANDEL
            {
                $$ = YMANDEL;
            }
            |  YATTRIBUTE
            {
                $$ = YATTRIBUTE;
            }
            |  YSPHERE
            {
                $$ = YSPHERE;
            }
            |  YCONIC
            {
                $$ = YCONIC;
            }
            |  YBOX
            {
                $$ = YBOX;
            }
            |  YBOARD
            {
                $$ = YBOARD;
            }
            |  YTRIANGLE
            {
                $$ = YTRIANGLE;
            }
            |  YVIEW
            {
                $$ = YVIEW;
            }
            |  YGLOBAL
            {
                $$ = YGLOBAL;
            }
            |  YCSG
            {
                $$ = YCSG;
            }
            ;

ColorList   : ColorAssmt
            | ColorList ColorAssmt
            ;

ColorAssmt  : YIdentifier '=' RGBValue ';'
            {
                AddRGBConst($1.text,$3.u.RGBVal);
            }
            ;

AssignmentList  : Assignment
                {
                    $$ = $1;
                }
                | AssignmentList Assignment
                {
                    assmt_type *t;
                    $$ = $1;
                    for (t=$$;t->next != NULL;t = t->next);
                    t->next = $2;
                }
                ;

Assignment  : YIdentifier '=' Module ';'
            {
                $$ = (assmt_type *) calloc(sizeof(assmt_type),1);
                $$->lvalue = stralloc($1.text);
                $$->rvalue = $3;
            }
            ;

ExpressionList  : Expr
                {
                    $$ = (value_type *) calloc(sizeof(value_type),1);
                    *$$ = $1;
                    $$->next = NULL;
                }
                | ExpressionList ',' Expr
                {
                    value_type *t;
                    $$ = $1;
                    for(t=$$;t->next != NULL;t = t->next);
                    t->next = (value_type *) calloc(sizeof(value_type),1);
                    *t->next = $3;
                    t->next->next = NULL;
                }
                ;


Expr   : '(' Expr ')'
        {
            $$ = $2;
        }
        | Expr '+' Expr
        {
            DoOperator('+',&$1,&$3,&$$);
        }
        | Expr '-' Expr
        {
            DoOperator('-',&$1,&$3,&$$);
        }
        | Expr '*' Expr
        {
            DoOperator('*',&$1,&$3,&$$);
        }
        | Expr '/' Expr
        {
            DoOperator('/',&$1,&$3,&$$);
        }
        | Expr '%' Expr
        {
            DoOperator('%',&$1,&$3,&$$);
        }
        | Expr '&' Expr
        {
            DoOperator('&',&$1,&$3,&$$);
        }
        | Expr '|' Expr
        {
            DoOperator('|',&$1,&$3,&$$);
        }
        | '-' Expr  %prec UMINUS
        {
            DoOperator('-',NULL,&$2,&$$);
        }
        | YInteger
        {
            $$.Type = INT;
            $$.u.IntVal = $1.val;
        }
        | YReal
        {
            $$.Type = DOUBLE;
            $$.u.DoubleVal = $1.val;
        }
        | XYZValue
        {
            $$ = $1;
        }
        | RGBValue
        {
            $$ = $1;
        }
        | YString
        {
            $$.Type = CHARPTR;
            $$.u.CharPtrVal = stralloc($1.val);
        }
        | YIdentifier
        {
            /* TEMP */
            $$.Type = CHARPTR;
            $$.u.CharPtrVal = stralloc($1.text);
        }
        ;


XYZValue    : YXYZ Vector3D
            {
                $$ = $2;
                $$.Type = XYZ;
            }
            ;

RGBValue    : YRGB Vector3D
            {
                $$.u.RGBVal.r = $2.u.XYZVal.x;
                $$.u.RGBVal.g = $2.u.XYZVal.y;
                $$.u.RGBVal.b = $2.u.XYZVal.z;
                $$.Type = RGB;
            }
            | YRGBString     /* from lex. ie. !DeepDarkRed */
            {
                rgb_type *t;
                t = GetRGBConst($1.val);
                $$.u.RGBVal.r = t->r;
                $$.u.RGBVal.g = t->g;
                $$.u.RGBVal.b = t->b;
                $$.Type = RGB;
            }
            ;

Vector3D    : '(' Expr ',' Expr ',' Expr ')' /* use for all 3 value types */
            {
                if ($2.Type == DOUBLE && $4.Type == DOUBLE && $6.Type == DOUBLE)
                {
                    $$.u.XYZVal.x = $2.u.DoubleVal;
                    $$.u.XYZVal.y = $4.u.DoubleVal;
                    $$.u.XYZVal.z = $6.u.DoubleVal;
                }
                else
                {
                     printf("Error: vector values must contain Doubles\n");
                     $$.u.XYZVal.x = 0.0;
                     $$.u.XYZVal.y = 0.0;
                     $$.u.XYZVal.z = 0.0;
                }
            }
            ;

%%
 /***********************/
 /****   FUNCTIONS   ****/
 /***********************/

/* we found an error */
void yyerror(s) char *s;
{
    printf("[%s %d] Error: %s\n",SAIL_CurrentFile,SAIL_LINE_NUMBER,s);
    SAIL_ERROR_FLAG=TRUE;
}
