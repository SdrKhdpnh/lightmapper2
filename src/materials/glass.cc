/****************************************************************************
 * 			glass.cc: a dielectric material with dispersion, two trivial mats
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
#include <yafraycore/nodematerial.h>
#include <materials/roughglass.h>
#include <core_api/environment.h>
#include <yafraycore/spectrum.h>


__BEGIN_YAFRAY

class glassMat_t: public nodeMaterial_t
{
	public:
		glassMat_t(float IOR, color_t filtC, const color_t &srcol, double disp_pow, bool fakeS);
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, unsigned int &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const {return color_t(0.0);}
	//	virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, float s1, float s2)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const {return 0.f;}
		virtual bool isTransparent() const { return fakeShadow; }
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
								 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const;
		virtual bool volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp, const ray_t &ray, color_t &col)const;
	//	virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const;
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
	protected:
		shaderNode_t* bumpS;
		shaderNode_t *mirColS;
		color_t filterCol, specRefCol;
		color_t beer_sigma_a;
		float ior;
		bool absorb, disperse, fakeShadow;
		BSDF_t tmFlags;
		PFLOAT dispersion_power;
		PFLOAT CauchyA, CauchyB;
};

glassMat_t::glassMat_t(float IOR, color_t filtC, const color_t &srcol, double disp_pow, bool fakeS):
		bumpS(0), mirColS(0), filterCol(filtC), specRefCol(srcol), absorb(false), disperse(false),
		fakeShadow(fakeS), dispersion_power(disp_pow)
{
	ior=IOR;
	bsdfFlags = BSDF_ALL_SPECULAR;
	if(fakeS) bsdfFlags |= BSDF_FILTER;
	tmFlags = fakeS ? BSDF_FILTER | BSDF_TRANSMIT : BSDF_SPECULAR | BSDF_TRANSMIT;
	if(disp_pow > 0.0)
	{
	//	std::cout << "dispersive glass material!\n";
		disperse = true;
		CauchyCoefficients(IOR, disp_pow, CauchyA, CauchyB);
		bsdfFlags |= BSDF_DISPERSIVE;
	}
}

void glassMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	nodeStack_t stack(state.userdata);
	if(bumpS) evalBump(stack, state, sp, bumpS);
	
	//eval viewindependent nodes
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
}

#define matches(bits, flags) ((bits & (flags)) == (flags))

color_t glassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	static bool debug=true;
	nodeStack_t stack(state.userdata);
	if( !(s.flags & BSDF_SPECULAR) && !((s.flags & bsdfFlags & BSDF_DISPERSIVE) && state.chromatic) )
	{
		s.pdf = 0.f;
		return color_t(0.f);
	}
	vector3d_t refdir, N;
	bool outside = sp.Ng*wo > 0;
	PFLOAT cos_wo_N = sp.N*wo;
	if(outside)	N = (cos_wo_N >= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	else		N = (cos_wo_N <= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	s.pdf = 1.f;
	
	// we need to sample dispersion
	if(disperse && state.chromatic)
	{
		PFLOAT cur_ior = getIOR(state.wavelength, CauchyA, CauchyB);
		if( refract(N, wo, refdir, cur_ior) )
		{
			CFLOAT Kr, Kt;
			fresnel(wo, N, cur_ior, Kr, Kt);
			float pKr = 0.01+0.99*Kr, pKt = 0.01+0.99*Kt;
			if( !(s.flags & BSDF_SPECULAR) || s.s1 < pKt)
			{
				wi = refdir;
				if(debug){ std::cout << "flags:"<<s.flags<<", matches:"<<(bool)(matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT))<<std::endl; debug=false; }
				s.pdf = (matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT)) ? pKt : 1.f;
				s.sampledFlags = BSDF_DISPERSIVE | BSDF_TRANSMIT;
				//color_t wl_col;
				//wl2rgb(state.wavelength, wl_col);
//				if(debug){ std::cout << "wl_col:"<<wl_col<<", wavelen.:"<<state.wavelength<<std::endl; debug=false; }
				return filterCol*/* wl_col* */(Kt/std::fabs(sp.N*wi));
			}
			else if( matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT) )
			{
				wi = reflect_plane(N, wo);
				s.pdf = pKr;
				s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
				return (mirColS ? mirColS->getColor(stack) : specRefCol) * (Kr/std::fabs(sp.N*wi));
			}
		}
		else if( matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT) ) //total inner reflection
		{
			wi = reflect_plane(N, wo);
			s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
			return color_t(1.f/std::fabs(sp.N*wi));
		}
	}
	else // no dispersion calculation necessary, regardless of material settings
	{
		PFLOAT cur_ior = disperse ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;
		if( refract(N, wo, refdir, cur_ior) )
		{
			CFLOAT Kr, Kt;
			fresnel(wo, N, ior, Kr, Kt);
			float pKr = 0.01+0.99*Kr, pKt = 0.01+0.99*Kt;
			if(s.s1 < pKt && matches(s.flags, tmFlags))
			{
				wi = refdir;
				s.pdf = pKt;
				s.sampledFlags = tmFlags;
				if(s.reverse)
				{
					s.pdf_back = s.pdf; //wrong...need to calc fresnel explicitly!
					s.col_back = filterCol*(Kt/std::fabs(sp.N*wo));
				}
				return filterCol*(Kt/std::fabs(sp.N*wi));
			}
			else if( matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT) )
			{
				wi = reflect_plane(N, wo);
				s.pdf = pKr;
				s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
				if(s.reverse)
				{
					s.pdf_back = s.pdf; //wrong...need to calc fresnel explicitly!
					s.col_back = (mirColS ? mirColS->getColor(stack) : specRefCol) * (Kr/std::fabs(sp.N*wo));
				}
				return (mirColS ? mirColS->getColor(stack) : specRefCol) * (Kr/std::fabs(sp.N*wi));
			}
		}
		else if( matches(s.flags, BSDF_SPECULAR|BSDF_REFLECT) )//total inner reflection
		{
			wi = reflect_plane(N, wo);
			s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
			color_t tir_col(1.f/std::fabs(sp.N*wi));
			if(s.reverse)
			{
				s.pdf_back = s.pdf;
				s.col_back = tir_col;
			}
			return tir_col;
		}
	}
	s.pdf = 0.f;
	return color_t(0.f);
}

