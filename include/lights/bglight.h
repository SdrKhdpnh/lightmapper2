/****************************************************************************
 * 			bglight.h: a light source using the background
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef Y_BACKGROUNDLIGHT_H
#define Y_BACKGROUNDLIGHT_H

#include <core_api/light.h>

__BEGIN_YAFRAY

class background_t;
class pdf1D_t;

class bgLight_t : public light_t
{
	public:
		bgLight_t(background_t *bg, int sampl);
		virtual ~bgLight_t();
		virtual void init(scene_t &scene);
		virtual color_t totalEnergy() const;
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual bool diracLight() const { return false; }
		//virtual bool illumSample(const surfacePoint_t &sp, float s1, float s2, color_t &col, float &ipdf, ray_t &wi) const;
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi)const { return false; }
		virtual float illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const;
		virtual int nSamples() const { return samples; }
		virtual bool canIntersect() const{ return true; }
		virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
//	static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		void initIS();
		void sample_dir(float s1, float s2, vector3d_t &dir, float &pdf) const;
		float dir_pdf(const vector3d_t dir) const;
		pdf1D_t *uDist, *vDist;
		int samples;
		int nv; //!< gives the array size of uDist
		point3d_t worldCenter;
		float worldRadius;
		float area, invArea;
		background_t *background;
};

__END_YAFRAY

#endif // Y_BACKGROUNDLIGHT_H
