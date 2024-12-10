
#include <core_api/vector3d.h>

__BEGIN_YAFRAY

float AS_Aniso_D(vector3d_t h, PFLOAT e_u, PFLOAT e_v)
{
	PFLOAT exponent = (e_u * h.x*h.x + e_v * h.y*h.y)/(1.f - h.z*h.z);
	return sqrt( (e_u + 1.f)*(e_v + 1.f) ) * std::pow(std::abs(h.z), exponent);
}

float AS_Aniso_Pdf(vector3d_t h, PFLOAT cos_w_H, PFLOAT e_u, PFLOAT e_v)
{
	PFLOAT exponent = (e_u * h.x*h.x + e_v * h.y*h.y)/(1.f - h.z*h.z);
	//again, our PDFs are multiplied by Pi...
	return sqrt( (e_u + 1.f)*(e_v + 1.f) ) * std::pow(std::abs(h.z), exponent) / (2.f * 4.f * cos_w_H);
}

inline void sample_quadrant(vector3d_t &H, float s1, float s2, PFLOAT e_u, PFLOAT e_v)
{
	PFLOAT phi = atan(sqrt((e_u + 1.f)/(e_v + 1.f)) * tan(M_PI * s1 * 0.5f));
	PFLOAT cos_phi = cos(phi), sin_phi, cos_theta, sin_theta;
	PFLOAT c_2 = cos_phi*cos_phi;
	PFLOAT s_2 = 1.f - c_2;
	cos_theta = std::pow((PFLOAT)s2, 1.f/(e_u*c_2 + e_v*s_2 + 1.f));
	sin_theta = sqrt(std::max(PFLOAT(0.f), 1.f - cos_theta*cos_theta));
	sin_phi = sqrt(std::max(PFLOAT(0.f), 1.f - cos_phi*cos_phi));
	H.x = sin_theta * cos_phi;
	H.y = sin_theta * sin_phi;
	H.z = cos_theta;
}

void AS_Aniso_Sample(vector3d_t &H, float s1, float s2, PFLOAT e_u, PFLOAT e_v)
{
	if(s1 < 0.25f)
	{
		sample_quadrant(H, 4.f*s1, s2, e_u, e_v);
	}
	else if(s1 < 0.5f)
	{
		sample_quadrant(H, 4.f*(0.5f - s1), s2, e_u, e_v);
		H.x = -H.x;
	}
	else if(s1 < 0.75f)
	{
		sample_quadrant(H, 4.f*(s1 - 0.5f), s2, e_u, e_v);
		H.x = -H.x;
		H.y = -H.y;
	}
	else
	{
		sample_quadrant(H, 4.f*(1.f - s1), s2, e_u, e_v);
		H.y = -H.y;
	}
}



__END_YAFRAY