color_t glassMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	CFLOAT Kr, Kt;
	fresnel(wo, N, ior, Kr, Kt);
	return Kt*filterCol;
}

void glassMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const
{
	nodeStack_t stack(state.userdata);
	bool outside = sp.Ng*wo > 0;
	vector3d_t N;
	PFLOAT cos_wo_N = sp.N*wo;
	if(outside)
	{
		N = (cos_wo_N >= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	}
	else
	{
		N = (cos_wo_N <= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	}
//	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	vector3d_t refdir;
	
	PFLOAT cur_ior = disperse ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;
	if( refract(N, wo, refdir, cur_ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wo, N, cur_ior, Kr, Kt);
		if(!state.chromatic || !disperse)
		{
			col[1] = Kt*filterCol;
			dir[1] = refdir;
			refr = true;
		}
		else refr = false; // in this case, we need to sample dispersion, i.e. not considered specular
		// accounting for fresnel reflection when leaving refractive material is a real performance
		// killer as rays keep bouncing inside objects and contribute little after few bounces, so limit we it:
		if(outside || state.raylevel < 2)
		{
			dir[0] = reflect_plane(N, wo);
			col[0] = (mirColS ? mirColS->getColor(stack) : specRefCol) * Kr;
			refl = true;
		}
		else refl = false;
	}
	else //total inner reflection
	{
		col[0] = color_t(1.f);
		dir[0] = reflect_plane(N, wo);
		refl = true;
		refr = false;
	}
}

bool glassMat_t::volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp, const ray_t &ray, color_t &col)const
{
	// absorption due to beer's law (simple RGB based)
	if(absorb)
	{
		// outgoing dir is -ray.dir, we check if it coming from backside (i.e. wo*Ng < 0)
		// so ray.dir*Ng needs to be larger than 0...
		if( (ray.dir * sp.Ng) < 0.f )
		{
			if(ray.tmax < 0.f || ray.tmax > 1e30f) //infinity check...
			{
				col = color_t( 0.f, 0.f, 0.f);
				return true;
			}
			CFLOAT dist = ray.tmax; // maybe substract ray.tmin...
			color_t be(-dist*beer_sigma_a);
			col = color_t( exp(be.getR()), exp(be.getG()), exp(be.getB()) );
			return true;
		}
	}
	return false;
}

/* bool glassMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const
{
	if(!(s.flags & BSDF_SPECULAR)) return false;
	nodeStack_t stack(state.userdata);
	color_t scol = color_t(1.f,1.f,1.f);
	vector3d_t refdir;
	
	if( refract(sp.N, wi, refdir, ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wi, sp.N, ior, Kr, Kt);
		if(s.s1 < Kt)
		{
			wo = refdir;
			s.sampledFlags = BSDF_SPECULAR | BSDF_TRANSMIT;
			scol = filterCol;
		}
		else
		{
			wo = reflect_plane(sp.N, wo);
			s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
			scol = (mirColS ? mirColS->getColor(stack) : specRefCol);
		}
	}
	else //total inner reflection
	{
		wo = reflect_plane(sp.N, wo);
		s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
	}
	color_t cnew = s.lcol * s.alpha * scol;
	CFLOAT new_max = std::max( std::max(cnew.getR(), cnew.getG()), cnew.getB() );
	CFLOAT old_max = std::max( std::max(s.lcol.getR(), s.lcol.getG()), s.lcol.getB() );
	float prob = std::min(1.f, new_max/old_max);
	if(s.s3 <= prob)
	{
		s.color = cnew*(1.f/prob);
		return true;
	}
	return false;
} */

material_t* glassMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	double IOR=1.4;
	double filt=0.f;
	double disp_power=0.0;
	color_t filtCol(1.f), absorp(1.f), srCol(1.f);
	const std::string *name=0;
	bool fake_shad = false;
	params.getParam("IOR", IOR);
	params.getParam("filter_color", filtCol);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", srCol);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);
	glassMat_t *mat = new glassMat_t(IOR, filt*filtCol + color_t(1.f-filt), srCol, disp_power, fake_shad);
	if( params.getParam("absorption", absorp) )
	{
		double dist=1.f;
		if(absorp.R < 1.f || absorp.G < 1.f || absorp.B < 1.f)
		{
			//deprecated method:
			color_t sigma(0.f);
			if(params.getParam("absorption_dist", dist))
			{
				const CFLOAT maxlog = log(1e38);
				sigma.R = (absorp.R > 1e-38) ? -log(absorp.R) : maxlog;
				sigma.G = (absorp.G > 1e-38) ? -log(absorp.G) : maxlog;
				sigma.B = (absorp.B > 1e-38) ? -log(absorp.B) : maxlog;
				if (dist!=0.f) sigma *= 1.f/dist;
			}
			mat->absorb = true;
			mat->beer_sigma_a = sigma;
			mat->bsdfFlags |= BSDF_VOLUMETRIC;
			// creat volume handler (backwards compatibility)
			if(params.getParam("name", name))
			{
				paraMap_t map;
				map["type"] = std::string("beer");
				map["absorption_col"] = absorp;
				map["absorption_dist"] = parameter_t(dist);
				mat->volI = render.createVolumeH(*name, map);
				mat->bsdfFlags |= BSDF_VOLUMETRIC;
			}
		}
	}
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(paramList, render))
	{
		if(params.getParam("mirror_color_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->mirColS = i->second; roots.push_back(mat->mirColS); }
			else std::cout << "[WARNING]: mirror col. shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("bump_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->bumpS = i->second; roots.push_back(mat->bumpS); }
			else std::cout << "[WARNING]: bump shader node '"<<*name<<"' does not exist!\n";
			std::cout << "bump shader: " << name << "(" << (void*)mat->bumpS << ")\n";
		}
	}
	else std::cout << "loadNodes() failed!\n";
	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::vector<shaderNode_t *> colorNodes;
		if(mat->mirColS) mat->getNodeList(mat->mirColS, colorNodes);
		mat->filterNodes(colorNodes, mat->allViewdep, VIEW_DEP);
		mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);
		if(mat->bumpS)
		{
			mat->getNodeList(mat->bumpS, mat->bumpNodes);
		}
	}
	mat->reqMem = mat->reqNodeMem;
	return mat;
}

