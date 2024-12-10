
#include <materials/shinydiff.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

shinyDiffuseMat_t::shinyDiffuseMat_t(const color_t &col, const color_t &srcol, float diffuse, float transp, float transl, float sp_refl, float emit):
			isTranspar(false), isTransluc(false), isReflective(false), isDiffuse(false), fresnelEffect(false),
			diffuseS(0), bumpS(0), transpS(0), translS(0), specReflS(0), mirColS(0), color(col), specRefCol(srcol),
			mSpecRefl(sp_refl), mTransp(transp), mTransl(transl), mDiffuse(diffuse), orenNayar(false), nBSDF(0)
{
	emitCol = emit*col;
	emitVal = emit;
	mDiffuse = diffuse;
	bsdfFlags = BSDF_NONE;
}

shinyDiffuseMat_t::~shinyDiffuseMat_t()
{
	//already done in ~nodeMaterial_t();
	//clear nodes map:
	//for(std::map<std::string,shaderNode_t *>::iterator i=shader_table.begin();i!=shader_table.end();++i) delete i->second;
	//shader_table.clear();
}

/*! ATTENTION! You *MUST* call this function before using the material, no matter
	if you want to use shaderNodes or not!
*/
void shinyDiffuseMat_t::config(shaderNode_t *diff, shaderNode_t *refl, shaderNode_t *transp, shaderNode_t *transl, shaderNode_t *bump)
{
	diffuseS = diff;
	bumpS = bump;
	transpS = transp;
	translS = transl;
	specReflS = refl;
	nBSDF=0;
	viNodes[0] = viNodes[1] = viNodes[2] = viNodes[3] = false;
	vdNodes[0] = vdNodes[1] = vdNodes[2] = vdNodes[3] = false;
	float acc = 1.f;
	if(mSpecRefl > 0.00001f || specReflS)
	{
		isReflective = true;
		if(specReflS){ if(specReflS->isViewDependant())vdNodes[0] = true; else viNodes[0] = true; }
		else if(!fresnelEffect) acc = 1.f - mSpecRefl;
		bsdfFlags |= BSDF_SPECULAR | BSDF_REFLECT;
		cFlags[nBSDF] = BSDF_SPECULAR | BSDF_REFLECT;
		cIndex[nBSDF] = 0;
//		std::cout << "(mSpecRefl:" << mSpecRefl << ", node:" << (bool)(specReflS!=0) << ")";
		++nBSDF;
	}
	if(mTransp*acc > 0.00001f || transpS)
	{
		isTranspar = true;
		if(transpS){ if(transpS->isViewDependant())vdNodes[1] = true; else viNodes[1] = true; }
		else acc *= 1.f - mTransp;
		bsdfFlags |= BSDF_TRANSMIT | BSDF_FILTER;
		cFlags[nBSDF] = BSDF_TRANSMIT | BSDF_FILTER;
		cIndex[nBSDF] = 1;
//		std::cout << "(mTransp:" << mTransp << ", node:" << (bool)(transpS!=0) << ")";
		++nBSDF;
	}
	if(mTransl*acc > 0.00001f || translS)
	{
		isTransluc = true;
		if(translS){ if(translS->isViewDependant())vdNodes[2] = true; else viNodes[2] = true; }
		else acc *= 1.f - mTransp;
		bsdfFlags |= BSDF_DIFFUSE | BSDF_TRANSMIT;
		cFlags[nBSDF] = BSDF_DIFFUSE | BSDF_TRANSMIT;
		cIndex[nBSDF] = 2;
//		std::cout << "(mTransl:" << mTransl << ", node:" << (bool)(translS!=0) << ")";
		++nBSDF;
	}
	if(mDiffuse*acc > 0.00001f)
	{
		isDiffuse = true;
		if(diffuseS){ if(diffuseS->isViewDependant())vdNodes[3] = true; else viNodes[3] = true; }
		bsdfFlags |= BSDF_DIFFUSE | BSDF_REFLECT;
		cFlags[nBSDF] = BSDF_DIFFUSE | BSDF_REFLECT;
		cIndex[nBSDF] = 3;
//		std::cout << "(mDiffuse:" << mDiffuse << ")";
		++nBSDF;
	}
	reqMem = reqNodeMem + sizeof(SDDat_t);
//	std::cout << std::endl;
}

