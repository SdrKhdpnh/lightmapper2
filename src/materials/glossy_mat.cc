/****************************************************************************
 * 			glossymat.cc: a glossy material based on Ashikhmin&Shirley's Paper
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
#include <core_api/environment.h>
#include <utilities/sample_utils.h>
#include <materials/microfacet.h>


__BEGIN_YAFRAY

class glossyMat_t: public nodeMaterial_t
{
	public:
		glossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, float expo, bool as_diffuse);
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
//		virtual void getSpecular(const renderState_t &state, const vector3d_t &wo, const surfacePoint_t &sp,
//								 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const;
		virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const;
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
		
		struct MDat_t
		{
			float mDiffuse, mGlossy, pDiffuse;
			void *stack;
		};
	protected:
		// color_t getDiffuse(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl)const;
		void evalVdNodes(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, nodeStack_t &stack)const;
		
		shaderNode_t* diffuseS;
		shaderNode_t* glossyS;
		shaderNode_t* glossyRefS;
		shaderNode_t* bumpS;
		color_t gloss_color, diff_color;
		float exponent, exp_u, exp_v;
		float reflectivity;
		//float pDiffuse;
		CFLOAT mDiffuse;
		bool as_diffuse, with_diffuse, anisotropic;
		int aniso_mode;
};

glossyMat_t::glossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, float expo, bool as_diff):
			diffuseS(0), glossyS(0), glossyRefS(0), bumpS(0), gloss_color(col), diff_color(dcol), exponent(expo),
			reflectivity(reflect), mDiffuse(diff), as_diffuse(as_diff), with_diffuse(false), anisotropic(false), aniso_mode(1)
{
	bsdfFlags = BSDF_NONE;
	if(diff>0)
	{
		bsdfFlags = BSDF_DIFFUSE | BSDF_REFLECT;
		with_diffuse = true;
	}
	bsdfFlags |= as_diffuse? (BSDF_DIFFUSE | BSDF_REFLECT) : (BSDF_GLOSSY | BSDF_REFLECT);
	//pDiffuse = std::min(0.6f , 1.f - (reflectivity/(reflectivity + (1.f-reflectivity)*diff)) );
}

void glossyMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	dat->stack = (char*)state.userdata + sizeof(MDat_t);
	nodeStack_t stack(dat->stack);
	if(bumpS) evalBump(stack, state, sp, bumpS);
	
	//eval viewindependent nodes
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
	dat->mDiffuse = mDiffuse;
	dat->mGlossy = glossyRefS ? glossyRefS->getScalar(stack) : reflectivity;
	dat->pDiffuse = std::min(0.6f , 1.f - (dat->mGlossy/(dat->mGlossy + (1.f-dat->mGlossy)*dat->mDiffuse)) );
}

/* inline color_t glossyMat_t::getDiffuse(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl)const
{
	if(diffuseS)
	{
		nodeStack_t stack(state.userdata);
		std::vector<shaderNode_t *>::const_iterator iter, end=allViewdep.end();
		for(iter = allViewdep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp, wo, wl);
		
		return diffuseS->getColor(stack);
	}
	return diff_color;
} */


//! evaluate view-dependant nodes:

inline void glossyMat_t::evalVdNodes(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, nodeStack_t &stack)const
{
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewdep.end();
	for(iter = allViewdep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp, wo, wl);
}

//===========================================================================
// some inline functions, possibly worth an extra utility-header or even lib
// ==========================================================================

// i still believe it's (exponent + 1.f)...otherwise it doesn't integrate to 1.0 but >1.0
/* inline PFLOAT Blinn_D(PFLOAT cos_h, PFLOAT exponent)
{
	return (cos_h>0) ? (exponent+2.f) * std::pow(cos_h, exponent) : 0.f;
}

inline float Blinn_Pdf(PFLOAT costheta, PFLOAT cos_w_H, PFLOAT exponent)
{
	return ((exponent + 2.f) * std::pow(costheta, exponent)) / (2.f * 4.f * cos_w_H); //our PDFs are multiplied by Pi...
}

inline PFLOAT SchlickFresnel(PFLOAT costheta, PFLOAT R)
{
	PFLOAT cm1 = 1.f - costheta;
	PFLOAT cm1_2 = cm1*cm1;
	return R + (1.f - R) * cm1*cm1_2*cm1_2;
}

inline void Blinn_Sample(vector3d_t &H, float s1, float s2, PFLOAT exponent)
{
	// Compute sampled half-angle vector H for Blinn distribution
	PFLOAT costheta = pow(s1, 1.f / (exponent+1.f));
	PFLOAT sintheta = sqrt(std::max(0.f, 1.f - costheta*costheta));
	PFLOAT phi = s2 * 2.f * M_PI;
	H = vector3d_t(sintheta*sin(phi), sintheta*cos(phi), costheta); //returning directly the spherical coords would allow some optimization..
	//pdf = ((exponent + 2.f) * pow(costheta, exponent)) / (2.f *  4.f * (wo*H)); //our PDFs are multiplied by Pi...
} */

