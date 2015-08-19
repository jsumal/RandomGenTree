/*
  Jasjot Sumal
  CMPS 371, Fall 2014
*/
#include  "vecFxns.h"

#ifndef _IMPORTOBJ_H_
#define _IMPORTOBJ_H_

#define MAX_TRI 6000            // Max triangle objects

typedef struct t_triobj {
  Vec norm;
  Vec vertex[3];
  Vec color;
} TriObj;

TriObj tri[MAX_TRI];    // Contains triangle of object after import
unsigned int ntri;

void          initImportTri     (void);
unsigned int  importTriObj      (char *, Vec, int);
char          *findSpace        (char *);

#endif
