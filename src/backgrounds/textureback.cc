/****************************************************************************
 * 			textureback.cc: a background using the texture class
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
 
#include <yafray_config.h>

#include <core_api/environment.h>
#include <core_api/background.h>
#include <core_api/texture.h>
#include <core_api/light.h>
#include <utilities/sample_utils.h>
#include <lights/bglight.h>

__BEGIN_YAFRAY

class textureBackground_t: public background_t
{
	public:
		enum PROJECTION { spherical=0, angular };
		textureBackground_t(const texture_t *texture, PROJECTION proj, bool doIBL, int nsam, CFLOAT bpower, float rot);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool filtered=false) const;
		virtual color_t eval(const ray_t &ray, bool filtered=false) const;
		virtual light_t* getLight() const { return envLight; }
		virtual ~textureBackground_t();
		static background_t *factory(paraMap_t &,renderEnvironment_t &);
	protected:
//		color_t sample(float s1, float s2, vector3d_t &dir, float &pdf) const;

		void initIS();
		const texture_t *tex;
		bool ibl; //!< indicate wether to do image based lighting
		PROJECTION project;
		pdf1D_t *uDist, *vDist;
		int nu, nv, iblSam;
		light_t *envLight;
		CFLOAT power;
		float rotation;
		PFLOAT sin_r, cos_r;
};

class constBackground_t: public background_t
{
	public:
		constBackground_t(color_t col);
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool filtered=false) const;
		virtual color_t eval(const ray_t &ray, bool filtered=false) const;
		virtual ~constBackground_t();
		static background_t *factory(paraMap_t &params,renderEnvironment_t &render);
	protected:
		color_t color;
};


class envLight_t : public light_t
{
  public:
	envLight_t(pdf1D_t *uDist, pdf1D_t *vDist, const texture_t *texture, int nsam, CFLOAT lpower=1.f, float rot=0.f);
	virtual void init(scene_t &scene);
	virtual color_t totalEnergy() const;
	virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
	virtual bool diracLight() const { return false; }
	virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
	virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi)const { return false; }
	virtual int nSamples() const { return samples; }
	virtual bool canIntersect() const{ return true; }
	virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
//	static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
  protected:
	color_t sample_dir(float s1, float s2, vector3d_t &dir, float &pdf) const;
	const pdf1D_t *uDist, *vDist;
	const texture_t *tex;
	int samples;
	int nv; //!< gives the array size of uDist
	point3d_t worldCenter;
	float worldRadius;
	float area, invArea;
	CFLOAT power;
	float rotation;
};

textureBackground_t::textureBackground_t(const texture_t *texture, PROJECTION proj, bool IBL, int nsam, CFLOAT bpower, float rot):
	tex(texture), ibl(IBL), project(proj), uDist(0), vDist(0), nu(0), nv(0), iblSam(nsam), envLight(0), power(bpower)
{
	rotation = rot / 360.f;
	sin_r = sin(2*M_PI*rot);
	cos_r = cos(2*M_PI*rot);
	if(ibl) initIS();
}

textureBackground_t::~textureBackground_t()
{
	if(uDist)
//	{
//		for(int i=0; i<nu; ++i) delete uDist[i];
//		delete[] uDist;
//	}
		delete[] uDist;
	if(vDist) delete vDist;
	if(envLight) delete envLight;
}

color_t textureBackground_t::operator() (const ray_t &ray, renderState_t &state, bool filtered) const
{
	PFLOAT u=0, v=0;
	// need transform, currently y & z swap to conform to Blender axes
	if (project==angular)
	{
		point3d_t dir(ray.dir);
		dir.x = ray.dir.x * cos_r + ray.dir.y * sin_r;
		dir.y = ray.dir.x * -sin_r + ray.dir.y * cos_r;
		angmap(dir, u, v);
	}
	else {
		// currently only other possible type is sphere
		spheremap(ray.dir, u, v);
		// v is upside down in this case
		v = 1.0-v;
		u += rotation;
	}
	return power * tex->getColor(point3d_t(u, v, 0.0f));
}

color_t textureBackground_t::eval(const ray_t &ray, bool filtered) const
{
	PFLOAT u=0, v=0;
	// need transform, currently y & z swap to conform to Blender axes
	if (project==angular)
	{
		point3d_t dir(ray.dir);
		dir.x = ray.dir.x * cos_r + ray.dir.y * sin_r;
		dir.y = ray.dir.x * -sin_r + ray.dir.y * cos_r;
		angmap(dir, u, v);
	}
	else {
		// currently only other possible type is sphere
		spheremap(ray.dir, u, v);
		// v is upside down in this case
		v = 1.0-v;
		u += rotation;
	}
	return power * tex->getColor(point3d_t(u, v, 0.0f));
}

void textureBackground_t::initIS()
{
	if(project != spherical)
	{
		envLight = new bgLight_t(this, iblSam);
		return;
	}
	if(tex->discrete())
	{
		int dummy;
		tex->resolution(nu, nv, dummy);

		float *func = new float[std::max(nu, nv)];
		uDist = new pdf1D_t[nv];
		
		// Compute sampling distribution for lines
		for (int y=0; y<nv; ++y) {
			float sin_phi = sin(M_PI * float(y+.5)/float(nv));
			for(int x=0; x<nu; ++x)
				func[x] = tex->getColor(x, y, 0).energy() * sin_phi;
			/*uDist[y] = */ new (uDist+y) pdf1D_t(func, nu);
		}
		// compute sampling distribution of image lines
		for (int y=0; y<nv; ++y)
			func[y] = uDist[y].integral;
		vDist = new pdf1D_t(func, nv);
		
		delete[] func;
	}
	else
	{
		nu = 360, nv= 180;
		float *func = new float[std::max(nu, nv)];
		float inu = 1.f/(float)nu, inv = 1.f/(float)nv;
		uDist = new pdf1D_t[nv];
		
		// Compute sampling distribution for lines
		for (int y=0; y<nv; ++y) {
			float sin_phi = sin(M_PI * float(y+.5)/float(nv));
			for(int x=0; x<nu; ++x)
			{
				point3d_t p((float)x+0.5f*inu, (float)y+0.5f*inv, 0);
				func[x] = tex->getColor(p).energy() * sin_phi;
			}
			/*uDist[y] = */ new (uDist+y) pdf1D_t(func, nu);
		}
		// compute sampling distribution of image lines
		for (int y=0; y<nv; ++y)
			func[y] = uDist[y].integral;
		vDist = new pdf1D_t(func, nv);
		
		delete[] func;
	}
	envLight = new envLight_t(uDist, vDist, tex, iblSam, power, rotation);
}
/*
color_t textureBackground_t::sample(float s1, float s2, vector3d_t &dir, float &pdf) const
{
	if(project != spherical)
	{
		std::cerr << "(background_HDRI): Hey you shouldn't call me!\n";
		return color_t(0.0);
	}
	float u, v;
	float pdfs[2];
	int iv, iu;
	v = vDist->Sample(s2, pdfs+1);
	//clamp...
	iv = (int)(v+0.4999);
	iv = (iv<0)? 0 : ((iv>nv-1) ? nv-1 : iv);
	
	u = uDist[iv]->Sample(s1, pdfs);
	iu = (int)(u+0.4999);
	
	u/=(float)nu; //replace with pdf1D_t->invCount...
	v/=(float)nv;
	
	//coordinates:
	float theta = v * M_PI;
	float phi = u * 2.0 * M_PI;
	float costheta = cos(theta), sintheta = sin(theta);
	float sinphi = sin(phi), cosphi = cos(phi);
	dir.y = -1.0 * sintheta * cosphi, dir.x = sintheta * sinphi, dir.z = -costheta;
	pdf = (pdfs[0] * pdfs[1]) / sintheta;// / (2. * M_PI * M_PI * sintheta);
	
	return tex->getColor(point3d_t(u, v, 0.0f));
}
*/

