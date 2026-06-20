/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | parser.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 17 Oct 1992
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | The header file for the parser!
              | (if you know yacc, you know this file)
              |
==============================================================================
*/


/* For now this contains only the YYSTYPE type decl; maybe more later. */

typedef union {

    /* these are for passing stuff from lex */
    struct {
	char *text;
    } keyword;

    struct {
	char *text;
    } YYSidentifier;

    struct {
	int  val;
    } YYSinteger;

    struct {
	double val;
    }  YYSreal;

    struct {
        char *val;
    }  YYSstring;

    struct {
        char val;
    }  YYSchar;

    struct {
        char *val;
    }  YYSRGBString;

    value_type Value;
    value_type *ValuePtr;
    rvalue_type RValue;
    assmt_type *AssmtsPtr;


    /* some commom types */
    char cvalue;            /* a char */
    double fvalue;          /* a double */
    char *svalue;           /* a string */
    int ivalue;             /* a integer */
    rgb_type rgbvalue;      /* rgb value */
    point_type xyzvalue;    /* a xyz coord and/or vector */

    nodep nodepointer;  /* for the complex types */
}  YYSTYPE;

static nodep parsetree = null;
