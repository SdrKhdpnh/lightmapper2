/****************************************************************************
 * 			sunlight.cc: a directional light with soft shadows
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

#include <core_api/light.h>
#include <core_api/environment.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

class sunLight_t : public light_t
{
  public:
	sunLight_t(vector3d_t dir, const color_t &col, CFLOAT inte, float angle, int nSamples);
	virtual void init(scene_t &scene);
	virtual color_t totalEnergy() const { return color * worldRadius*worldRadius * M_PI; }
	virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
	virtual bool diracLight() const { return false; }
	virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
	virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const{ return false; }
	virtual bool canIntersect() const{ return true; }
	virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
	virtual int nSamples() const { return samples; }
	static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
  protected:
	point3d_t worldCenter;
	color_t color, colPdf;
	vector3d_t direction, du, dv;
	float intensity;
	float pdf, invpdf;
	double cosAngle;
	int samples;
	PFLOAT worldRadius;
//	bool infinite;
//	int majorAxis; //!< the largest component of direction
};

sunLight_t::sunLight_t(vector3d_t dir, const color_t &col, CFLOAT inte, float angle, int nSamples):
	direction(dir), samples(nSamples)
{
	color = col * inte;
	intensity = color.energy();
	direction.normalize();
	createCS(dir, du, dv);
//	vector3d_t &d = direction;
//	majorAxis = (d.x>d.y) ? ((d.x>d.z) ? 0 : 2) : ((d.y>d.z) ? 1:2 );
//	sinAngle = sin(angle);
	if(angle > 80.f) angle = 80.f;
	cosAngle = cos((double)angle*M_PI/180.0);
	invpdf = (2.f * M_PI * (1.f - cosAngle));
	pdf = 1.0 / invpdf;
	colPdf = color*pdf;
}

void sunLight_t::init(scene_t &scene)
{
	// calculate necessary parameters for photon mapping
	bound_t w=scene.getSceneBound();
	worldRadius = 0.5 * (w.g - w.a).length();
	worldCenter = 0.5 * (w.a + w.g);
}

bool sunLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	// FIXME: only lighten points in front of the sun, sun's position is needed for that
	//vector3d_t lightDir = wi.from - sun.pos;
	//if (direction * lightDir < 0) return false;
	
	//sample direction uniformly inside cone:
	wi.dir = sampleCone(direction, du, dv, cosAngle, s.s1, s.s2);
	wi.tmax = -1.f;
	
	s.col = colPdf;
	// ipdf: inverse of uniform cone pdf; calculated in constructor.
	s.pdf = pdf;
	
	return true;
}

bool sunLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	PFLOAT cosine = ray.dir*direction;
	if(cosine < cosAngle) return false;
	col = colPdf;
	t = -1;
	ipdf =invpdf;
}

color_t sunLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	vector3d_t ldir = sampleCone(direction, du, dv, cosAngle, s3, s4);
	vector3d_t du2, dv2;
	minRot(direction, du, ldir, du2, dv2);
	PFLOAT u, v;
	ShirleyDisk(s1, s2, u, v);
	
	ipdf = M_PI * worldRadius*worldRadius; // uhm...for sure not correct, but only the result of ipdf*color really matters...
	ray.from = worldCenter + worldRadius*(u*du2 + v*dv2 + ldir);
	ray.dir = -ldir;
	return color;
}


light_t *sunLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t dir(0.0, 0.0, 1.0);
	color_t color(1.0);
	CFLOAT power = 1.0;
	float angle = 0.27; //angular (half-)size of the real sun;
	int samples = 4;
//	bool shadow = true;

	params.getParam("direction",dir);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("angle",angle);
	params.getParam("samples",samples);
//	params.getParam("cast_shadows",shadow);

	return new sunLight_t(vector3d_t(dir.x, dir.y, dir.z), color, power, angle, samples);
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("sunlight",sunLight_t::factory);
}

}

__END_YAFRAY
