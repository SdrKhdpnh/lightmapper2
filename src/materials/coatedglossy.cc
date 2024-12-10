/****************************************************************************
 * 			coatedglossy.cc: a glossy material with specular coating
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

/*! Coated Glossy Material.
	A Material with Phong/Anisotropic Phong microfacet base layer and a layer of
	(dielectric) perfectly specular coating. This is to simulate surfaces like
	metallic paint */

#define C_SPECULAR 	0
#define C_GLOSSY  	1
#define C_DIFFUSE 	2

class coatedGlossyMat_t: public nodeMaterial_t
{
	public:
		coatedGlossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, PFLOAT ior, float expo, bool as_diff);
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
//		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, float s1, float s2)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
								 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const;
		virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const;
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
		
		struct MDat_t
		{
			float mDiffuse, mGlossy, pDiffuse;
			void *stack;
		};
	protected:
		shaderNode_t* diffuseS;
		shaderNode_t* glossyS;
		shaderNode_t* glossyRefS;
		shaderNode_t* bumpS;
		color_t gloss_color, diff_color; //!< color of glossy base
		PFLOAT IOR;
		float exponent, exp_u, exp_v;
		float reflectivity;
		//float pDiffuse;
		CFLOAT mGlossy, mDiffuse;
		bool as_diffuse, with_diffuse, anisotropic;
		BSDF_t specFlags, glossyFlags;
		BSDF_t cFlags[3];
		int nBSDF;
};

coatedGlossyMat_t::coatedGlossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, PFLOAT ior, float expo, bool as_diff):
	diffuseS(0), glossyS(0), glossyRefS(0), bumpS(0), gloss_color(col), diff_color(dcol), IOR(ior), exponent(expo), reflectivity(reflect), mDiffuse(diff),
	as_diffuse(as_diff), with_diffuse(false), anisotropic(false)
{
	cFlags[C_SPECULAR] = (BSDF_SPECULAR | BSDF_REFLECT);
	cFlags[C_GLOSSY] = as_diffuse? (BSDF_DIFFUSE | BSDF_REFLECT) : (BSDF_GLOSSY | BSDF_REFLECT);
	if(diff>0)
	{
		cFlags[C_DIFFUSE] = BSDF_DIFFUSE | BSDF_REFLECT;
		with_diffuse = true;
		nBSDF = 3;
	}
	else
	{
		cFlags[C_DIFFUSE] = BSDF_NONE;
		nBSDF = 2;
	}
	bsdfFlags = cFlags[C_SPECULAR] | cFlags[C_GLOSSY] | cFlags[C_DIFFUSE];
	
	//pDiffuse = std::min(0.6f , 1.f - (reflectivity/(reflectivity + (1.f-reflectivity)*diff)) );
}

void coatedGlossyMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	dat->stack = (char*)state.userdata + sizeof(MDat_t);
	nodeStack_t stack(dat->stack);
	if(bumpS) evalBump(stack, state, sp, bumpS);
	
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
	dat->mDiffuse = mDiffuse;
	dat->mGlossy = glossyRefS ? glossyRefS->getScalar(stack) : reflectivity;
	dat->pDiffuse = std::min(0.6f , 1.f - (dat->mGlossy/(dat->mGlossy + (1.f-dat->mGlossy)*dat->mDiffuse)) );
}

color_t coatedGlossyMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	using std::abs;
	MDat_t *dat = (MDat_t *)state.userdata;
	color_t col(0.f);
	if( !(bsdfs & BSDF_REFLECT) ) return col;
	nodeStack_t stack(dat->stack);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	bool diffuse_flag = bsdfs & BSDF_DIFFUSE;
	CFLOAT Kr, Kt;
	fresnel(wo, N, IOR, Kr, Kt);
	if( (as_diffuse && diffuse_flag) || (!as_diffuse && (bsdfs & BSDF_GLOSSY)) )
	{
		vector3d_t H = (wo + wi).normalize(); // half-angle
		PFLOAT cos_wi_H = wi*H;
		PFLOAT glossy;
		if(anisotropic)
		{
			vector3d_t Hs(H*sp.NU, H*sp.NV, H*N);
			glossy = Kt * AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wi_H, dat->mGlossy) / 
							( 8.f * abs(cos_wi_H) * std::max(abs(wo*N), abs(wi*N))  );
		}
		else
		{
			glossy = Kt * Blinn_D(H*N, exponent) * SchlickFresnel(cos_wi_H, dat->mGlossy) / 
							( 8.f * abs(cos_wi_H) * std::max(abs(wo*N), abs(wi*N))  ); // our reflectances are multiplied by Pi too...
		}
		col = (CFLOAT)glossy*(glossyS ? glossyS->getColor(stack) : gloss_color);
	}
	if(with_diffuse && diffuse_flag)
	{
		PFLOAT f_wi = 1.f - 0.5f*std::fabs(wi*N);
		PFLOAT f_wo = 1.f - 0.5f*std::fabs(wo*N);
		PFLOAT f_wi2 = f_wi*f_wi, f_wo2 = f_wo*f_wo;
		PFLOAT diffuse = Kt * (28.f/23.f)*(1.f - dat->mGlossy)*(1.f - f_wi2*f_wi2*f_wi)*(1.f - f_wo2*f_wo2*f_wo);
		color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
		col += (CFLOAT)diffuse * dat->mDiffuse * diff_base;
	}
	return col;
}

