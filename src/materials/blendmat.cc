/****************************************************************************
 * 			blendmat.cc: a material that blends two material
 *      This is part of the yafray package
 *      Copyright (C) 2008  Mathias Wein
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
 
#include <materials/blendmat.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

blendMat_t::blendMat_t(const material_t *m1, const material_t *m2, CFLOAT bval):
	mat1(m1), mat2(m2), blendS(0), blendVal(bval)
{
	bsdfFlags = mat1->getFlags() | mat2->getFlags();
	mmem1 = mat1->getReqMem();
	mmem2 = mat2->getReqMem();
}

blendMat_t::~blendMat_t()
{}

#define PTR_ADD(ptr,sz) ((char*)ptr+(sz))

void blendMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	//create our "stack" to save node results
	nodeStack_t stack(state.userdata);
	evalNodes(state, sp, allSorted, stack);
	CFLOAT val = (blendS) ? blendS->getScalar(stack) : blendVal;
	val = std::max(std::min(val,1.f), 0.f);
	*(CFLOAT*)state.userdata = val;
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	BSDF_t matFlags = BSDF_NONE;
	bsdfTypes = BSDF_NONE;
	if(val<1.f) mat1->initBSDF(state, sp, bsdfTypes);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f) mat2->initBSDF(state, sp, matFlags);
	bsdfTypes |= matFlags;
	
	//todo: bump mapping blending
	state.userdata = old_udat;
}

color_t blendMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const
{
	CFLOAT val = *(CFLOAT*)state.userdata;
	color_t col(0.f);
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	if(val<1.f) col += mat1->eval(state, sp, wo, wl, bsdfs) * (1.f - val);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f) col += mat2->eval(state, sp, wo, wl, bsdfs) * val;
	state.userdata = old_udat;
	return col;
}

void blendMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							  bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const
{
	CFLOAT val = *(CFLOAT*)state.userdata;
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	reflect=false; refract=false;
	bool m1_reflect=false, m1_refract=false;
	vector3d_t m1_dir[2];
	color_t m1_col[2];
	if(val<1.f) mat1->getSpecular(state, sp, wo, m1_reflect, m1_refract, m1_dir, m1_col);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f) mat2->getSpecular(state, sp, wo, reflect, refract, dir, col);
	state.userdata = old_udat;
	
	if(reflect)
	{
		col[0] *= val;
		if(m1_reflect)
		{
			col[0] += m1_col[0] * (1.f - val);
			dir[0] = dir[0] * val + m1_dir[0] * (1.f - val);
		}
	}
	else if(m1_reflect)
	{
		col[0] = m1_col[0] * (1.f - val);
		dir[0] = m1_dir[0];
	}
	
	if(refract)
	{
		col[1] *= val;
		if(m1_refract)
		{
			col[1] += m1_col[1] * (1.f - val);
			dir[1] = dir[1] * val + m1_dir[1] * (1.f - val);
		}
	}
	else if(m1_refract)
	{
		col[1] = m1_col[1] * (1.f - val);
		dir[1] = m1_dir[1];
	}
	
	
	reflect = reflect || m1_reflect;
	refract = refract || m1_refract;
	if(reflect) dir[0].normalize();
	if(refract) dir[1].normalize();
}

color_t blendMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	CFLOAT val = *(CFLOAT*)state.userdata;
	color_t col(0.f);
	s.pdf = 0.f;
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	if(val < 1.f && s.s1 > val)
	{
		s.s1 = (s.s1 - val) / (1.f - val);
		col = mat1->sample(state, sp, wo, wi, s) * (1.f - val);
		s.pdf *= 1.f - val;
		if(!(s.sampledFlags & BSDF_SPECULAR))
		{
			state.userdata = PTR_ADD(state.userdata, mmem1);
			col += mat2->eval(state, sp, wo, wi, s.flags) * val;
			s.pdf += mat2->pdf(state, sp, wo, wi, s.flags) * val;
		}
	}
	else if(val > 0.f)
	{
		state.userdata = PTR_ADD(state.userdata, mmem1);
		s.s1 = s.s1 / val;
		col = mat2->sample(state, sp, wo, wi, s) * val;
		s.pdf *= val;
		if(!(s.sampledFlags & BSDF_SPECULAR))
		{
			state.userdata = PTR_ADD(state.userdata, -mmem1);
			col += mat1->eval(state, sp, wo, wi, s.flags) * (1.f - val);
			s.pdf += mat1->pdf(state, sp, wo, wi, s.flags) * (1.f - val);
		}
	}
	state.userdata = old_udat;
	return col;
}

float blendMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	CFLOAT val = *(CFLOAT*)state.userdata;
	float pdf = 0.f;
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	if(val<1.f) pdf += mat1->pdf(state, sp, wo, wi, bsdfs) * (1.f - val);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f) pdf += mat2->pdf(state, sp, wo, wi, bsdfs) * val;
	state.userdata = old_udat;
	return pdf;
}

bool blendMat_t::isTransparent() const
{
	return mat1->isTransparent() || mat2->isTransparent();
}

color_t blendMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	//create our "stack" to save node results
	nodeStack_t stack(state.userdata);
	evalNodes(state, sp, allNodes, stack);
	CFLOAT val = (blendS) ? blendS->getScalar(stack) : blendVal;
	val = std::max(std::min(val,1.f), 0.f);
	*(CFLOAT*)state.userdata = val;
	color_t col(0.f);
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	if(val<1.f && mat1->isTransparent()) col += mat1->getTransparency(state, sp, wo) * (1.f - val);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f && mat2->isTransparent()) col += mat2->getTransparency(state, sp, wo) * val;
	
	state.userdata = old_udat;
	return col;
}

color_t blendMat_t::volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const
{
	return color_t(1.,1.,1.);
}

color_t blendMat_t::emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	CFLOAT val = *(CFLOAT*)state.userdata;
	color_t col(0.f);
	void *old_udat = state.userdata;
	state.userdata = PTR_ADD(state.userdata, sizeof(CFLOAT));
	if(val<1.f) col += mat1->emit(state, sp, wo) * (1.f - val);
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f) col += mat2->emit(state, sp, wo) * val;
	state.userdata = old_udat;
	return col;
}

material_t* blendMat_t::factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &env)
{
	const std::string *name = 0;
	const material_t *m1=0, *m2=0;
	double blend_val = 0.5;
	
	if(! params.getParam("material1", name) ) return 0;
	m1 = env.getMaterial(*name);
	if(! params.getParam("material2", name) ) return 0;
	m2 = env.getMaterial(*name);
	params.getParam("blend_value", blend_val);
	//if(! params.getParam("mask", name) ) return 0;
	//mask = env.getTexture(*name);
	
	if(m1==0 || m2==0 ) return 0;
	
	blendMat_t *mat = new blendMat_t(m1, m2, blend_val);
	
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(eparams, env))
	{
		if(params.getParam("mask", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->blendS = i->second; roots.push_back(mat->blendS); }
			else
			{
				std::cout << "[ERROR]: blend shader node '"<<*name<<"' does not exist!\n";
				delete mat;
				return 0;
			}
		}
	}
	else
	{
		std::cout << "[ERROR]: loadNodes() failed!\n";
		delete mat;
		return 0;
	}
	mat->solveNodesOrder(roots);
	size_t inputReq = std::max(m1->getReqMem(), m2->getReqMem());
	mat->reqMem = std::max( mat->reqNodeMem, sizeof(bool) + inputReq);
	return mat;
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("blend_mat", blendMat_t::factory);
	}

}

__END_YAFRAY