// component should be initialized with mSpecRefl, mTransp, mTransl, mDiffuse
// since values for which useNode is false do not get touched so it can be applied
// twice, for view-independent (initBSDF) and view-dependent (sample/eval) nodes

int shinyDiffuseMat_t::getComponents(const bool *useNode, nodeStack_t &stack, float *component) const
{
	if(isReflective)
	{
		component[0] = useNode[0] ? specReflS->getScalar(stack) : mSpecRefl;
	}
	if(isTranspar)
	{
		component[1] = useNode[1] ? transpS->getScalar(stack) : mTransp;
	}
	if(isTransluc)
	{
		component[2] = useNode[2] ? translS->getScalar(stack) : mTransl;
	}
	if(isDiffuse)
	{
		component[3] = mDiffuse;
	}
	return 0;
}

inline CFLOAT shinyDiffuseMat_t::getFresnel(const vector3d_t &wo, const vector3d_t &N) const
{
	CFLOAT Kr=1.f, Kt;
	if(fresnelEffect)
	{
		fresnel(wo, N, IOR, Kr, Kt);
		return Kr;
	}
	else return 1.f;
}

// calculate the absolute value of scattering components from the "normalized"
// fractions which are between 0 (no scattering) and 1 (scatter all remaining light)
// Kr is an optional reflection multiplier (e.g. from Fresnel)
static inline void accumulate(const float *component, float *accum, float Kr)
{
	accum[0] = component[0]*Kr;
	float acc = 1.f - accum[0];
	accum[1] = component[1] * acc;
	acc *= 1.f - component[1];
	accum[2] = component[2] * acc;
	acc *= 1.f - component[2];
	accum[3] = component[3] * acc;
}

void shinyDiffuseMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	SDDat_t *dat = (SDDat_t *)state.userdata;
	memset(dat, 0, 8*sizeof(float));
	dat->nodeStack = (char*)state.userdata + sizeof(SDDat_t);
	//create our "stack" to save node results
	nodeStack_t stack(dat->nodeStack);
	
	//bump mapping (extremely experimental)
	if(bumpS)
	{
		evalBump(stack, state, sp, bumpS);
	}
	
	//eval viewindependent nodes
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
	
	getComponents(viNodes, stack, dat->component);
}

void shinyDiffuseMat_t::initOrenNayar(double sigma)
{
	double sigma2 = sigma*sigma;
	A = 1.0 - 0.5*(sigma2 / (sigma2+0.33));
	B = 0.45 * sigma2 / (sigma2 + 0.09);
	orenNayar = true;
}

CFLOAT shinyDiffuseMat_t::OrenNayar(const vector3d_t &wi, const vector3d_t &wo, const vector3d_t &N) const
{
	PFLOAT cos_ti = N*wi;
	if (cos_ti<=0.f) return 0.f;
	PFLOAT cos_to = N*wo;
	if (cos_to<=0.f) cos_to=0.f;
	CFLOAT maxcos_f = 0.f;
	if(cos_ti < 0.9999f && cos_to < 0.9999f)
	{
		vector3d_t v1 = (wi - N*cos_ti).normalize();
		vector3d_t v2 = (wo - N*cos_to).normalize();
		maxcos_f = std::max(0.f, v1*v2);
	}
	CFLOAT sin_alpha, tan_beta;
	if(cos_to > cos_ti)
	{
		sin_alpha = sqrt(1.f - cos_ti*cos_ti);
		tan_beta = sqrt(1.f - cos_to*cos_to) / cos_to;
	}
	else
	{
		sin_alpha = sqrt(1.f - cos_to*cos_to);
		tan_beta = sqrt(1.f - cos_ti*cos_ti) / cos_ti;
	}
	return A + B * maxcos_f * sin_alpha * tan_beta;
}


