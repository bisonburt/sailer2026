/*
=================== Surface Attributes Interpreted Language ===================
              |
    File:     | jsonscene.c
    Version:  | 3.0 SAILER (JSON front-end)
    Author:   | David Fletcher (original), JSON port 2026
              |
    Summary:  | Replaces the original lex/yacc front-end (lex.l + parser.y).
              | Reads a JSON scene description and builds exactly the same
              | parse-tree structures (value_type / rvalue_type / assmt_type)
              | that the grammar used to build, then hands each top-level
              | object to NewSailModule(). Every downstream BuildXXX() module
              | builder and the ray-tracer kernel are therefore reused
              | unchanged.
              |
    JSON model (mirrors the old "object-like" scene language):
              |
      Top level: either a JSON array of module objects, or an object
      { "colortable": { ... }, "scene": [ ... ] }.
              |
      A module object has:
        "kind"  : string  -- view|global|paramap|bbox|check|range|
                             bitmap|mandel|attribute|sphere|conic|box|
                             board|triangle|csg
        "name"  : string  -- optional, registers a reusable module
        <other keys>      -- assignments (lvalue = rvalue)
              |
      Value (rvalue) encoding:
        number              -> scalar (int if integral, else double)
        string              -> identifier reference OR string literal
        ["a","b"]           -> flag / expression list
        {"xyz":[x,y,z]}     -> xyz() vector
        {"rgb":[r,g,b]}     -> rgb() color
        {"kind":...}        -> nested module
        { ...plain object } -> assignment block (e.g. mapx, solid, fade)
        [ {..}, {..} ]      -> repeated assignment (same lvalue), used by
                               range's multiple solid/fade entries
==============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mathtype.h"
#include "structs.h"
#include "support.h"
#include "sailpub.h"
#include "parse_tr.h"
#include "sail.h"
#include "tokens.h"
#include "cJSON.h"

/* from support.c */
extern void InitNoise(void);

/* The file/line the lexer used to track; kept so module error messages that
   reference SAIL_LINE_NUMBER still link and print something sensible. */
int SAIL_LINE_NUMBER = 0;
char SAIL_CurrentFile[128] = "<json>";

/* ------------------------------------------------------------------ */
/* kind keyword -> yacc token id                                       */
/* ------------------------------------------------------------------ */
static int kind_token(const char *s)
{
    if (s == NULL) return -1;
    if (strcmp(s, "view") == 0)       return YVIEW;
    if (strcmp(s, "global") == 0)     return YGLOBAL;
    if (strcmp(s, "paramap") == 0)    return YPARAMAP;
    if (strcmp(s, "bbox") == 0)       return YBBOX;
    if (strcmp(s, "check") == 0)      return YCHECK;
    if (strcmp(s, "range") == 0)      return YRANGE;
    if (strcmp(s, "bitmap") == 0)     return YBITMAP;
    if (strcmp(s, "mandel") == 0)     return YMANDEL;
    if (strcmp(s, "attribute") == 0)  return YATTRIBUTE;
    if (strcmp(s, "sphere") == 0)     return YSPHERE;
    if (strcmp(s, "conic") == 0)      return YCONIC;
    if (strcmp(s, "box") == 0)        return YBOX;
    if (strcmp(s, "board") == 0)      return YBOARD;
    if (strcmp(s, "triangle") == 0)   return YTRIANGLE;
    if (strcmp(s, "csg") == 0 || strcmp(s, "CSG") == 0) return YCSG;
    if (strcmp(s, "colortable") == 0) return YCOLORTABLE;
    return -1;
}

/* ------------------------------------------------------------------ */
/* allocation helpers                                                  */
/* ------------------------------------------------------------------ */
static value_type *new_value(void)
{
    return (value_type *)calloc(sizeof(value_type), 1);
}

static assmt_type *new_assmt(void)
{
    return (assmt_type *)calloc(sizeof(assmt_type), 1);
}

/* Is a cJSON number integral (so it should map to INT not DOUBLE)?   */
static int is_integral(double d)
{
    return d == floor(d) && fabs(d) < 2.0e9;
}