color_t coatedGlossyMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
//static bool dbg=true;
	MDat_t *dat = (MDat_t *)state.userdata;
	PFLOAT cos_Ng_wo = sp.Ng*wo, cos_Ng_wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	vector3d_t Hs;
	s.pdf = 0.f;
	CFLOAT Kr, Kt;
	fresnel(wo, N, IOR, Kr, Kt);
	
	// missing! get components
	nodeStack_t stack(dat->stack);
	bool use[3] = {false, false, false};
	float sum=0.f, accumC[3], val[3], width[3];
	int cIndex[3]; // entry values: 0 := specular part, 1 := glossy part, 2:= diffuse part;
	int rcIndex[3]; // reverse fmapping of cIndex, gives position of spec/glossy/diff in val/width array
	accumC[0] = Kr;
	accumC[1] = Kt*(1.f - dat->pDiffuse);
	accumC[2] = Kt*(dat->pDiffuse);
	
	int nMatch=0, pick=-1;
	for(int i=0; i<nBSDF; ++i)
	{
		if((s.flags & cFlags[i]) == cFlags[i])
		{
			use[i] = true;
			width[nMatch] = accumC[i];
			cIndex[nMatch] = i;
			rcIndex[i] = nMatch;
			sum += width[nMatch];
			val[nMatch] = sum;
			++nMatch;
		}
	}
	if(!nMatch || sum < 0.00001){ return color_t(0.f); }
	else if(nMatch==1){ pick=0; width[0]=1.f; }
	else
	{
		float inv_sum = 1.f/sum;
		for(int i=0; i<nMatch; ++i)
		{
			val[i] *= inv_sum;
			width[i] *= inv_sum;
			if((s.s1 <= val[i]) && (pick<0 ))	pick = i;
		}
	}
	if(pick<0) pick=nMatch-1;
	float s1;
	if(pick>0) s1 = (s.s1 - val[pick-1]) / width[pick];
	else 	   s1 = s.s1 / width[pick];
	
	color_t scolor(0.f);
	switch(cIndex[pick])
	{
		case C_SPECULAR: // specular reflect
			wi = reflect_dir(N, wo);
			scolor = color_t(Kr)/std::fabs(sp.N*wi);
			s.pdf = width[pick];
			if(s.reverse)
			{
				s.pdf_back = s.pdf; // mirror is symmetrical
				s.col_back = color_t(Kr)/std::fabs(sp.N*wo);
			}
			break;
		case C_GLOSSY: // glossy
			if(anisotropic) AS_Aniso_Sample(Hs, s1, s.s2, exp_u, exp_v);
			else 			Blinn_Sample(Hs, s1, s.s2, exponent);
			break;
		case C_DIFFUSE: // lambertian
		default:
			wi = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s.s2);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
	}
	
	if(cIndex[pick] != C_SPECULAR)
	{
		// evaluate BSDFs and PDFs...
		if(use[C_GLOSSY])
		{
			PFLOAT glossy;
			PFLOAT cos_wo_H;
			if(cIndex[pick] != C_GLOSSY)
			{
				vector3d_t H = (wi+wo).normalize();
				Hs = vector3d_t(H*sp.NU, H*sp.NV, H*N);
				cos_wo_H = wo*H;
			}
			else
			{
				vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
				cos_wo_H = wo*H;
				if ( cos_wo_H < 0.f){ H = reflect_plane(N, H)/* -H */; cos_wo_H = wo*H/* -cos_wo_H */; }
				// Compute incident direction by reflecting wo about H
				wi = reflect_dir(H, wo);
				cos_Ng_wi = sp.Ng*wi;
				if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			}
			
			if(anisotropic)
			{
				s.pdf += AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v) * width[rcIndex[C_GLOSSY]];
				// same as eval(...) only that we don't need to redo some steps
				glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wo_H, dat->mGlossy) / 
									( 8.f * std::abs(cos_wo_H) * std::max(std::abs(wo*N), std::abs(wi*N))  );
			}
			else
			{
				s.pdf += Blinn_Pdf(Hs.z, cos_wo_H, exponent) * width[rcIndex[C_GLOSSY]];
				// same as eval(...) only that we don't need to redo some steps
				glossy = Blinn_D(Hs.z, exponent) * SchlickFresnel(cos_wo_H, dat->mGlossy) / 
									( 8.f * std::abs(cos_wo_H) * std::max(std::abs(wo*N), std::abs(wi*N))  );
			}
			scolor = (CFLOAT)glossy*Kt*(glossyS ? glossyS->getColor(stack) : gloss_color);
		}
		if(use[C_DIFFUSE])
		{
			PFLOAT f_wi = 1.f - 0.5f*std::fabs(wi*N);
			PFLOAT f_wo = 1.f - 0.5f*std::fabs(wo*N);
			PFLOAT f_wi2 = f_wi*f_wi, f_wo2 = f_wo*f_wo;
			CFLOAT diff = (28.f/23.f)*(1.f - dat->mGlossy)*(1.f - f_wi2*f_wi2*f_wi)*(1.f - f_wo2*f_wo2*f_wo);
			color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
			scolor += (CFLOAT)diff*Kt*dat->mDiffuse*diff_base;
			s.pdf += std::fabs(wi*N) * width[rcIndex[C_DIFFUSE]];
		}
	}
	s.sampledFlags = cFlags[cIndex[pick]];
	
	return scolor;
}

float coatedGlossyMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t flags)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	bool transmit = (sp.Ng*wo)*(sp.Ng*wi) < 0.f;
	if(transmit) return 0.f;
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	float pdf = 0.f;
	CFLOAT Kr, Kt;
	fresnel(wo, N, IOR, Kr, Kt);
	
	float accumC[3], sum=0.f, width;
	accumC[0] = Kr;
	accumC[1] = Kt*(1.f - dat->pDiffuse);
	accumC[2] = Kt*(dat->pDiffuse);
	
	int nMatch=0;
	for(int i=0; i<nBSDF; ++i)
	{
		if((flags & cFlags[i]) == cFlags[i])
		{
			width = accumC[i];
			sum += width;
			if(i == C_GLOSSY)
			{
				vector3d_t H = (wi+wo).normalize();
				PFLOAT cos_wo_H = wo*H;
				PFLOAT cos_N_H = N*H;
				if(anisotropic)
				{
					vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
					pdf += AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v) * width;
				}
				else pdf += Blinn_Pdf(cos_N_H, cos_wo_H, exponent) * width;
			}
			else if(i == C_DIFFUSE)
			{
				pdf += std::fabs(wi*N) * width;
			}
			++nMatch;
		}
	}
	if(!nMatch || sum < 0.00001) return 0.f;
	return pdf / sum;
}

void coatedGlossyMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							 bool &refl, bool &refr, vector3d_t *const dir, color_t *const col)const
{
	bool outside = sp.Ng*wo >= 0;
	vector3d_t N, Ng;
	PFLOAT cos_wo_N = sp.N*wo;
	if(outside)
	{
		N = (cos_wo_N >= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
		Ng = sp.Ng;
	}
	else
	{
		N = (cos_wo_N <= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
		Ng = -sp.Ng;
	}
//	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
//	vector3d_t refdir;
	
	CFLOAT Kr, Kt;
	fresnel(wo, N, IOR, Kr, Kt);
	
	refr = false;
	dir[0] = reflect_plane(N, wo);
	col[0] = color_t(Kr);
	PFLOAT cos_wi_Ng = dir[0]*Ng;
	if(cos_wi_Ng < 0.01){ dir[0] += (0.01-cos_wi_Ng)*Ng; dir[0].normalize(); }
	refl = true;
}

bool coatedGlossyMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi,
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

material_t* coatedGlossyMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	color_t col(1.f), dcol(1.f);
	float refl=1.f;
	float diff=0.f;
	float exponent=50.f; //wild guess, do sth better
	double ior=1.4;
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
	params.getParam("IOR", ior);

	coatedGlossyMat_t *mat = new coatedGlossyMat_t(col, dcol , refl, diff, ior, exponent, as_diff);
	if(aniso)
	{
		double e_u=50.0, e_v=50.0;
		params.getParam("exp_u", e_u);
		params.getParam("exp_v", e_v);
		mat->anisotropic = true;
		mat->exp_u = e_u;
		mat->exp_v = e_v;
	}
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
		render.registerFactory("coated_glossy", coatedGlossyMat_t::factory);
	}
}

__END_YAFRAY
