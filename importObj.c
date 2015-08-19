/*
  Jasjot Sumal
  CMPS 371, Fall 2014
*/
#include  <stdlib.h>    // malloc, free
#include  <stdio.h>     // FILE
#include  <ctype.h>     // isspace
//#include  "vecFxns.h"
#include  "importObj.h"

#define rnd() (Flt)rand() / (Flt)RAND_MAX

void initImportTri (void)
{
  if (ntri == 0) return;

  unsigned int i, j;
  for (i = 0; i < ntri; i++) {
      tri[i].norm[0] =
      tri[i].norm[1] =
      tri[i].norm[2] = 0.0f;
      for (j = 0; j < 3; j++) {
        tri[i].vertex[j][0] =
        tri[i].vertex[j][1] =
        tri[i].vertex[j][2] = 0.0f;
      }
      tri[i].color[0] =
      tri[i].color[1] =
      tri[i].color[2] = 0.0f;
  }
  ntri = 0;
}

unsigned int importTriObj(char * fname_str, Vec rgb, int rndON)
{
  TriObj *o;
  char ts[200], *ptr;
  int nverts=0, nfaces=0;
  int i, f;
  Vec *vert = (Vec *) malloc(MAX_TRI);
  iVec *face = (iVec *) malloc(MAX_TRI);
  const int startidx = ntri;

  FILE *fin = fopen(fname_str, "r");
  if (!fin) printf("file not open\n");
  if (fin) {
    while(1) {
      if (feof(fin)) break;
      fgets(ts,100,fin);
      ptr = ts;
      if (*ptr == 'v') {
        ptr = findSpace(ptr)+1;
        vert[nverts][0] = atof(ptr);
        ptr = findSpace(ptr)+1;
        vert[nverts][2] = atof(ptr);
        ptr = findSpace(ptr)+1;
        vert[nverts][1] = atof(ptr);
        nverts++;
        continue;
      }
      if (*ptr == 'f') {
        ptr = findSpace(ptr)+1;
        face[nfaces][0] = atoi(ptr);
        ptr = findSpace(ptr)+1;
        face[nfaces][1] = atoi(ptr);
        ptr = findSpace(ptr)+1;
        face[nfaces][2] = atoi(ptr);
        nfaces++;
      }
    }
    fclose(fin);
  }
  // insert tri object at end of array
  for (i=0; i<nfaces; i++) {
    o = &tri[ntri];
    f=face[i][0]-1;
    vecMake(vert[f][0],vert[f][1],vert[f][2],o->vertex[1]);
    f=face[i][1]-1;
    vecMake(vert[f][0],vert[f][1],vert[f][2],o->vertex[0]);
    f=face[i][2]-1;
    vecMake(vert[f][0],vert[f][1],vert[f][2],o->vertex[2]);
    if (rndON) {
      vecMake(rgb[0]*0.66f + rnd()*rgb[0]*0.33f,
              rgb[1]*0.66f + rnd()*rgb[1]*0.33f,
              rgb[2]*0.66f + rnd()*rgb[2]*0.33f, o->color);
    }
    else {
      vecMake(rgb[0],rgb[1],rgb[2],o->color);
    }
    getTriangleNormal(o->vertex, o->norm);
    ntri++;
  }

  free(vert);
  free(face);
  return (ntri-startidx);   // return number of triangles in object
}

char *findSpace(char *ptr) {
  while (*ptr && !isspace(*ptr))
    ptr++;
  return ptr;
}