/* Fill a value_type from a scalar/string cJSON node. */
static void fill_scalar(value_type *out, cJSON *v)
{
    if (cJSON_IsNumber(v)) {
        if (is_integral(v->valuedouble)) {
            out->Type = INT;
            out->u.IntVal = (int)v->valuedouble;
        } else {
            out->Type = DOUBLE;
            out->u.DoubleVal = v->valuedouble;
        }
    } else if (cJSON_IsString(v)) {
        out->Type = CHARPTR;
        out->u.CharPtrVal = stralloc(v->valuestring);
    } else if (cJSON_IsBool(v)) {
        out->Type = INT;
        out->u.IntVal = cJSON_IsTrue(v) ? 1 : 0;
    } else {
        out->Type = BAD;
        printf("[json] unsupported scalar value for '%s'\n",
               v->string ? v->string : "?");
        SAIL_ERROR_FLAG = TRUE;
    }
}

/* Read a 3-element numeric array into x,y,z. Returns 1 on success. */
static int read_vec3(cJSON *arr, double *x, double *y, double *z)
{
    cJSON *a, *b, *c;
    if (!cJSON_IsArray(arr) || cJSON_GetArraySize(arr) != 3) return 0;
    a = cJSON_GetArrayItem(arr, 0);
    b = cJSON_GetArrayItem(arr, 1);
    c = cJSON_GetArrayItem(arr, 2);
    if (!cJSON_IsNumber(a) || !cJSON_IsNumber(b) || !cJSON_IsNumber(c)) return 0;
    *x = a->valuedouble; *y = b->valuedouble; *z = c->valuedouble;
    return 1;
}

/* forward */
static assmt_type *assmts_from_object(cJSON *obj);
static rvalue_type rvalue_from_json(cJSON *v);

/* Build an EXPRLST value chain from an array of scalars/strings (flags). */
static value_type *exprlist_from_array(cJSON *arr)
{
    value_type *head = NULL, *tail = NULL;
    cJSON *item;
    cJSON_ArrayForEach(item, arr) {
        value_type *vv = new_value();
        fill_scalar(vv, item);
        vv->next = NULL;
        if (head == NULL) head = tail = vv;
        else { tail->next = vv; tail = vv; }
    }
    return head;
}

/* Turn one JSON value into an rvalue_type, exactly as the grammar did. */
static rvalue_type rvalue_from_json(cJSON *v)
{
    rvalue_type rv;
    memset(&rv, 0, sizeof(rv));

    if (cJSON_IsNumber(v) || cJSON_IsString(v) || cJSON_IsBool(v)) {
        value_type *vv = new_value();
        fill_scalar(vv, v);
        vv->next = NULL;
        rv.Type = EXPRLST;
        rv.data.ExprLst = vv;
        return rv;
    }

    if (cJSON_IsArray(v)) {
        /* array of scalars/strings -> expression (flag) list */
        rv.Type = EXPRLST;
        rv.data.ExprLst = exprlist_from_array(v);
        return rv;
    }

    if (cJSON_IsObject(v)) {
        cJSON *xyz = cJSON_GetObjectItemCaseSensitive(v, "xyz");
        cJSON *rgb = cJSON_GetObjectItemCaseSensitive(v, "rgb");
        cJSON *kind = cJSON_GetObjectItemCaseSensitive(v, "kind");

        if (xyz != NULL) {
            value_type *vv = new_value();
            double x, y, z;
            if (read_vec3(xyz, &x, &y, &z)) {
                vv->Type = XYZ;
                vv->u.XYZVal.x = x; vv->u.XYZVal.y = y; vv->u.XYZVal.z = z;
            } else {
                vv->Type = BAD;
                printf("[json] 'xyz' must be an array of 3 numbers\n");
                SAIL_ERROR_FLAG = TRUE;
            }
            vv->next = NULL;
            rv.Type = EXPRLST;
            rv.data.ExprLst = vv;
            return rv;
        }
        if (rgb != NULL) {
            value_type *vv = new_value();
            double x, y, z;
            if (read_vec3(rgb, &x, &y, &z)) {
                vv->Type = RGB;
                vv->u.RGBVal.r = x; vv->u.RGBVal.g = y; vv->u.RGBVal.b = z;
            } else {
                vv->Type = BAD;
                printf("[json] 'rgb' must be an array of 3 numbers\n");
                SAIL_ERROR_FLAG = TRUE;
            }
            vv->next = NULL;
            rv.Type = EXPRLST;
            rv.data.ExprLst = vv;
            return rv;
        }
        if (kind != NULL && cJSON_IsString(kind)) {
            /* nested module: paramap{}, bbox{}, attribute{}, range{}, ... */
            int tok = kind_token(kind->valuestring);
            cJSON *nm = cJSON_GetObjectItemCaseSensitive(v, "name");
            char *name = (nm && cJSON_IsString(nm)) ? stralloc(nm->valuestring) : NULL;
            assmt_type *list = assmts_from_object(v);
            if (tok < 0) {
                printf("[json] unknown module kind '%s'\n", kind->valuestring);
                SAIL_ERROR_FLAG = TRUE;
                rv.Type = EMPTY;
                return rv;
            }
            rv.Type = MODULE;
            rv.data.Module = NewSailModule(tok, name, list);
            return rv;
        }

        /* plain object -> assignment block (ASSMTS): mapx, solid, fade ... */
        rv.Type = ASSMTS;
        rv.data.Assmts = assmts_from_object(v);
        return rv;
    }

    /* null or unsupported */
    rv.Type = EMPTY;
    return rv;
}

