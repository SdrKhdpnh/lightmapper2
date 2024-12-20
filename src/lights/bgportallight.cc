/****************************************************************************
 * 			meshlight.cc: a light source using a triangle mesh as shape
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
 
#include <limits>

#include <lights/bgportallight.h>
#include <core_api/background.h>
#include <core_api/environment.h>
#include <utilities/sample_utils.h>
#include <utilities/mcqmc.h>
#include <yafraycore/kdtree.h>

__BEGIN_YAFRAY

bgPortalLight_t::bgPortalLight_t(unsigned int msh, int sampl):
	objID(msh), samples(sampl), tree(0)
{
	mesh = 0;
	//initIS();
}

bgPortalLight_t::~bgPortalLight_t()
{
	delete areaDist;
	areaDist = 0;
	delete[] tris;
	if(tree) delete tree;
}

void bgPortalLight_t::initIS()
{
	nTris = mesh->numPrimitives();
	tris = new const triangle_t*[nTris];
	mesh->getPrimitives(tris);
	float *areas = new float[nTris];
	double totalArea = 0.0;
	for(int i=0; i<nTris; ++i)
	{
		areas[i] = tris[i]->surfaceArea();
		totalArea += areas[i];
	}
	areaDist = new pdf1D_t(areas, nTris);
	area = (float)totalArea;
	invArea = (float)(1.0/totalArea);
	//delete[] tris;
	delete[] areas;
	if(tree) delete tree;
	tree = new triKdTree_t(tris, nTris, -1, 1, 0.8, 0.33);
}

void bgPortalLight_t::init(scene_t &scene)
{
	bg = scene.getBackground();
	worldCenter = scene.getSceneBound().center();
	mesh = scene.getMesh(objID);
	if(mesh) initIS();
	std::cout << "bgPortalLight:init(): triangles:" << nTris << ", area:" << area << std::endl;
	// tell the mesh that a meshlight is associated with it (not sure if this is the best place though):
	mesh->setLight(this);
//	stats = new int[nTris];
//	for(int i=0; i<nTris; ++i) stats[i]=0;
}

void bgPortalLight_t::sampleSurface(point3d_t &p, vector3d_t &n, float s1, float s2) const
{
	float primPdf;
	int primNum = areaDist->DSample(s1, &primPdf);
	if(primNum >= areaDist->count){ std::cout << "meshLight sampling error!\n"; return; }
	float ss1, delta = areaDist->cdf[primNum+1];
	if(primNum > 0)
	{
		delta -= areaDist->cdf[primNum];
		ss1 = (s1 - areaDist->cdf[primNum]) / delta;
	}
	else ss1 = s1 / delta;
	tris[primNum]->sample(ss1, s2, p, n);
//	++stats[primNum];
}

color_t bgPortalLight_t::totalEnergy() const
{
	ray_t wo;
	wo.from = worldCenter;
	color_t energy, col;
	for(int i=0; i<1000; ++i) //exaggerated?
	{
		wo.dir = SampleSphere( ((float)i+0.5f)/1000.f, RI_vdC(i) );
		col = bg->eval(wo);
		for(int j=0; j<nTris; j++)
		{
			PFLOAT cos_n = -wo.dir * tris[j]->getNormal(); //not 100% sure about sign yet...
			if(cos_n > 0) energy += col*cos_n*tris[j]->surfaceArea();
		}
	}
	energy *= M_PI/(CFLOAT)1000;
	return energy;
}

bool bgPortalLight_t::illumSample(const surfacePoint_t &sp, float s1, float s2, color_t &col, float &ipdf, ray_t &wi) const
{
	vector3d_t n;
	point3d_t p;
	sampleSurface(p, n, s1, s2);
	
	vector3d_t ldir = p - sp.P;
	PFLOAT dist_sqr = ldir.lengthSqr();
	PFLOAT dist = sqrt(dist_sqr);
	if(dist <= 0.0) return false;
	PFLOAT idist_sqr = 1.f/(dist_sqr);
	ldir *= 1.f/dist;
	PFLOAT cos_angle = ldir*(-n);
	//no light if point is behind area light surface and single sided!
	if(cos_angle <= 0) return false;
	
	// fill direction
	wi.tmax = dist;
	wi.dir = ldir;
	
	col = bg->eval(wi);
	// pdf = distance^2 / area * cos(norm, ldir); ipdf = 1/pdf;
	ipdf = idist_sqr * area * cos_angle * (1.f/M_PI);
	
	return true;
}

bool bgPortalLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	vector3d_t n;
	point3d_t p;
	sampleSurface(p, n, s.s1, s.s2);
	
	vector3d_t ldir = p - sp.P;
	//normalize vec and compute inverse square distance
	PFLOAT dist_sqr = ldir.lengthSqr();
	PFLOAT dist = sqrt(dist_sqr);
	if(dist <= 0.0) return false;
	ldir *= 1.f/dist;
	PFLOAT cos_angle = -(ldir*n);
	//no light if point is behind area light (single sided!)
	if(cos_angle <= 0) return false;
	
	// fill direction
	wi.tmax = dist;
	wi.dir = ldir;
	
	color_t col = bg->eval(wi);
	// pdf = distance^2 / area * cos(norm, ldir);
	s.pdf = dist_sqr*M_PI / (area * cos_angle);
	s.flags = flags;
	if(s.sp)
	{
		s.sp->P = p;
		s.sp->N = s.sp->Ng = n;
	}
	return true;
}

color_t bgPortalLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	vector3d_t normal, du, dv;
	ipdf = area;
	sampleSurface(ray.from, normal, s3, s4);
	createCS(normal, du, dv);
	
	ray.dir = SampleCosHemisphere(normal, du, dv, s1, s2);
	ray_t r2(ray.from, -ray.dir);
	return bg->eval(r2);
}

color_t bgPortalLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.areaPdf = invArea * M_PI;
	sampleSurface(s.sp->P, s.sp->Ng, s.s3, s.s4);
	s.sp->N = s.sp->Ng;
	vector3d_t du, dv;
	createCS(s.sp->Ng, du, dv);
	
	wo = SampleCosHemisphere(s.sp->Ng, du, dv, s.s1, s.s2);
	s.dirPdf = std::fabs(s.sp->Ng * wo);
	
	s.flags = flags;
	ray_t r2(s.sp->P, -wo);
	return bg->eval(r2);
}

bool bgPortalLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	if(!tree) return false;
	PFLOAT dis;
	unsigned char udat[PRIM_DAT_SIZE];
	triangle_t *hitt=0;
	if(ray.tmax<0) dis=std::numeric_limits<PFLOAT>::infinity();
	else dis=ray.tmax;
	// intersect with tree:
	if( ! tree->Intersect(ray, dis, &hitt, t, (void*)&udat[0]) ){ return false; }
	
	vector3d_t n = hitt->getNormal();
	PFLOAT cos_angle = ray.dir*(-n);
	if(cos_angle <= 0) return false;
	PFLOAT idist_sqr = 1.f / (t*t);
	ipdf = idist_sqr * area * cos_angle * (1.f/M_PI);
	col = bg->eval(ray);
	
	return true;
}

float bgPortalLight_t::illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const
{
	vector3d_t wo = sp.P - sp_light.P;
	PFLOAT r2 = wo.normLenSqr();
	float cos_n = wo * sp_light.Ng;
	return cos_n > 0 ? ( r2 * M_PI / (area * cos_n) ) : 0.f;
}

void bgPortalLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = invArea * M_PI;
	cos_wo = wo*sp.N;
	//if(sp.N * normal < 0.95f) std::cout << "whow!\n";
	dirPdf = cos_wo > 0.f ? cos_wo  : 0.f;
}


light_t* bgPortalLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	int samples = 4;
	int object = 0;

	params.getParam("object", object);
	params.getParam("samples", samples);
	
	return new bgPortalLight_t(object, samples);
}

__END_YAFRAY
