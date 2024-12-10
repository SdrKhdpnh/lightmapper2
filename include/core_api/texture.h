#ifndef Y_TEXTURE_H
#define Y_TEXTURE_H

#include <yafray_config.h>
#include "surface.h"

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT texture_t
{
	public :
		/* indicate wether the the texture is discrete (e.g. image map) or continuous */
		virtual bool discrete() const { return false; }
		/* indicate wether the the texture is 3-dimensional. If not, or p.z (and
		   z for discrete textures) are unused on getColor and getFloat calls */
		virtual bool isThreeD() const { return true; }
		virtual colorA_t getColor(const point3d_t &p) const = 0;
		virtual colorA_t getColor(int x, int y, int z) const { return colorA_t(0.f); }
		virtual CFLOAT getFloat(const point3d_t &p) const { return getColor(p).energy(); }
		virtual CFLOAT getFloat(int x, int y, int z) const { return getColor(x, y, z).energy(); }
		/* gives the number of values in each dimension for discrete textures */
		virtual void resolution(int &x, int &y, int &z) const { x=0, y=0, z=0; }
		virtual ~texture_t() {}
};

inline void angmap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	PFLOAT r = p.x*p.x + p.z*p.z;
	if (r!=0.f)  {
		r = 1.f/sqrt(r);
		if (p.y>1.f)
			r = 0;
		else if (p.y>=-1.f)
			r *= M_1_PI * acos(p.y);
	}
	if ((u = 0.5f - 0.5f*p.x*r)<0.f) u=0.f; else if (u>1.f) u=1.f;
	if ((v = 0.5f + 0.5f*p.z*r)<0.f) v=0.f; else if (v>1.f) v=1.f;
}

// slightly modified Blender's own functions,
// works better than previous functions which needed extra tweaks
inline void tubemap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	u = 0;
	v = 1 - (p.z + 1)*0.5;
	PFLOAT d = p.x*p.x + p.y*p.y;
	if (d>0) {
		d = 1/sqrt(d);
		u = 0.5*(1 - (atan2(p.x*d, p.y*d) *M_1_PI));
	}
}

inline void spheremap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	PFLOAT d = p.x*p.x + p.y*p.y + p.z*p.z;
	u = v= 0;
	if (d>0) {
		if ((p.x!=0) && (p.y!=0)) u = 0.5*(1 - atan2(p.x, p.y)*M_1_PI);
		v = acos(p.z/sqrt(d)) * M_1_PI;
	}
}

__END_YAFRAY

#endif // Y_TEXTURE_H