// ==========================================================================

color_t glossyMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	using std::abs;
	MDat_t *dat = (MDat_t *)state.userdata;
	color_t col(0.f);
	if( !(bsdfs & BSDF_REFLECT) ) return col;
	//vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	PFLOAT cos_Ng_wo = sp.Ng*wo;
	bool inside = (cos_Ng_wo<0);
	vector3d_t N = (inside) ? -sp.N : sp.N;
	bool diffuse_flag = bsdfs & BSDF_DIFFUSE;
	nodeStack_t stack(dat->stack);
	
	if( (as_diffuse && diffuse_flag) || (!as_diffuse && (bsdfs & BSDF_GLOSSY)) )
	{
		vector3d_t H = (wo + wi).normalize(); // half-angle
		PFLOAT cos_wi_H = wi*H;
		PFLOAT glossy;
		if(anisotropic)
		{
			//vector3d_t Hs(H*sp.NU, H*sp.NV, H*N);
			vector3d_t Hs = aniso_invtransf(sp, H, aniso_mode);
			if(inside) Hs = -Hs;
			glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wi_H, dat->mGlossy) / 
							( 8.f * abs(cos_wi_H) * std::max(abs(wo*N), abs(wi*N))  );
		}
		else
		{
			glossy = Blinn_D(H*N, exponent) * SchlickFresnel(cos_wi_H, dat->mGlossy) / 
							( 8.f * abs(cos_wi_H) * std::max(abs(wo*N), abs(wi*N))  ); // our reflectances are multiplied by Pi too...
		}
		col = (CFLOAT)glossy*(glossyS ? glossyS->getColor(stack) : gloss_color);
	}
	if(with_diffuse && diffuse_flag)
	{
		PFLOAT f_wi = 1.f - 0.5f*std::fabs(wi*N);
		PFLOAT f_wo = 1.f - 0.5f*std::fabs(wo*N);
		PFLOAT f_wi2 = f_wi*f_wi, f_wo2 = f_wo*f_wo;
		PFLOAT diffuse = (28.f/23.f)*(1.f - dat->mGlossy)*(1.f - f_wi2*f_wi2*f_wi)*(1.f - f_wo2*f_wo2*f_wo);
		color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
		col += (CFLOAT)diffuse*dat->mDiffuse*diff_base;
	}
	return col;
}

