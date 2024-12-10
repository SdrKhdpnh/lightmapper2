
#ifndef Y_SAMPLEUTILS_H
#define Y_SAMPLEUTILS_H

#include <core_api/ray.h>
#include <algorithm>
#include <string.h>

__BEGIN_YAFRAY

//! Sample a cosine-weighted hemisphere given the the coordinate system built by N, Ru, Rv.

vector3d_t inline SampleCosHemisphere(const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv, float s1, float s2)
{
	PFLOAT z1 = s1;
	PFLOAT z2 = s2*2.f*M_PI;
	return (Ru*cos(z2) + Rv*sin(z2))*sqrt(1.0-z1) + N*sqrt(z1);
}

//! Uniform sample a sphere

vector3d_t inline SampleSphere(float s1, float s2)
{
	vector3d_t dir;
	dir.z = 1.0f - 2.0f*s1;
	PFLOAT r = 1.0f - dir.z*dir.z;
	if(r>0.0f)
	{
		r = sqrt(r);
		PFLOAT a = 2.0f * M_PI * s2;
		dir.x = cos(a) * r;
		dir.y = sin(a) * r;
	}
	else
	{
		dir.x = 0.0f;
		dir.y = 0.0f;
	}
	return dir;
}

//! uniformly sample a cone. Using doubles because for small cone angles the cosine is very close to one...

vector3d_t inline sampleCone(const vector3d_t &D, const vector3d_t &U, const vector3d_t &V, double maxCosAng, PFLOAT s1, PFLOAT s2)
{
	double cosAng = 1.0 - (1.0-(double)maxCosAng) * (double)s2;
	double sinAng = sqrt(1.0 - cosAng*cosAng);
	PFLOAT t1 = 2.0*M_PI*s1;
	return (U*cos(t1) + V*sin(t1))*(PFLOAT)sinAng + D*(PFLOAT)cosAng;
}


void inline CumulateStep1dDF(const float *f, int nSteps, float *integral, float *cdf)
{
	int i;
	double c = 0.0, delta = 1.0/(double)nSteps;
	cdf[0] = 0.0;
	for (i = 1; i < nSteps+1; ++i)
	{
		c += (double)f[i-1] * delta;
		cdf[i] = float(c);
	}
	*integral = (float)c;// * delta;
	for (i = 1; i < nSteps+1; ++i)
		cdf[i] /= *integral;
}

/*! class that holds a 1D probability distribution function (pdf) and is also able to
	take samples from it. In order to do this the cumulative distribution function (cdf)
	is also calculated on construction.
*/

class pdf1D_t
{
	public:
	pdf1D_t() {}
	pdf1D_t(float *f, int n) {
		func = new float[n];
		cdf = new float[n+1];
		count = n;
		memcpy(func, f, n*sizeof(float));
		CumulateStep1dDF(func, n, &integral, cdf);
		invIntegral = 1.f / integral;
		invCount = 1.f / count;
	}
	~pdf1D_t(){ delete[] func, delete[] cdf; }
	float Sample(float u, float *pdf)const
	{
		// Find surrounding cdf segments
		float *ptr = std::lower_bound(cdf, cdf+count+1, u);
		int index = (int) (ptr-cdf-1);
		// Return offset along current cdf segment
		float delta = (u - cdf[index]) / (cdf[index+1] - cdf[index]);
		if(pdf) *pdf = func[index] * invIntegral;
		return index + delta;
	}
	// take a discrete sample.
	// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
	int DSample(float u, float *pdf)const
	{
		if(u == 0.f){ *pdf = func[0] * invIntegral; return 0; }
		float *ptr = std::lower_bound(cdf, cdf+count+1, u);
		int index = (int) (ptr-cdf-1);
		if(pdf) *pdf = func[index] * invIntegral;
		return index;
	}
	// Distribution1D Data
	float *func, *cdf;
	float integral, invIntegral, invCount;
	int count;
};

// rotate the coord-system D, U, V with minimum rotation so that D gets
// mapped to D2, i.e. rotate around D^D2.
// V is assumed to be D^U, accordingly V2 is D2^U2; all input vectors must be normalized!

void inline minRot(const vector3d_t &D, const vector3d_t &U,
				   const vector3d_t &D2, vector3d_t &U2, vector3d_t &V2)
{
	PFLOAT cosAlpha = D*D2;
	PFLOAT sinAlpha = sqrt(1 - cosAlpha*cosAlpha);
	vector3d_t v = D^D2;
	U2 = cosAlpha*U + (1.f-cosAlpha) * (v*U) + sinAlpha * (v^U);
	V2 = D2^U2;
}

inline vector3d_t reflect_plane(const vector3d_t &n,const vector3d_t &v)
{
	const PFLOAT vn = v*n;
	return (2.f*vn)*n - v;
}

//! Just a "modulo 1" float addition, assumed that both values are in range [0;1]

inline float addMod1(float a, float b)
{
	float s = a+b;
	return s>1 ? s-1.f : s;
}

__END_YAFRAY

#endif // Y_SAMPLEUTILS_H
