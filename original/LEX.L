    /*
        WHAT:   lex.l
        FOR:    sail ver 1.0
        BY:     David Fletcher
        WHEN:   16 Oct 1992
        LEGAL:  (c) 1992, David Fletcher
    */

 /**** Externals ****/
%{
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include "mathtype.h"
#include "basics.h"
#include "support.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "parser.h"
#include "tokens.h"


char SAIL_CurrentFile[128];
extern YYSTYPE yylval;
int SAIL_LINE_NUMBER=1; /* GLOBAL for current line */
%}

digit           [0-9]
letter          [a-zA-Z]
exp_digit       [eE]
alphanumeric    {letter}|{digit}
other_id_char   [_]
instr           [^\"\n]|\\\"
string          \"({instr})*\"
char            \'.\'|\'\\.\'
whitespace      [ \t]
newline         [\n]
other           {alphanumeric}|{other_id_char}
identifier      {letter}({other})*
rgbconstant     !{identifier}
integer         {digit}+
real            {integer}"."({integer})?({exp_digit}[+-]?{integer})?
octal_digit     [0-7]
octal           "0"({octal_digit})+
hex_digit       [0-9a-fA-F]
hex             "0"[xX]({hex_digit})+
directive1       #({whitespace})*{integer}({whitespace})*{string}
directive2       #({whitespace})*{integer}({whitespace})*{string}({whitespace})*{integer}

 /***********************
  *****  LEX RULES  *****
  ***********************/
%%


 /* these are the module type keywords ADD_BELOW_FOR_MODULE */

paramap     {yylval.keyword.text = stralloc(yytext); return(YPARAMAP);}
bbox        {yylval.keyword.text = stralloc(yytext); return(YBBOX);}
check       {yylval.keyword.text = stralloc(yytext); return(YCHECK);}
range       {yylval.keyword.text = stralloc(yytext); return(YRANGE);}
bitmap      {yylval.keyword.text = stralloc(yytext); return(YBITMAP);}
mandel      {yylval.keyword.text = stralloc(yytext); return(YMANDEL);}
attribute   {yylval.keyword.text = stralloc(yytext); return(YATTRIBUTE);}
sphere      {yylval.keyword.text = stralloc(yytext); return(YSPHERE);}
conic       {yylval.keyword.text = stralloc(yytext); return(YCONIC);}
box         {yylval.keyword.text = stralloc(yytext); return(YBOX);}
board       {yylval.keyword.text = stralloc(yytext); return(YBOARD);}
triangle    {yylval.keyword.text = stralloc(yytext); return(YTRIANGLE);}
view        {yylval.keyword.text = stralloc(yytext); return(YVIEW);}
global      {yylval.keyword.text = stralloc(yytext); return(YGLOBAL);}
csg         {yylval.keyword.text = stralloc(yytext); return(YCSG);}
CSG         {yylval.keyword.text = stralloc(yytext); return(YCSG);}


OR         {yylval.keyword.text = stralloc(yytext); return(YOR);}
DIV        {yylval.keyword.text = stralloc(yytext); return(YDIV);}
AND        {yylval.keyword.text = stralloc(yytext); return(YAND);}
END        {yylval.keyword.text = stralloc(yytext); return(YEND);}
RGB        {yylval.keyword.text = stralloc(yytext); return(YRGB);}
XYZ        {yylval.keyword.text = stralloc(yytext); return(YXYZ);}
rgb        {yylval.keyword.text = stralloc(yytext); return(YRGB);}
xyz        {yylval.keyword.text = stralloc(yytext); return(YXYZ);}
colortable {yylval.keyword.text = stralloc(yytext); return(YCOLORTABLE);}

"%"             {yylval.keyword.text = stralloc(yytext); return('%');}
"|"             {yylval.keyword.text = stralloc(yytext); return('|');}
"&"             {yylval.keyword.text = stralloc(yytext); return('&');}
";"             {yylval.keyword.text = stralloc(yytext); return(';');}
"="             {yylval.keyword.text = stralloc(yytext); return('=');}
"+"             {yylval.keyword.text = stralloc(yytext); return('+');}
"-"             {yylval.keyword.text = stralloc(yytext); return('-');}
"*"             {yylval.keyword.text = stralloc(yytext); return('*');}
"/"             {yylval.keyword.text = stralloc(yytext); return('/');}
"("             {yylval.keyword.text = stralloc(yytext); return('(');}
")"             {yylval.keyword.text = stralloc(yytext); return(')');}
"{"             {yylval.keyword.text = stralloc(yytext); return('{');}
"}"             {yylval.keyword.text = stralloc(yytext); return('}');}
"["             {yylval.keyword.text = stralloc(yytext); return('[');}
"]"             {yylval.keyword.text = stralloc(yytext); return(']');}
","             {yylval.keyword.text = stralloc(yytext); return(',');}


"/*"            {
                    register int c;
                    for ( ; ; )
                    {
                        /* eat up text of comment */
                        while ( (c = input()) != '*' && c != EOF );
                        if (c == '\n') SAIL_LINE_NUMBER++;
                        if ( c == '*')
                        {
                            while ((c = input()) == '*' );
                            if (c == '\n') SAIL_LINE_NUMBER++;
                            if ( c == '/') break; /* found the end */
                        }
                        if (c == EOF)
                        {
                            printf("Error in lex, EOF in comment\n");
                            break;
                        }
                    }
                }

{directive1}    {
                    int i=0;
                    sscanf(yytext,"# %d \"%s",&SAIL_LINE_NUMBER,SAIL_CurrentFile);
                    SAIL_LINE_NUMBER--;
                    while ((SAIL_CurrentFile[i] != '\"') && (SAIL_CurrentFile[i] != '\0')) i++;
                    SAIL_CurrentFile[i] = '\0';
                }

{directive2}    {
                    int i=0;
                    sscanf(yytext,"# %d \"%s",&SAIL_LINE_NUMBER,SAIL_CurrentFile);
                    SAIL_LINE_NUMBER--;
                    while ((SAIL_CurrentFile[i] != '\"') && (SAIL_CurrentFile[i] != '\0')) i++;
                    SAIL_CurrentFile[i] = '\0';
                }

{hex}           {
                    int x;
                    sscanf(yytext,"%xH",&x); yylval.YYSinteger.val = x;
                    return(YInteger);
                }

{octal}         {
                    int x;
                    sscanf(yytext,"%oB",&x);
                    yylval.YYSinteger.val = x;
                    return(YInteger);
                }

{string}        {
                    char buff[256]; int i=1,j=0;
                    while (i<yyleng-1)
                    {
                        if (yytext[i] != '\\')
                        {
                            buff[j++]=yytext[i++];
                        }
                        else
                        {
                            switch(yytext[++i])
                            {
                                case 'n': buff[j++] = '\n';i++; break;
                                case 't': buff[j++] = '\t';i++; break;
                                default: buff[j++] = yytext[i++]; break;
                            }
                         }
                    }
                    buff[j]=NULL;
                    yylval.YYSstring.val = stralloc(buff);
                    return(YString);
                }

{char}          {
                    if (yytext[1] == '\\')
                    {
                        switch (yytext[2])
                        {
                            case 'n' : yylval.YYSchar.val = '\n'; break;
                            case 't' : yylval.YYSchar.val = '\t'; break;
                            default : yylval.YYSchar.val = yytext[2];
                        }
                    }
                    else
                    {
                        yylval.YYSchar.val = yytext[1];
                    }
                    return(YChar);
                }

{identifier}	{
                    yylval.YYSidentifier.text = stralloc(yytext);
		            return(YIdentifier);
                }

{rgbconstant}   {
                    yylval.YYSRGBString.val = stralloc(yytext);
                    return(YRGBString);

                }

{integer}       {
                    yylval.YYSinteger.val = atoi(yytext);
                    return(YInteger);
                }

{real}  		{   char c;
                    if (yytext[yyleng-1] == '.')
                    {
                        c = input(); unput(c);
                        if ((c == '.'))
                        {
                            REJECT;
                            break;
                        }
                    }
                    yylval.YYSreal.val = atof(yytext);
                    return(YReal);
                }
{newline}       {SAIL_LINE_NUMBER++;};
{whitespace}    ;
.               {
                    printf("[%s %d] Error in lex, invalid char: %c\n",SAIL_CurrentFile,SAIL_LINE_NUMBER,yytext[0]);
                }
