/*
  Jasjot Sumal
  CMPS 371, Fall 2014
*/
#ifndef _VECFXNS_H_
#define _VECFXNS_H_

typedef float Flt;
typedef Flt Vec[3];
typedef int iVec[3];

// ---- VECTOR FUNCTIONS ----
void  vecCrossProduct   (Vec, Vec, Vec);
Flt   vecDotProduct     (Vec, Vec);
void  vecZero           (Vec);
Flt   vecDist           (Vec, Vec);
void  vecNegate         (Vec);
void  vecMake           (Flt, Flt, Flt, Vec);
void  vecCopy           (Vec, Vec);
Flt   vecLength         (Vec);
void  vecNormalize      (Vec);
void  vecSub            (Vec, Vec, Vec);
void  getTriangleNormal (Vec[3], Vec);
void  getNormal         (Vec, Vec, Vec, Vec);

#endif