color_t shinyDiffuseMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const
{
	PFLOAT cos_Ng_wo = sp.Ng*wo;
	PFLOAT cos_Ng_wl = sp.Ng*wl;
	// face forward:
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	if(!(bsdfs & bsdfFlags & BSDF_DIFFUSE)) return color_t(0.f);
	
	SDDat_t *dat = (SDDat_t *)state.userdata;
	nodeStack_t stack(dat->nodeStack);
//	std::vector<shaderNode_t *>::const_iterator iter, end=allViewdep.end();
//	for(iter = allViewdep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp, wo, wl);
	
//	getComponents(vdNodes, stack, dat->component); //that's wrong! we can't eval viewdependant nodes yet...
	CFLOAT Kr = getFresnel(wo, N);
	float mT = (1.f - Kr*dat->component[0])*(1.f - dat->component[1]);
	
	bool transmit = ( cos_Ng_wo * cos_Ng_wl ) < 0;
	if(transmit) // light comes from opposite side of surface
	{
		if(isTransluc) return dat->component[2] * mT * (diffuseS ? diffuseS->getColor(stack) : color);
	}
	else
	{
		if(N*wl <= 0.0) return color_t(0.f);
		float mD = mT*(1.f - dat->component[2]) * dat->component[3];
		if(orenNayar) mD *= OrenNayar(wo, wl, N);
		return mD * (diffuseS ? diffuseS->getColor(stack) : color);
	}
	return color_t(0.f);
}

color_t shinyDiffuseMat_t::emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	SDDat_t *dat = (SDDat_t *)state.userdata;
	nodeStack_t stack(dat->nodeStack);
	
	return (diffuseS ? diffuseS->getColor(stack) * emitVal : emitCol);
}

color_t shinyDiffuseMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	float accumC[4];
	PFLOAT cos_Ng_wo = sp.Ng*wo, cos_Ng_wi, cos_N;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	
	SDDat_t *dat = (SDDat_t *)state.userdata;
	nodeStack_t stack(dat->nodeStack);
//	std::vector<shaderNode_t *>::const_iterator iter, end=allViewdep.end();
//	for(iter = allViewdep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp, wo, wl);
	
//	getComponents(vdNodes, stack, dat->component);
	CFLOAT Kr = getFresnel(wo, N);
	accumulate(dat->component, accumC, Kr);
	
	float sum=0.f, val[4], width[4];
	BSDF_t choice[4];
	int nMatch=0, pick=-1;
	for(int i=0; i<nBSDF; ++i)
	{
		if((s.flags & cFlags[i]) == cFlags[i])
		{
			width[nMatch] = accumC[cIndex[i]];
			sum += width[nMatch];
			choice[nMatch] = cFlags[i];
			val[nMatch] = sum;
			++nMatch;
		}
	}
	if(!nMatch || sum < 0.00001){ s.sampledFlags=BSDF_NONE; s.pdf=0.f; return color_t(1.f); }
	float inv_sum = 1.f/sum;
	for(int i=0; i<nMatch; ++i)
	{
		val[i] *= inv_sum;
		width[i] *= inv_sum;
		if((s.s1 <= val[i]) && (pick<0 ))	pick = i;
	}
	if(pick<0) pick=nMatch-1;
	float s1;
	if(pick>0) s1 = (s.s1 - val[pick-1]) / width[pick];
	else 	   s1 = s.s1 / width[pick];
	
	color_t scolor(0.f);
	switch(choice[pick])
	{
		case (BSDF_SPECULAR | BSDF_REFLECT): // specular reflect
			wi = reflect_dir(N, wo);
			s.pdf = width[pick]; 
			scolor = (mirColS ? mirColS->getColor(stack) : specRefCol) * (accumC[0]);
			if(s.reverse)
			{
				s.pdf_back = s.pdf;
				s.col_back = scolor/std::fabs(sp.N*wo);
			}
			scolor *= 1.f/std::fabs(sp.N*wi);
			break;
		case (BSDF_TRANSMIT | BSDF_FILTER): // "specular" transmit
			wi = -wo;
			scolor = accumC[1] * (filter*(diffuseS ? diffuseS->getColor(stack) : color) + color_t(1.f-filter) );
			cos_N = std::fabs(wi*N);
			if(cos_N < 1e-6) s.pdf = 0.f;
			else
			{
				scolor *= 1.f/CFLOAT(cos_N);
				s.pdf = width[pick];
			}
			break;
		case (BSDF_DIFFUSE | BSDF_TRANSMIT): // translucency (diffuse transmitt)
			wi = SampleCosHemisphere(-N, sp.NU, sp.NV, s1, s.s2);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) scolor = accumC[2] * (diffuseS ? diffuseS->getColor(stack) : color);
			//else if(isDiffuse) scolor = accumC[3] * (diffuseS ? diffuseS->getColor(stack) : color);
			s.pdf = std::abs(wi*N) * width[pick]; break;
		case (BSDF_DIFFUSE | BSDF_REFLECT): // diffuse reflect
		default:
			wi = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s.s2);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi > 0) scolor = accumC[3] * (diffuseS ? diffuseS->getColor(stack) : color);
			if(orenNayar) scolor *= OrenNayar(wo, wi, N);
			//else if(isTransluc) scolor = accumC[2] * (diffuseS ? diffuseS->getColor(stack) : color);
			s.pdf = std::abs(wi*N) * width[pick]; break;
	}
	s.sampledFlags = choice[pick];
	
	return scolor;
}

float shinyDiffuseMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	if(!(bsdfs & BSDF_DIFFUSE)) return 0.f;
	
	SDDat_t *dat = (SDDat_t *)state.userdata;
	float pdf=0.f;
	float accumC[4];
	PFLOAT cos_Ng_wo = sp.Ng*wo, cos_Ng_wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	CFLOAT Kr = getFresnel(wo, N);
	accumulate(dat->component, accumC, Kr);
	float sum=0.f, width;
	int nMatch=0;
	for(int i=0; i<nBSDF; ++i)
	{
		if((bsdfs & cFlags[i]) == cFlags[i])
		{
			width = accumC[cIndex[i]];
			sum += width;
			
			switch(cFlags[i])
			{
				case (BSDF_DIFFUSE | BSDF_TRANSMIT): // translucency (diffuse transmitt)
					cos_Ng_wi = sp.Ng*wi;
					if(cos_Ng_wo*cos_Ng_wi < 0) pdf += std::abs(wi*N) * width; break;
				
				case (BSDF_DIFFUSE | BSDF_REFLECT): // lambertian
					cos_Ng_wi = sp.Ng*wi;
					if(cos_Ng_wo*cos_Ng_wi > 0) pdf += std::abs(wi*N) * width; break;
			}
			++nMatch;
		}
	}
	if(!nMatch || sum < 0.00001) return 0.f;
	return pdf / sum;
}



// todo!

void shinyDiffuseMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							  bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const
{
	SDDat_t *dat = (SDDat_t *)state.userdata;
	nodeStack_t stack(dat->nodeStack);
	bool backface = sp.Ng * wo < 0;
	vector3d_t N = backface ? -sp.N : sp.N;
	vector3d_t Ng = backface ? -sp.Ng : sp.Ng;
	CFLOAT Kr = getFresnel(wo, N);
	refract = isTranspar;
	if(isTranspar)
	{
		dir[1] = -wo;
		color_t tcol = filter*(diffuseS ? diffuseS->getColor(stack) : color) + color_t(1.f-filter);
		col[1] = (1.f - dat->component[0]*Kr) * dat->component[1] * tcol;
	}
	reflect=isReflective;
	if(isReflective)
	{
		dir[0] = reflect_plane(N, wo);
		PFLOAT cos_wi_Ng = dir[0]*Ng;
		if(cos_wi_Ng < 0.01){ dir[0] += (0.01-cos_wi_Ng)*Ng; dir[0].normalize(); }
		col[0] = (mirColS ? mirColS->getColor(stack) : specRefCol) * (dat->component[0]*Kr);
	}
}

color_t shinyDiffuseMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	nodeStack_t stack(state.userdata);
	std::vector<shaderNode_t *>::const_iterator iter, end=allSorted.end();
	for(iter = allSorted.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	float accum=1.f;
	if(isReflective)
	{
		vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
		CFLOAT Kr = getFresnel(wo, N);
		accum = 1.f - Kr*(specReflS ? specReflS->getScalar(stack) : mSpecRefl);
	}
	if(isTranspar) //uhm...should actually be true if this function gets called anyway...
	{
		accum *= transpS ? transpS->getScalar(stack) * accum : mTransp * accum;
	}
	color_t tcol = filter*(diffuseS ? diffuseS->getColor(stack) : color) + color_t(1.f-filter);
	return accum * tcol;
}

CFLOAT shinyDiffuseMat_t::getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	SDDat_t *dat = (SDDat_t *)state.userdata;
	if(isTranspar)
	{
		vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
		CFLOAT Kr = getFresnel(wo, N);
		CFLOAT refl = (1.f - dat->component[0]*Kr) * dat->component[1];
		return 1.f - refl;
	}
	return 1.f;
}

bool shinyDiffuseMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const
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

