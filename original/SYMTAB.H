/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | symtab.h
    Version:  | 1.0 SAIL, 1.0 SAILER
              |
    Date:     | 28 Jun 1993
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | header file for symtab.c
              |
=============================================================================
*/

typedef struct symtab_struct
{
    struct symtabEntry_struct *topSymtabEntry;
    struct symtabEntry_struct *list;
    int listcount;
} symtab_type;

typedef struct symtabEntry_struct
{
    char name[32];                  /* the name of the entry */
    void *info;                     /* the data stored in this function */
    void (*freemem_funct)(void *);  /* if we wish to delete an object we need */
                                    /* to know how to delete it */
    symtab_type *symtab;            /* the symtab_struct that I belong to */
    int id;                         /* a unique ID for this entry */
    int class;                      /* used by the client */
    struct symtabEntry_struct *next;/* next item in list */
} symtabEntry;

/* prototypes */

symtab_type * AddEntry(symtab_type *symtab,symtabEntry *data);
symtabEntry * FindEntryByName(symtab_type *symtab,char *);
symtabEntry * FindEntryByID(symtab_type *symtab,int id);
symtab_type * GetSymtab(symtabEntry *entry);
int DelEntry(symtab_type *symtab,symtabEntry *entryPtr);



