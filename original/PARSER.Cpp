
# line 27 "parser.y"
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
# define YDIFFUSE 257
# define YREFLECT 258
# define YTRANS 259
# define YNORMAL 260
# define YHIGHLIGHT 261
# define YBBOX 262
# define YPARAMAP 263
# define YATTRIBUTE 264
# define YCOLORTABLE 265
# define YCHECK 266
# define YRANGE 267
# define YBITMAP 268
# define YMANDEL 269
# define YSPHERE 270
# define YCONIC 271
# define YBOX 272
# define YBOARD 273
# define YTRIANGLE 274
# define YVIEW 275
# define YGLOBAL 276
# define YCSG 277
# define YAND 278
# define YOR 279
# define YEND 280
# define YDIV 281
# define YRGB 282
# define YXYZ 283
# define YIdentifier 284
# define YInteger 285
# define YReal 286
# define YChar 287
# define YString 288
# define YKeyword 289
# define YRGBString 290
# define UMINUS 291
# define YASSMNT_OP 292
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 410 "parser.y"

 /***********************/
 /****   FUNCTIONS   ****/
 /***********************/

/* we found an error */
void yyerror(s) char *s;
{
    printf("[%s %d] Error: %s\n",SAIL_CurrentFile,SAIL_LINE_NUMBER,s);
    SAIL_ERROR_FLAG=TRUE;
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 51
# define YYLAST 289
short yyact[]={

  24,  33,  83,  79,  77,  25,  61,  40,  43,  34,
  66,  49,  50,  59,  44,  90,  47,  45,  29,  46,
  65,  48,  49,  50,  81,  63,  74,  47,  45,  24,
  46,  42,  48,   3,  25,  41,  36,  87,  24,  49,
  50,  84,  57,  25,  47,  45,  88,  46,  38,  48,
  49,  50,  37,  54,  55,  47,  45,  82,  46,  64,
  48,  49,  50,   2,   5,  28,  47,  45,   8,  46,
   4,  48,   1,  62,  49,  50,  60,   0,   0,  47,
   0,   0,   0,   6,  48,  80,   0,  56,   0,   0,
   0,   0,  62,  52,  53,  76,   0,  78,  51,   0,
  85,   0,   0,   0,   0,   0,   0,  58,  62,  51,
   0,   0,   6,   0,  67,  68,  69,  70,  71,  72,
  73,   0,   0,   0,  75,   0,  51,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  51,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  51,   0,
   0,  86,   0,   0,   0,   0,   0,  89,   0,   0,
   0,  43,  66,  43,   0,  43,   0,   0,  39,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,  10,   9,  15,   7,  11,  12,  13,  14,
  16,  17,  18,  19,  20,  21,  22,  23,   0,   0,
  35,   0,  33,  32,  31,  26,  27,   0,  30,   0,
  34,  10,   9,  15,   7,  11,  12,  13,  14,  16,
  17,  18,  19,  20,  21,  22,  23,   0,   0,   0,
   0,  33,  32,  31,  26,  27,   0,  30,   0,  34,
  33,  32,  31,  26,  27,   0,  30,   0,  34 };
short yypact[]={

 -11,-1000, -40,  -7,   4,-116,-276,-109,  24,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,  -2,  -2,-1000,-1000,-1000,-1000,
-1000,-1000,  14,  14,-1000,-1000, -17,-1000,  -2,-110,
-276,-119,-1000, -36,-274,  -2,  -2,  -2,  -2,  -2,
  -2,  -2, -15,-1000,-1000,  -2,-1000,-1000,  24,-276,
-121,-1000,-1000, -11,-122,-1000, -37,  37,  37,-1000,
-1000,-1000,-1000,  37,-1000,  13,-123,-1000, -18,-1000,
-1000,-281,  -2,-1000,-1000, -22,   2,-1000,  -2, -26,
-1000 };
short yypgo[]={

   0,  72,  31,  35,  68,  70,  53,  65,  18,  33,
  64,  63,  59,  20 };
short yyr1[]={

   0,   1,  11,  11,   9,   9,   9,   9,   9,  10,
  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
  10,  10,  10,  10,  12,  12,  13,   3,   3,   2,
   5,   5,   4,   4,   4,   4,   4,   4,   4,   4,
   4,   4,   4,   4,   4,   4,   4,   7,   8,   8,
   6 };
short yyr2[]={

   0,   2,   2,   3,   1,   5,   4,   3,   4,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   4,   1,   2,   4,
   1,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   2,   1,   1,   1,   1,   1,   1,   2,   2,   1,
   7 };
short yychk[]={

-1000,  -1, -11,  -9,  -5, -10, 123, 265,  -4, 263,
 262, 266, 267, 268, 269, 264, 270, 271, 272, 273,
 274, 275, 276, 277,  40,  45, 285, 286,  -7,  -8,
 288, 284, 283, 282, 290, 280,  -9,  59,  44, 284,
 123,  -3,  -2, 284, 123,  43,  45,  42,  47,  37,
  38, 124,  -4,  -4,  -6,  40,  -6,  59,  -4, 123,
  -3, 125,  -2,  61, -12, -13, 284,  -4,  -4,  -4,
  -4,  -4,  -4,  -4,  41,  -4,  -3, 125,  -9, 125,
 -13,  61,  44, 125,  59,  -8,  -4,  59,  44,  -4,
  41 };
short yydef[]={

   0,  -2,   0,   0,   4,   0,   0,   0,  30,   9,
  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
  20,  21,  22,  23,   0,   0,  41,  42,  43,  44,
  45,  46,   0,   0,  49,   1,   0,   2,   0,   0,
   0,   0,  27,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,  47,   0,  48,   3,  31,   0,
   0,   7,  28,   0,   0,  24,   0,  33,  34,  35,
  36,  37,  38,  39,  32,   0,   0,   6,   0,   8,
  25,   0,   0,   5,  29,   0,   0,  26,   0,   0,
  50 };
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif

#
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 2:
# line 150 "parser.y"
{
        } break;
case 3:
# line 153 "parser.y"
{
        } break;
case 4:
# line 158 "parser.y"
{
            yyval.RValue.Type = EXPRLST;
            yyval.RValue.data.ExprLst = yypvt[-0].ValuePtr;
        } break;
case 5:
# line 163 "parser.y"
{
            yyval.RValue.Type = MODULE;
            yyval.RValue.data.Module = NewSailModule(yypvt[-4].ivalue,yypvt[-3].YYSidentifier.text,yypvt[-1].AssmtsPtr);
        } break;
case 6:
# line 168 "parser.y"
{
            yyval.RValue.Type = MODULE;
            yyval.RValue.data.Module = NewSailModule(yypvt[-3].ivalue,NULL,yypvt[-1].AssmtsPtr);
        } break;
case 7:
# line 173 "parser.y"
{
            yyval.RValue.Type = ASSMTS;
            yyval.RValue.data.Assmts = yypvt[-1].AssmtsPtr;
        } break;
case 8:
# line 178 "parser.y"
{
            printf("Color Table built\n");
            /* does not pass up anything */
            yyval.RValue.Type = EMPTY;
        } break;
case 9:
# line 189 "parser.y"
{
                yyval.ivalue = YPARAMAP;
            } break;
case 10:
# line 193 "parser.y"
{
                yyval.ivalue = YBBOX;
            } break;
case 11:
# line 197 "parser.y"
{
                yyval.ivalue = YCHECK;
            } break;
case 12:
# line 201 "parser.y"
{
                yyval.ivalue = YRANGE;
            } break;
case 13:
# line 205 "parser.y"
{
                yyval.ivalue = YBITMAP;
            } break;
case 14:
# line 209 "parser.y"
{
                yyval.ivalue = YMANDEL;
            } break;
case 15:
# line 213 "parser.y"
{
                yyval.ivalue = YATTRIBUTE;
            } break;
case 16:
# line 217 "parser.y"
{
                yyval.ivalue = YSPHERE;
            } break;
case 17:
# line 221 "parser.y"
{
                yyval.ivalue = YCONIC;
            } break;
case 18:
# line 225 "parser.y"
{
                yyval.ivalue = YBOX;
            } break;
case 19:
# line 229 "parser.y"
{
                yyval.ivalue = YBOARD;
            } break;
case 20:
# line 233 "parser.y"
{
                yyval.ivalue = YTRIANGLE;
            } break;
case 21:
# line 237 "parser.y"
{
                yyval.ivalue = YVIEW;
            } break;
case 22:
# line 241 "parser.y"
{
                yyval.ivalue = YGLOBAL;
            } break;
case 23:
# line 245 "parser.y"
{
                yyval.ivalue = YCSG;
            } break;
case 26:
# line 255 "parser.y"
{
                AddRGBConst(yypvt[-3].YYSidentifier.text,yypvt[-1].Value.u.RGBVal);
            } break;
case 27:
# line 261 "parser.y"
{
                    yyval.AssmtsPtr = yypvt[-0].AssmtsPtr;
                } break;
case 28:
# line 265 "parser.y"
{
                    assmt_type *t;
                    yyval.AssmtsPtr = yypvt[-1].AssmtsPtr;
                    for (t=yyval.AssmtsPtr;t->next != NULL;t = t->next);
                    t->next = yypvt[-0].AssmtsPtr;
                } break;
case 29:
# line 274 "parser.y"
{
                yyval.AssmtsPtr = (assmt_type *) calloc(sizeof(assmt_type),1);
                yyval.AssmtsPtr->lvalue = stralloc(yypvt[-3].YYSidentifier.text);
                yyval.AssmtsPtr->rvalue = yypvt[-1].RValue;
            } break;
case 30:
# line 282 "parser.y"
{
                    yyval.ValuePtr = (value_type *) calloc(sizeof(value_type),1);
                    *yyval.ValuePtr = yypvt[-0].Value;
                    yyval.ValuePtr->next = NULL;
                } break;
case 31:
# line 288 "parser.y"
{
                    value_type *t;
                    yyval.ValuePtr = yypvt[-2].ValuePtr;
                    for(t=yyval.ValuePtr;t->next != NULL;t = t->next);
                    t->next = (value_type *) calloc(sizeof(value_type),1);
                    *t->next = yypvt[-0].Value;
                    t->next->next = NULL;
                } break;
case 32:
# line 300 "parser.y"
{
            yyval.Value = yypvt[-1].Value;
        } break;
case 33:
# line 304 "parser.y"
{
            DoOperator('+',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 34:
# line 308 "parser.y"
{
            DoOperator('-',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 35:
# line 312 "parser.y"
{
            DoOperator('*',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 36:
# line 316 "parser.y"
{
            DoOperator('/',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 37:
# line 320 "parser.y"
{
            DoOperator('%',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 38:
# line 324 "parser.y"
{
            DoOperator('&',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 39:
# line 328 "parser.y"
{
            DoOperator('|',&yypvt[-2].Value,&yypvt[-0].Value,&yyval.Value);
        } break;
case 40:
# line 332 "parser.y"
{
            DoOperator('-',NULL,&yypvt[-0].Value,&yyval.Value);
        } break;
case 41:
# line 336 "parser.y"
{
            yyval.Value.Type = INT;
            yyval.Value.u.IntVal = yypvt[-0].YYSinteger.val;
        } break;
case 42:
# line 341 "parser.y"
{
            yyval.Value.Type = DOUBLE;
            yyval.Value.u.DoubleVal = yypvt[-0].YYSreal.val;
        } break;
case 43:
# line 346 "parser.y"
{
            yyval.Value = yypvt[-0].Value;
        } break;
case 44:
# line 350 "parser.y"
{
            yyval.Value = yypvt[-0].Value;
        } break;
case 45:
# line 354 "parser.y"
{
            yyval.Value.Type = CHARPTR;
            yyval.Value.u.CharPtrVal = stralloc(yypvt[-0].YYSstring.val);
        } break;
case 46:
# line 359 "parser.y"
{
            /* TEMP */
            yyval.Value.Type = CHARPTR;
            yyval.Value.u.CharPtrVal = stralloc(yypvt[-0].YYSidentifier.text);
        } break;
case 47:
# line 368 "parser.y"
{
                yyval.Value = yypvt[-0].Value;
                yyval.Value.Type = XYZ;
            } break;
case 48:
# line 375 "parser.y"
{
                yyval.Value.u.RGBVal.r = yypvt[-0].Value.u.XYZVal.x;
                yyval.Value.u.RGBVal.g = yypvt[-0].Value.u.XYZVal.y;
                yyval.Value.u.RGBVal.b = yypvt[-0].Value.u.XYZVal.z;
                yyval.Value.Type = RGB;
            } break;
case 49:
# line 382 "parser.y"
{
                rgb_type *t;
                t = GetRGBConst(yypvt[-0].YYSRGBString.val);
                yyval.Value.u.RGBVal.r = t->r;
                yyval.Value.u.RGBVal.g = t->g;
                yyval.Value.u.RGBVal.b = t->b;
                yyval.Value.Type = RGB;
            } break;
case 50:
# line 393 "parser.y"
{
                if (yypvt[-5].Value.Type == DOUBLE && yypvt[-3].Value.Type == DOUBLE && yypvt[-1].Value.Type == DOUBLE)
                {
                    yyval.Value.u.XYZVal.x = yypvt[-5].Value.u.DoubleVal;
                    yyval.Value.u.XYZVal.y = yypvt[-3].Value.u.DoubleVal;
                    yyval.Value.u.XYZVal.z = yypvt[-1].Value.u.DoubleVal;
                }
                else
                {
                     printf("Error: vector values must contain Doubles\n");
                     yyval.Value.u.XYZVal.x = 0.0;
                     yyval.Value.u.XYZVal.y = 0.0;
                     yyval.Value.u.XYZVal.z = 0.0;
                }
            } break; 
		}
		goto yystack;  /* stack new state and value */

	}