material_t* shinyDiffuseMat_t::factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &render)
{
	shinyDiffuseMat_t *mat;
	color_t col=1.f, srCol=1.f;
	const std::string *name=0;
	float transp=0.f, emit=0.f, transl=0.f;
	float sp_refl=0.f;
	bool fresnEff=false;
	double IOR = 1.33, filt=1.0;
	CFLOAT diffuse=1.f;
	//bool error=false;
	params.getParam("color", col);
	params.getParam("mirror_color", srCol);
	params.getParam("transparency", transp);
	params.getParam("translucency", transl);
	params.getParam("diffuse_reflect", diffuse);
	params.getParam("specular_reflect", sp_refl);
	params.getParam("emit", emit);
	params.getParam("IOR", IOR);
	params.getParam("fresnel_effect", fresnEff);
	params.getParam("transmit_filter", filt);
	// !!remember to put diffuse multiplier in material itself!
	mat = new shinyDiffuseMat_t(col, srCol, diffuse, transp, transl, sp_refl, emit);
	mat->filter = filt;
	
	if(fresnEff)
	{
		mat->IOR = IOR;
		mat->fresnelEffect = true;
	}
	if(params.getParam("diffuse_brdf", name))
	{
		if(*name == "oren_nayar")
		{
			double sigma=0.1;
			params.getParam("sigma", sigma);
			mat->initOrenNayar(sigma);
		}
	}
	shaderNode_t *diffuseS=0, *bumpS=0, *specReflS=0, *transpS=0, *translS=0;
	std::vector<shaderNode_t *> roots;
	// create shader nodes:
	bool success = mat->loadNodes(eparams, render);
	if(success)
	{
		if(params.getParam("diffuse_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ diffuseS = i->second; roots.push_back(diffuseS); }
			else std::cout << "[WARNING]: diffuse shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("mirror_color_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->mirColS = i->second; roots.push_back(mat->mirColS); }
			else std::cout << "[WARNING]: mirror col. shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("bump_shader", name))
		{
			std::cout << "bump_shader: " << name << std::endl;
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ bumpS = i->second; roots.push_back(bumpS); }
			else std::cout << "[WARNING]: bump shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("mirror_shader", name))
		{
			std::cout << "mirror_shader: " << name << std::endl;
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ specReflS = i->second; roots.push_back(specReflS); }
			else std::cout << "[WARNING]: mirror shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("transparency_shader", name))
		{
			std::cout << "transparency_shader: " << name << std::endl;
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ transpS = i->second; roots.push_back(transpS); }
			else std::cout << "[WARNING]: transparency shader node '"<<*name<<"' does not exist!\n";
		}
		if(params.getParam("translucency_shader", name))
		{
			std::cout << "translucency_shader: " << name << std::endl;
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ translS = i->second; roots.push_back(translS); }
			else std::cout << "[WARNING]: transparency shader node '"<<*name<<"' does not exist!\n";
		}
	}
	else std::cout << "creating nodes failed!" << std::endl;
	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::cout << "evaluation order:\n";
		for(unsigned int k=0; k<mat->allSorted.size(); ++k) std::cout << (void*)mat->allSorted[k]<<"\n";
		std::vector<shaderNode_t *> colorNodes;
		if(diffuseS) mat->getNodeList(diffuseS, colorNodes);
		if(mat->mirColS) mat->getNodeList(mat->mirColS, colorNodes);
		if(specReflS) mat->getNodeList(specReflS, colorNodes);
		if(transpS) mat->getNodeList(transpS, colorNodes);
		if(translS) mat->getNodeList(translS, colorNodes);
		mat->filterNodes(colorNodes, mat->allViewdep, VIEW_DEP);
		mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);
		if(bumpS)
		{
			mat->getNodeList(bumpS, mat->bumpNodes);
		}
	}
	mat->config(diffuseS, specReflS, transpS, translS, bumpS);
	//===!!!=== test
	if(params.getParam("name", name))
	{
		//std::cout << name->substr(0, 6) << std::endl;
		if(name->substr(0, 6) == "MAsss_")
		{
			paraMap_t map;
			map["type"] = std::string("sss");
			map["absorption_col"] = color_t(0.5f, 0.2f, 0.2f);
			map["absorption_dist"] = 0.5f;
			map["scatter_col"] = color_t(0.9f);
			mat->volI = render.createVolumeH(*name, map);
			mat->bsdfFlags |= BSDF_VOLUMETRIC;
		}
		//else std::cout << "not creating volume\n";
	}
	//===!!!=== end of test
	return mat;
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("shinydiffusemat", shinyDiffuseMat_t::factory);
	}

}

__END_YAFRAY