/* Does a JSON array hold objects (=> repeated assignments) rather than
   scalars (=> a single expression list)? */
static int array_of_objects(cJSON *arr)
{
    cJSON *first = cJSON_GetArrayItem(arr, 0);
    return first != NULL && cJSON_IsObject(first);
}

/* Build an assmt_type list from a JSON object's members, skipping the
   reserved "kind"/"name" keys. */
static assmt_type *assmts_from_object(cJSON *obj)
{
    assmt_type *head = NULL, *tail = NULL;
    cJSON *member;

    cJSON_ArrayForEach(member, obj) {
        const char *key = member->string;
        if (key == NULL) continue;
        /* reserved/ignored keys: module tag, name, and free-form comments
           ("comment" or any key beginning with "//" or "_") so scenes can be
           annotated despite JSON having no native comment syntax. */
        if (strcmp(key, "kind") == 0 || strcmp(key, "name") == 0) continue;
        if (strcmp(key, "comment") == 0) continue;
        if (key[0] == '_' || (key[0] == '/' && key[1] == '/')) continue;

        if (cJSON_IsArray(member) && array_of_objects(member)) {
            /* repeated assignment: one assmt node per array element */
            cJSON *item;
            cJSON_ArrayForEach(item, member) {
                assmt_type *a = new_assmt();
                a->lvalue = stralloc(key);
                a->rvalue = rvalue_from_json(item);
                a->next = NULL;
                if (head == NULL) head = tail = a;
                else { tail->next = a; tail = a; }
            }
        } else {
            assmt_type *a = new_assmt();
            a->lvalue = stralloc(key);
            a->rvalue = rvalue_from_json(member);
            a->next = NULL;
            if (head == NULL) head = tail = a;
            else { tail->next = a; tail = a; }
        }
    }
    return head;
}

/* Optional top-level colortable: { "name": {"rgb":[r,g,b]}, ... } */
static void load_colortable(cJSON *table)
{
    cJSON *entry;
    cJSON_ArrayForEach(entry, table) {
        double r, g, b;
        cJSON *rgb = cJSON_IsObject(entry)
                   ? cJSON_GetObjectItemCaseSensitive(entry, "rgb") : NULL;
        if (rgb && read_vec3(rgb, &r, &g, &b)) {
            rgb_type c; c.r = r; c.g = g; c.b = b;
            AddRGBConst(entry->string, c);
        } else {
            printf("[json] colortable entry '%s' must be {\"rgb\":[r,g,b]}\n",
                   entry->string ? entry->string : "?");
            SAIL_ERROR_FLAG = TRUE;
        }
    }
}