color_t glossyMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
//static bool dbg=true;
	MDat_t *dat = (MDat_t *)state.userdata;
	PFLOAT cos_Ng_wo = sp.Ng*wo, cos_Ng_wi;
	bool inside = (cos_Ng_wo<0);
	vector3d_t N = (inside) ? -sp.N : sp.N;
	s.pdf = 0.f;
	float s1 = s.s1;
	//float cos_wo_N = wo*sp.N;
	float cur_pDiffuse = dat->pDiffuse;
	bool use_glossy = as_diffuse ? (s.flags & BSDF_DIFFUSE) : (s.flags & BSDF_GLOSSY);
	bool use_diffuse = with_diffuse && (s.flags & BSDF_DIFFUSE);
	nodeStack_t stack(dat->stack);
	
	if(use_diffuse)
	{
		//float ffresnel = (1.f - cos_wo_N)*(1.f - cos_wo_N);
		//cur_pDiffuse = pDiffuse * (0.1f + 0.9f*(1.f-ffresnel*ffresnel));
		float s_pDiffuse = use_glossy ? cur_pDiffuse : 1.f;
		if(s1 < s_pDiffuse)
		{
			s1 /= s_pDiffuse;
			wi = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s.s2);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			s.pdf = std::fabs(wi*N);
			if(use_glossy)
			{
				vector3d_t H = (wi+wo).normalize();
				PFLOAT cos_wo_H = wo*H;
				PFLOAT cos_N_H = N*H;
				if(anisotropic)
				{
					//vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
					vector3d_t Hs = aniso_invtransf(sp, H, aniso_mode);
					if(inside) Hs = -Hs;
					s.pdf = s.pdf*cur_pDiffuse + AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v)*(1.f-cur_pDiffuse);
				}
				else s.pdf = s.pdf*cur_pDiffuse + Blinn_Pdf(cos_N_H, cos_wo_H, exponent)*(1.f-cur_pDiffuse);
			}
			s.sampledFlags = BSDF_DIFFUSE | BSDF_REFLECT;
			return eval(state, sp, wo, wi, s.flags);
		}
		s1 -= cur_pDiffuse;
		s1 /= (1.f - cur_pDiffuse);
	}
	
	if(use_glossy)
	{
		vector3d_t Hs; // H in "shading space"
		color_t col;
		PFLOAT glossy;
		if(anisotropic)
		{
			AS_Aniso_Sample(Hs, s1, s.s2, exp_u, exp_v);
			vector3d_t H = aniso_transform(sp, Hs, aniso_mode);//Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
			if(inside) H = -H;
		
			PFLOAT cos_wo_H = wo*H;
			if ( cos_wo_H < 0.f){ H = reflect_plane(N, H)/* -H */; cos_wo_H = wo*H/* -cos_wo_H */; }
			cos_Ng_wi = sp.Ng*wi;
			// Compute incident direction by reflecting wo about H
			wi = reflect_dir(H, wo);
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			s.pdf = AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v);
			
			// same as eval(...) only that we don't need to redo some steps
			glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wo_H, dat->mGlossy) / 
									( 8.f * std::abs(cos_wo_H) * std::max(std::abs(wo*N), std::abs(wi*N))  );
		}
		else
		{
 			Blinn_Sample(Hs, s1, s.s2, exponent);
			vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
			
			PFLOAT cos_wo_H = wo*H;
			if ( cos_wo_H < 0.f){ H = reflect_plane(N, H)/* -H */; cos_wo_H = wo*H/* -cos_wo_H */; }
			// Compute incident direction by reflecting wo about H
			wi = reflect_dir(H, wo);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			s.pdf = Blinn_Pdf(Hs.z, cos_wo_H, exponent);
			
			
			// same as eval(...) only that we don't need to redo some steps
			PFLOAT cos_wi_H = wi*H; // uhm...just use cos_wo_H !?
			glossy = Blinn_D(H*N, exponent) * SchlickFresnel(cos_wi_H, dat->mGlossy) / 
									( 8.f * std::abs(cos_wi_H) * std::max(std::abs(wo*N), std::abs(wi*N))  );
		}
		col = (CFLOAT)glossy*(glossyS ? glossyS->getColor(stack) : gloss_color);
		s.sampledFlags = as_diffuse ? BSDF_DIFFUSE | BSDF_REFLECT : BSDF_GLOSSY | BSDF_REFLECT;
	
		if(use_diffuse)
		{
			s.pdf = std::fabs(wi*N)*cur_pDiffuse + s.pdf*(1.f-cur_pDiffuse);
			PFLOAT f_wi = 1.f - 0.5f*std::fabs(wi*N);
			PFLOAT f_wo = 1.f - 0.5f*std::fabs(wo*N);
			PFLOAT f_wi2 = f_wi*f_wi, f_wo2 = f_wo*f_wo;
			PFLOAT diffuse = (28.f/23.f)*(1.f - dat->mGlossy)*(1.f - f_wi2*f_wi2*f_wi)*(1.f - f_wo2*f_wo2*f_wo);
			color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
			col += (CFLOAT)diffuse*dat->mDiffuse*diff_base;
		}
		return col;
	}
	return color_t(0.f);
}

float glossyMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t flags)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	PFLOAT cos_Ng_wo = sp.Ng*wo, cos_Ng_wi = sp.Ng*wi;
	bool inside = cos_Ng_wo < 0.f;
	bool transmit = cos_Ng_wo*cos_Ng_wi < 0.f;
	if(transmit) return 0.f;
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	float pdf = 0.f;
	//float cos_wo_N = wo*N;
	float cur_pDiffuse = dat->pDiffuse;
	bool use_glossy = as_diffuse ? (flags & BSDF_DIFFUSE) : (flags & BSDF_GLOSSY);
	bool use_diffuse = with_diffuse && (flags & BSDF_DIFFUSE);
	if(use_diffuse)
	{
		pdf = std::fabs(wi*N);
		//float ffresnel = (1.f - cos_wo_N)*(1.f - cos_wo_N);
		//cur_pDiffuse = pDiffuse * (0.1f + 0.9f*(1.f-ffresnel*ffresnel));
		if(use_glossy)
		{
			vector3d_t H = (wi+wo).normalize();
			PFLOAT cos_wo_H = wo*H;
			PFLOAT cos_N_H = N*H;
			if(anisotropic)
			{
				//vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
				vector3d_t Hs = aniso_invtransf(sp, H, aniso_mode);
				if(inside) Hs = -Hs;
				pdf = pdf*cur_pDiffuse + AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v)*(1.f-cur_pDiffuse);
			}
			else pdf = pdf*cur_pDiffuse + Blinn_Pdf(cos_N_H, cos_wo_H, exponent)*(1.f-cur_pDiffuse);
		}
		return pdf;
	}
	if(use_glossy)
	{
		vector3d_t H = (wi+wo).normalize();
		PFLOAT cos_wo_H = wo*H;
		PFLOAT cos_N_H = N*H;
		if(anisotropic)
		{
			//vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
			vector3d_t Hs = aniso_invtransf(sp, H, aniso_mode);
			if(inside) H = -H;
			pdf = AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v);
		}
		else pdf = Blinn_Pdf(cos_N_H, cos_wo_H, exponent);
	}
	return pdf;
}

