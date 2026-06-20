/*
=================== Surface Attributes Interpeted Language ===================
              |
    File:     | symtab.c
    Version:  | 2.0 SAIL, 2.0 SAILER
              |
    Date:     | 28 Jun 1993
    Author:   | David Fletcher
              |
    Edit date:|
    Editor:   |
              |
    Summery:  | Implements a simple symbol table to store objects in.
              | It uses a linked list to store objects. Each object may
              | store a linked list of sub objects.
              |
==============================================================================
*/


#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int GetID(void);

symtab_type * AddEntry(symtab_type *symtab,symtabEntry *data)
{
symtabEntry *curEntry;
symtab_type *curSymtab;


    /* replicate entry */
    curEntry = (symtabEntry *)malloc(sizeof(symtabEntry));
    *curEntry = *data;

    if (symtab == NULL)
    {
        /* first create a symtab_type */
        curSymtab = (symtab_type *) malloc(sizeof(symtab_type));
        curSymtab->topSymtabEntry = curEntry;
        curEntry->next = NULL;
    }
    else
    {
        curSymtab = symtab;
        curEntry->next = curSymtab->topSymtabEntry;
        curSymtab->topSymtabEntry = curEntry;
    }
    curEntry->id = GetID();
    curEntry->symtab = curSymtab;
    return(curSymtab);
}


symtabEntry * FindEntryByName(symtab_type *symtab,char *name)
{
symtabEntry *curEntry;

    curEntry = symtab->topSymtabEntry;

    while (curEntry && strcmp(name,curEntry->name) != 0)
    {
        curEntry = curEntry->next;
    }
    return(curEntry);
}


symtabEntry * FindEntryByID(symtab_type *symtab,int id)
{
symtabEntry *curEntry;

    curEntry = symtab->topSymtabEntry;

    while (curEntry && curEntry->id != id)
    {
        curEntry = curEntry->next;
    }
    return(curEntry);
}


/* this could be a macro */
symtab_type * GetSymtab(symtabEntry *entry)
{
    return(entry->symtab);
}


int DelEntry(symtab_type *symtab,symtabEntry *entryPtr)
{
    /* code later (no need now and i am lazy) */


    return(-1); /* no go since we did not do anything */
}

static int GetID()
{
static int counter=0;
    return(counter++);
}