/* Process one top-level module object. */
static void process_module(cJSON *obj)
{
    cJSON *kind, *nm;
    int tok;
    char *name;
    assmt_type *list;

    if (!cJSON_IsObject(obj)) {
        printf("[json] scene entries must be objects\n");
        SAIL_ERROR_FLAG = TRUE;
        return;
    }
    kind = cJSON_GetObjectItemCaseSensitive(obj, "kind");
    if (!kind || !cJSON_IsString(kind)) {
        printf("[json] scene entry is missing a \"kind\" string\n");
        SAIL_ERROR_FLAG = TRUE;
        return;
    }
    tok = kind_token(kind->valuestring);
    if (tok < 0) {
        printf("[json] unknown module kind '%s'\n", kind->valuestring);
        SAIL_ERROR_FLAG = TRUE;
        return;
    }
    if (tok == YCOLORTABLE) {
        cJSON *colors = cJSON_GetObjectItemCaseSensitive(obj, "colors");
        if (colors) load_colortable(colors);
        return;
    }

    nm = cJSON_GetObjectItemCaseSensitive(obj, "name");
    name = (nm && cJSON_IsString(nm)) ? stralloc(nm->valuestring) : NULL;
    list = assmts_from_object(obj);
    NewSailModule(tok, name, list);
}

/* Slurp an entire file into a malloc'd, NUL-terminated buffer. */
static char *read_file(const char *path)
{
    FILE *fp;
    long len;
    char *buf;
    size_t got;

    fp = fopen(path, "rb");
    if (fp == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len < 0) { fclose(fp); return NULL; }
    buf = (char *)malloc((size_t)len + 1);
    if (buf == NULL) { fclose(fp); return NULL; }
    got = fread(buf, 1, (size_t)len, fp);
    buf[got] = '\0';
    fclose(fp);
    return buf;
}

/*
======================================
    InitSail()  -- JSON replacement for the old lex/yacc driver.
    Declared in sailpub.h as: int InitSail(char *);
======================================
*/
int InitSail(char *filename)
{
    char *text;
    cJSON *root, *scene, *colortable;

    text = read_file(filename);
    if (text == NULL) {
        printf("Error opening scene file: %s\n", filename);
        return FALSE;
    }
    snprintf(SAIL_CurrentFile, sizeof(SAIL_CurrentFile), "%s", filename);

    root = cJSON_Parse(text);
    if (root == NULL) {
        const char *err = cJSON_GetErrorPtr();
        printf("JSON parse error in %s near: %.40s\n",
               filename, err ? err : "(unknown)");
        free(text);
        return FALSE;
    }

    InitNoise(); /* initialize noise lattices (was done in translat.c) */

    /* Accept either a bare array, or an object with scene/colortable. */
    if (cJSON_IsArray(root)) {
        scene = root;
    } else if (cJSON_IsObject(root)) {
        colortable = cJSON_GetObjectItemCaseSensitive(root, "colortable");
        if (colortable) load_colortable(colortable);
        scene = cJSON_GetObjectItemCaseSensitive(root, "scene");
        if (scene == NULL) {
            printf("[json] top-level object needs a \"scene\" array\n");
            cJSON_Delete(root);
            free(text);
            return FALSE;
        }
    } else {
        printf("[json] top level must be an array or object\n");
        cJSON_Delete(root);
        free(text);
        return FALSE;
    }

    if (!cJSON_IsArray(scene)) {
        printf("[json] \"scene\" must be an array of module objects\n");
        cJSON_Delete(root);
        free(text);
        return FALSE;
    }

    {
        cJSON *mod;
        cJSON_ArrayForEach(mod, scene) {
            process_module(mod);
        }
    }

    /* The built modules keep pointers into stralloc'd copies, not into the
       cJSON tree, so it is safe to free the parsed document now. */
    cJSON_Delete(root);
    free(text);

    return (SAIL_ERROR_FLAG == TRUE) ? FALSE : TRUE;
}
