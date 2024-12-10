
#ifndef Y_MICROFACET_H
#define Y_MICROFACET_H

#include <core_api/vector3d.h>

__BEGIN_YAFRAY

float AS_Aniso_D(vector3d_t h, PFLOAT e_u, PFLOAT e_v);
float AS_Aniso_Pdf(vector3d_t h, PFLOAT cos_w_H, PFLOAT e_u, PFLOAT e_v);
void AS_Aniso_Sample(vector3d_t &H, float s1, float s2, PFLOAT e_u, PFLOAT e_v);

static inline PFLOAT Blinn_D(PFLOAT cos_h, PFLOAT exponent)
{
	return (cos_h>0) ? (exponent+2.f) * /* (1.f/(2.f*M_PI)) * */ std::pow(std::fabs(cos_h), exponent) : 0.f;
}

static inline float Blinn_Pdf(PFLOAT costheta, PFLOAT cos_w_H, PFLOAT exponent)
{
	return ((exponent + 2.f) * std::pow(std::fabs(costheta), exponent)) / (2.f * /* M_PI *  */ 4.f * cos_w_H); //our PDFs are multiplied by Pi...
}

static inline PFLOAT SchlickFresnel(PFLOAT costheta, PFLOAT R)
{
	PFLOAT cm1 = 1.f - costheta;
	PFLOAT cm1_2 = cm1*cm1;
	return R + (1.f - R) * cm1*cm1_2*cm1_2;
}

static inline void Blinn_Sample(vector3d_t &H, float s1, float s2, PFLOAT exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	PFLOAT costheta = pow(s1, 1.f / (exponent+1.f));
	PFLOAT sintheta = sqrt(std::max(PFLOAT(0.f), 1.f - costheta*costheta));
	PFLOAT phi = s2 * 2.f * M_PI;
	H = vector3d_t(sintheta*sin(phi), sintheta*cos(phi), costheta); //returning directly the spherical coords would allow some optimization..
	//pdf = ((exponent + 2.f) * pow(costheta, exponent)) / (2.f * /* M_PI *  */ 4.f * (wo*H)); //our PDFs are multiplied by Pi...
}

#define TANGENT_U 1
#define TANGENT_V 2
#define RAW_VMAP 3

//! convert shading space vector to world space
static inline vector3d_t aniso_transform(const surfacePoint_t &sp, const vector3d_t &Hs, int mode)
{
	if(mode == TANGENT_U)
	{
		vector3d_t V = (sp.N ^ sp.dPdU).normalize();
		vector3d_t U = V ^ sp.N;
		return Hs.x*U + Hs.y*V + Hs.z*sp.N;
	}
	else if(mode == TANGENT_V)
	{
		vector3d_t U = (sp.dPdV ^ sp.N).normalize();
		vector3d_t V = sp.N ^ U;
		return Hs.x*U + Hs.y*V + Hs.z*sp.N;
	}
	else
	{
		return Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*sp.N;
	}
}

//! convert world space vector to shading space
static inline vector3d_t aniso_invtransf(const surfacePoint_t &sp, const vector3d_t &H, int mode)
{
	if(mode == TANGENT_U)
	{
		vector3d_t V = (sp.N ^ sp.dPdU).normalize();
		vector3d_t U = V ^ sp.N;
		return vector3d_t(H*U, H*V, H*sp.N);
	}
	else if(mode == TANGENT_V)
	{
		vector3d_t U = (sp.dPdV ^ sp.N).normalize();
		vector3d_t V = sp.N ^ U;
		return vector3d_t(H*U, H*V, H*sp.N);
	}
	else
	{
		return vector3d_t(H*sp.NU, H*sp.NV, H*sp.N);
	}
}

__END_YAFRAY

#endif // Y_MICROFACET_H
