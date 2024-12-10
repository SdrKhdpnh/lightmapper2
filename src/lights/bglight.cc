/****************************************************************************
 * 			bglight.cc: a light source using the background
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
 
#include <lights/bglight.h>
#include <core_api/background.h>
#include <core_api/texture.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

bgLight_t::bgLight_t(background_t *bg, int sampl): samples(sampl), background(bg)
{
	initIS();
}

bgLight_t::~bgLight_t()
{
	delete[] uDist;
	delete vDist;
}

void bgLight_t::initIS()
{
	nv= 360;
	float *func = new float[1024];
	float inu, inv = 1.f/(float)nv;
	uDist = new pdf1D_t[nv];
	
	// Compute sampling distribution for lines
	for (int y=0; y<nv; ++y)
	{
		float theta = (y+0.5f) * inv * M_PI;
		float costheta = cos(theta), sintheta = sin(theta);
		float circumf = sintheta;
		int nu = 2 + int(circumf*720);
		inu = 1.f/(float)nu;
		for(int x=0; x<nu; ++x)
		{
			ray_t ray;
			ray.from = point3d_t(0.f);
			
			float phi = (x+0.5f) * inu * 2.0 * M_PI;
			float cosphi = cos(phi), sinphi = sin(phi);
			ray.dir.y = -1.0 * sintheta * cosphi;
			ray.dir.x = sintheta * sinphi;
			ray.dir.z = -costheta;
			func[x] = background->eval(ray).energy() * sintheta;
		}
		new (uDist+y) pdf1D_t(func, nu);
	}
	// compute sampling distribution of image lines
	for (int y=0; y<nv; ++y)
		func[y] = uDist[y].integral;
	vDist = new pdf1D_t(func, nv);
	
	delete[] func;
}

void bgLight_t::sample_dir(float s1, float s2, vector3d_t &dir, float &pdf) const
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
	float phi = u * 2.0 * M_PI;
	float costheta = cos(theta), sintheta = sin(theta);
	float sinphi = sin(phi), cosphi = cos(phi);
	dir.y = -1.0 * sintheta * cosphi; dir.x = sintheta * sinphi; dir.z = -costheta;
	pdf = (pdfs[0] * pdfs[1]) / (2.0f * M_PI * sintheta);
}

bool bgLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	wi.tmax = -1.0;
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
	float phi = u * 2.0 * M_PI;
	float costheta = cos(theta), sintheta = sin(theta);
	float sinphi = sin(phi), cosphi = cos(phi);
	wi.dir.y = -1.0 * sintheta * cosphi; wi.dir.x = sintheta * sinphi; wi.dir.z = -costheta;
	//ipdf = 2.0f * M_PI * sintheta / (pdfs[0] * pdfs[1]);
	s.pdf = (pdfs[0] * pdfs[1]) / (2.0f * M_PI * sintheta);
	
	s.col = background->eval(wi);
	return true;
}

// dir points from surface point to background
float bgLight_t::dir_pdf(const vector3d_t dir) const
{
	PFLOAT u=0, v=0;
	spheremap(dir, u, v);
	// v is upside down in this case
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
	float sintheta = sin(theta); // sqrt(ray.dir.x*ray.dir.x + ray.dir.y*ray.dir.y)?
	
	return (pdfs[0] * pdfs[1]) / (2.0f * M_PI * sintheta );
}

bool bgLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	PFLOAT u=0, v=0;
	spheremap(ray.dir, u, v);
	// v is upside down in this case
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
	float sintheta = sin(theta); // sqrt(ray.dir.x*ray.dir.x + ray.dir.y*ray.dir.y)?
	float pdf_prod = pdfs[0] * pdfs[1];
	if(pdf_prod < 1e-6f) return false;
	ipdf = 2.0f * M_PI * sintheta / (pdf_prod);
	
	col = background->eval(ray);
	return true;
}

color_t bgLight_t::totalEnergy() const
{
	//return 0.0f;
	// maybe this one is correct:
	color_t energy = vDist->integral * 2.0 * M_PI * M_PI * worldRadius * worldRadius;
	return energy;
}

color_t bgLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	color_t pcol;
	float pdf, u, v;
	sample_dir(s3, s4, ray.dir, pdf);
	pcol = background->eval(ray);
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

color_t bgLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	color_t pcol;
	float pdf, u, v;
	sample_dir(s.s3, s.s4, wo, s.dirPdf);
	pcol = background->eval(ray_t(point3d_t(0,0,0), wo));
	wo = -wo;
	vector3d_t U, V;
	createCS(wo, U, V);
	ShirleyDisk(s.s1, s.s2, u, v);
	vector3d_t offs = u*U + v*V;
	//simply move shoot point by radius from disk...
	s.sp->P = worldCenter + worldRadius*offs - worldRadius*wo;
	s.sp->N = s.sp->Ng = wo;
	s.areaPdf = 1.f / (worldRadius*worldRadius);
	s.flags = flags;
	
	return pcol;
}

float bgLight_t::illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const
{
	vector3d_t dir = (sp_light.P - sp.P).normalize();
	return dir_pdf(dir);
}

void bgLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	cos_wo = 1.f;
	vector3d_t wi = -wo;
	dirPdf = dir_pdf(wi);
	areaPdf =  1.f / (worldRadius*worldRadius);
}

void bgLight_t::init(scene_t &scene)
{
	bound_t w=scene.getSceneBound();
	worldCenter = 0.5 * (w.a + w.g);
	worldRadius = 0.5 * (w.g - w.a).length();
}

__END_YAFRAY