background_t* textureBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	const texture_t *tex=0;
	const std::string *texname=0;
	const std::string *mapping=0;
	PROJECTION pr = spherical;
	double power = 1.0, rot=0.0;
	bool IBL = false;
	int IBL_sam = 8; //quite arbitrary really...
	
	if( !params.getParam("texture", texname) )
	{
		std::cerr << "error: no texture given for texture background!";
		return 0;
	}
	tex = render.getTexture(*texname);
	if( !tex )
	{
		std::cerr << "error: texture '"<<*texname<<"' for textureback not existant!\n";
		return 0;
	}
	if( params.getParam("mapping", mapping) )
	{
		if(*mapping == "probe" || *mapping == "angular") pr = angular;
	}
	params.getParam("ibl", IBL);
	params.getParam("ibl_samples", IBL_sam);
	params.getParam("power", power);
	params.getParam("rotation", rot);
	return new textureBackground_t(tex, pr, IBL, IBL_sam, (CFLOAT)power, float(rot));
}

/*==================================================
envLight methods definition:
==================================================*/

envLight_t::envLight_t(pdf1D_t *uDistrib, pdf1D_t *vDistrib, const texture_t *texture, int nsam, CFLOAT lpower, float rot):
	uDist(uDistrib), vDist(vDistrib), tex(texture), samples(nsam), power(lpower), rotation(rot)
{
	nv = vDistrib->count;
}

// todo! also totalEnergy()!

color_t envLight_t::totalEnergy() const
{
	//return 0.0f;
	// maybe this one is correct:
	return vDist->integral * 2.0 * M_PI * M_PI * worldRadius * worldRadius;
}
color_t envLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	color_t pcol;
	float pdf, u, v;
	pcol = sample_dir(s3, s4, ray.dir, pdf);
	ray.dir = -ray.dir;
	vector3d_t U, V;
	createCS(ray.dir, U, V);
	ShirleyDisk(s1, s2, u, v);
	vector3d_t offs = u*U + v*V;
	//simply move shoot point by radius from disk...
	ray.from = worldCenter + worldRadius*offs - worldRadius*ray.dir;
	ipdf = M_PI * worldRadius*worldRadius/pdf;
	
	return pcol;
}