/*====================================
a simple mirror mat
==================================*/

class mirrorMat_t: public material_t
{
	public:
	mirrorMat_t(color_t rCol, float refVal): ref(refVal)
	{
		if(ref>1.0) ref = 1.0;
		refCol = rCol * refVal;
		bsdfFlags = BSDF_SPECULAR;
	}
	virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, unsigned int &bsdfTypes)const { bsdfTypes=bsdfFlags; }
	virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const {return color_t(0.0);}
	virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
	virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const;
	virtual color_t volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const;
	virtual bool scatterPhoton(const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, float s1, float s2,
								BSDF_t bsdfs, BSDF_t &sampledBSDF, color_t &col) const;
	static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
	protected:
	color_t refCol;
	float ref;
};

color_t mirrorMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	wi = reflect_dir(sp.N, wo);
	s.sampledFlags = BSDF_SPECULAR | BSDF_REFLECT;
	return refCol * (1.f/std::fabs(sp.N*wi));
}

void mirrorMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const
{
	col[0] = refCol;
	col[1] = color_t(1.0);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	dir[0] = reflect_dir(N, wo);
	refl = true;
	refr = false;
}

color_t mirrorMat_t::volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const
{
	return color_t(1.,1.,1.);
}

bool mirrorMat_t::scatterPhoton(const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, float s1, float s2,
								BSDF_t bsdfs, BSDF_t &sampledBSDF, color_t &col) const 
{
	if(!(bsdfs & BSDF_SPECULAR)) return false;
	if(s1>ref) return false;
	s1 /= ref;
	col = refCol*(1.0f/ref);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	wi = reflect_dir(N, wo);

	sampledBSDF = BSDF_SPECULAR;
	return true;
}

material_t* mirrorMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	color_t col(1.0);
	float refl=1.0;
	params.getParam("color",col);
	params.getParam("reflect",refl);
	return new mirrorMat_t(col, refl);
}


/*=============================================================
a "dummy" material, useful e.g. to keep photons from getting
stored on surfaces that don't affect the scene
=============================================================*/

class nullMat_t: public material_t
{
	public:
	nullMat_t() { }
	virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, unsigned int &bsdfTypes)const { bsdfTypes=BSDF_NONE; }
	virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const {return color_t(0.0);}
	virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
//	virtual void getSpecular(const renderState_t &state, const vector3d_t &wo, const surfacePoint_t &sp,
//							 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const;
	virtual color_t volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const;
	static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
};

color_t nullMat_t::volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const
{
	return color_t(1.,1.,1.);
}

color_t nullMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	s.pdf = 0.f;
	return color_t(0.f);
}

material_t* nullMat_t::factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &)
{
	return new nullMat_t();
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("glass", glassMat_t::factory);
		render.registerFactory("mirror", mirrorMat_t::factory);
		render.registerFactory("null", nullMat_t::factory);
		render.registerFactory("rough_glass", roughGlassMat_t::factory);
	}
}

__END_YAFRAY