bool glossyMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi,
								vector3d_t &wo, pSample_t &s) const
{
	color_t scol = sample(state, sp, wi, wo, s);
	if(s.pdf > 1.0e-6f)
	{
		color_t cnew = s.lcol * s.alpha * scol * (std::fabs(wo*sp.N)/s.pdf);
		CFLOAT new_max = std::max( std::max(cnew.getR(), cnew.getG()), cnew.getB() );
		CFLOAT old_max = std::max( std::max(s.lcol.getR(), s.lcol.getG()), s.lcol.getB() );
		float prob = std::min(1.f, new_max/old_max);
		if(s.s3 <= prob)
		{
			s.color = cnew*(1.f/prob);
			return true;
		}
	}
	return false;
}

material_t* glossyMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	color_t col(1.f), dcol(1.f);
	float refl=1.f;
	float diff=0.f;
	float exponent=50.f; //wild guess, do sth better
	bool as_diff=true;
	bool aniso=false;
	const std::string *name=0;
	params.getParam("color", col);
	params.getParam("diffuse_color", dcol);
	params.getParam("diffuse_reflect", diff);
	params.getParam("glossy_reflect", refl);
	params.getParam("as_diffuse", as_diff);
	params.getParam("exponent", exponent);
	params.getParam("anisotropic", aniso);
	glossyMat_t *mat = new glossyMat_t(col, dcol , refl, diff, exponent, as_diff);
	if(aniso)
	{
		double e_u=50.0, e_v=50.0;
		params.getParam("exp_u", e_u);
		params.getParam("exp_v", e_v);
		mat->anisotropic = true;
		mat->exp_u = e_u;
		mat->exp_v = e_v;
	}
	//shaderNode_t *diffuseS=0, *glossyS=0, *bumpS=0;
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(paramList, render))
	{
		if(params.getParam("diffuse_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->diffuseS = i->second; roots.push_back(mat->diffuseS); }
			else std::cout << "[WARNING]: diffuse shader node '"<<*name<<"' does not exist!\n";
			std::cout << "diffuse shader: " << name << "(" << (void*)mat->diffuseS << ")\n";
		}
		if(params.getParam("glossy_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->glossyS = i->second; roots.push_back(mat->glossyS); }
			else std::cout << "[WARNING]: glossy shader node '"<<*name<<"' does not exist!\n";
			std::cout << "glossy shader: " << name << "(" << (void*)mat->glossyS << ")\n";
		}
		if(params.getParam("glossy_reflect_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->glossyRefS = i->second; roots.push_back(mat->glossyRefS); }
			else std::cout << "[WARNING]: glossy ref. shader node '"<<*name<<"' does not exist!\n";
			std::cout << "glossy ref. shader: " << name << "(" << (void*)mat->glossyRefS << ")\n";
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
		std::cout << "evaluation order:\n";
		for(unsigned int k=0; k<mat->allSorted.size(); ++k) std::cout << (void*)mat->allSorted[k]<<"\n";
		std::vector<shaderNode_t *> colorNodes;
		if(mat->diffuseS) mat->getNodeList(mat->diffuseS, colorNodes);
		if(mat->glossyS) mat->getNodeList(mat->glossyS, colorNodes);
		if(mat->glossyRefS) mat->getNodeList(mat->glossyRefS, colorNodes);
		mat->filterNodes(colorNodes, mat->allViewdep, VIEW_DEP);
		mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);
		if(mat->bumpS) mat->getNodeList(mat->bumpS, mat->bumpNodes);
	}
	mat->reqMem = mat->reqNodeMem + sizeof(MDat_t);
	return mat;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("glossy", glossyMat_t::factory);
	}
}

__END_YAFRAY