void envLight_t::init(scene_t &scene)
{
	bound_t w=scene.getSceneBound();
	worldCenter = 0.5 * (w.a + w.g);
	worldRadius = 0.5 * (w.g - w.a).length();
}

color_t envLight_t::sample_dir(float s1, float s2, vector3d_t &dir, float &pdf) const
{
	float u, v;
	float pdfs[2];
	int iv, iu;
	v = vDist->Sample(s2, pdfs+1);
	//clamp...
	iv = (int)(v+0.4999);
	iv = (iv<0)? 0 : ((iv>nv-1) ? nv-1 : iv);
	
	u = uDist[iv].Sample(s1, pdfs);
	iu = (int)(u+0.4999);
	
	u *= uDist[iv].invCount;
	v *= vDist->invCount;
	
	//coordinates:
	float theta = v * M_PI;
	float phi = (u - rotation) * 2.0 * M_PI;
	float costheta = cos(theta), sintheta = sin(theta);
	float sinphi = sin(phi), cosphi = cos(phi);
	dir.y = -1.0 * sintheta * cosphi; dir.x = sintheta * sinphi; dir.z = -costheta;
	pdf = (pdfs[0] * pdfs[1]) / (2.0f * M_PI * sintheta);
	
	return power * tex->getColor(point3d_t(u, v, 0.0f));
	
}

bool envLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	wi.tmax = -1.0;
	//test as reference!
/*	wi.dir = SampleCosHemisphere(sp.N, sp.NU, sp.NV, s1, s2);
	float u, v;
	spheremap(wi.dir, u, v);
	v = 1.0-v;
	ipdf = 1.0/std::abs(sp.N*wi.dir);
	col = tex->getColor(point3d_t(u, v, 0.0f));
	return true; */
	//end of test reference
	float u, v;
	float pdfs[2];
	int iv, iu;
	v = vDist->Sample(s.s2, pdfs+1);
	//clamp...
	iv = (int)(v+0.4999);
	iv = (iv<0)? 0 : ((iv>nv-1) ? nv-1 : iv);
	
	u = uDist[iv].Sample(s.s1, pdfs);
	iu = (int)(u+0.4999);
	
	u *= uDist[iv].invCount;
	v *= vDist->invCount;
	
	//coordinates:
	float theta = v * M_PI;
	float phi = (u - rotation) * 2.0 * M_PI;
	float costheta = cos(theta), sintheta = sin(theta);
	float sinphi = sin(phi), cosphi = cos(phi);
	wi.dir.y = -1.0 * sintheta * cosphi; wi.dir.x = sintheta * sinphi; wi.dir.z = -costheta;
	//ipdf = 2.0f * M_PI * sintheta / (pdfs[0] * pdfs[1]);
	s.pdf = (pdfs[0] * pdfs[1]) / (2.0f * M_PI * sintheta);
	
	s.col = power * tex->getColor(point3d_t(u, v, 0.0f));
	return true;
}

bool envLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
//	static int blubb=0;
	PFLOAT u=0, v=0;
	spheremap(ray.dir, u, v);
	// v is upside down in this case
	u += rotation;
	if(u>1.0) u -= 1.0;
	v = 1.0-v;
	
	float pdfs[2];
	int iv, iu;
	//clamp...
	iv = (int)(v*vDist->count+0.4999);
	iv = (iv<0)? 0 : ((iv>nv-1) ? nv-1 : iv);
	pdfs[1] = vDist->func[iv] * vDist->invIntegral;
	
	iu = (int)(u*uDist[iv].count+0.4999);
	iu = (iu<0)? 0 : ((iu >= uDist[iv].count) ? uDist[iv].count-1 : iu);
	pdfs[0] = uDist[iv].func[iu] * uDist[iv].invIntegral;
	
	//coordinates:
	float theta = v * M_PI;
	float sintheta = sin(theta);
	float pdf_prod = pdfs[0] * pdfs[1];
	if(pdf_prod < 1e-6f) return false;
	ipdf = 2.0f * M_PI * sintheta / (pdf_prod);
	
	col = power * tex->getColor(point3d_t(u, v, 0.0f));
//	if(++blubb<25) std::cout << "ratio:" << col.energy()*ipdf << std::endl;
	return true;
}

/* ========================================
/ minimalistic background...
/ ========================================= */

constBackground_t::constBackground_t(color_t col):color(col) {}
constBackground_t::~constBackground_t() {}

color_t constBackground_t::operator() (const ray_t &ray, renderState_t &state, bool filtered) const
{
	return color;
}

color_t constBackground_t::eval(const ray_t &ray, bool filtered) const
{
	return color;
}

background_t* constBackground_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	color_t col(0.f);
	double power = 1.0;
	params.getParam("color", col);
	params.getParam("power", power);
	return new constBackground_t(col*(CFLOAT)power);
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("textureback",textureBackground_t::factory);
		render.registerFactory("constant", constBackground_t::factory);
	}

}
__END_YAFRAY
