/*
  Jasjot Sumal
  CMPS 371, Fall 2014
*/
#include  <math.h>
#include  "vecFxns.h"

void vecCrossProduct (Vec v0, Vec v1, Vec dest)
{
	dest[0] = v0[1]*v1[2] - v1[1]*v0[2];
	dest[1] = v0[2]*v1[0] - v1[2]*v0[0];
	dest[2] = v0[0]*v1[1] - v1[0]*v0[1];
}

Flt vecDotProduct (Vec v0, Vec v1)
{
	return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
}

void vecZero (Vec v)
{
	v[0] = v[1] = v[2] = 0.0;
}

Flt vecDist (Vec v0, Vec v1)
{
  Vec v2;
  vecSub(v0, v1, v2);
  return vecLength(v2);
}

void vecNegate (Vec v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void vecMake (Flt a, Flt b, Flt c, Vec v)
{
	v[0] = a;
	v[1] = b;
	v[2] = c;
}

void vecCopy (Vec source, Vec dest)
{
	dest[0] = source[0];
	dest[1] = source[1];
	dest[2] = source[2];
}

Flt vecLength (Vec v)
{
	return sqrt(vecDotProduct(v, v));
}

void vecNormalize (Vec v)
{
	Flt len = vecLength(v);
	if (len == 0.0) {
		vecMake(0,0,1,v);
		return;
	}
	len = 1.0 / len;
	v[0] *= len;
	v[1] *= len;
	v[2] *= len;
}

void vecSub (Vec To, Vec From, Vec dest)
{
	dest[0] = To[0] - From[0];
	dest[1] = To[1] - From[1];
	dest[2] = To[2] - From[2];
}

void getTriangleNormal (Vec tri[3], Vec norm)
{
	Vec v0,v1;
	vecSub(tri[1], tri[0], v0);
	vecSub(tri[2], tri[0], v1);
	vecCrossProduct(v0, v1, norm);
	vecNormalize(norm);
}

void getNormal (Vec v0, Vec v1, Vec v2, Vec norm)
{
	Vec t0, t1;
	vecSub(v0, v1, t0);
	vecSub(v2, v1, t1);
	vecCrossProduct(t0, t1, norm);
	vecNormalize(norm);
}


