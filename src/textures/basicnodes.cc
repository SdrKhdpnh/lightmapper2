
#include <textures/basicnodes.h>
#include <textures/layernode.h>
#include <core_api/object3d.h>

__BEGIN_YAFRAY

textureMapper_t::textureMapper_t(const texture_t *texture): vmap(0), tex(texture), bumpStr(0.02), doScalar(true)
{
	map_x=1; map_y=2, map_z=3;
}

void textureMapper_t::setup()
{
	if(tex->discrete())
	{
		int u, v, w;
		tex->resolution(u, v, w);
		deltaU = 1.0/(PFLOAT)u;
		deltaV = 1.0/(PFLOAT)v;
		if(tex->isThreeD())
		{
			deltaW = 1.0/(PFLOAT)w;
			delta = sqrt(deltaU*deltaU + deltaV*deltaV + deltaW*deltaW);
		}
		else delta = sqrt(deltaU*deltaU + deltaV*deltaV);
	}
	else
	{
		deltaU = 0.0002;
		deltaV = 0.0002;
		deltaW = 0.0002;
		delta  = 0.0002;
	}
	PFLOAT mapScale = scale.length();
	delta /= mapScale;
	bumpStr /= mapScale;
}

inline point3d_t tubemap(const point3d_t &p)
{
	point3d_t res;
	res.x = 0;
	res.y = 1 - (p.z + 1)*0.5;
	PFLOAT d = p.x*p.x + p.y*p.y;
	if (d>0) {
		res.z = d = 1/sqrt(d);
		res.x = 0.5*(1 - (atan2(p.x*d, p.y*d) *M_1_PI));
	}
	else res.z = 0;
	return res;
}

inline point3d_t spheremap(const point3d_t &p)
{
	point3d_t res(0.f);
	PFLOAT d = p.x*p.x + p.y*p.y + p.z*p.z;
	if (d>0) {
		res.z = sqrt(d);
		if ((p.x!=0) && (p.y!=0)) res.x = 0.5*(1 - atan2(p.x, p.y)*M_1_PI);
		res.y = acos(p.z/res.z) * M_1_PI;
	}
	return res;
}

inline point3d_t cubemap(const point3d_t &p, const vector3d_t &n)
{
	const int ma[3][3] = { {1, 2, 0}, {0, 2, 1}, {0, 1, 2} };
	int axis = n.x > n.y ? (n.x > n.z ? n.x : n.z) : (n.y > n.z ? n.y : n.z);
	return point3d_t(p[ma[axis][0]], p[ma[axis][1]], p[ma[axis][2]]);
}

point3d_t textureMapper_t::doMapping(const point3d_t &p, const vector3d_t &N)const
{
	/* switch(tex_coords)
	{
		case TXC_UV: 	return vector3d_t(0.f); // bad idea to call for uv-mapping!
		case TXC_GLOB:	return v;
		default: 		return v;
	} */
	point3d_t texpt(p);
	if(tex_coords == TXC_TRAN) texpt = mtx * texpt;
	PFLOAT texmap[4] = {0, texpt.x, texpt.y, texpt.z};
	texpt.x=texmap[map_x];
	texpt.y=texmap[map_y];
	texpt.z=texmap[map_z];
	// projection:
	switch(tex_maptype)
	{
		case TXP_TUBE: texpt = tubemap(texpt); break;
		case TXP_SPHERE: texpt = spheremap(texpt); break;
		case TXP_CUBE: texpt = cubemap(texpt, N); break;
		case TXP_PLAIN: // nothing to do for "plain"
		default: break;
	}
	// scale and offset:
	texpt = mult((texpt+offset), scale);
	return texpt;
}

point3d_t eval_uv(const surfacePoint_t &sp, int vmap)
{
	if(vmap == 0)
	{
		return point3d_t(sp.U, sp.V, 0.f);
	}
	else
	{
		float vmval[4];
		point3d_t p(0,0,0);
		int dim = sp.object->evalVMap(sp, vmap, vmval);
		if(dim < 2) return p;
		else
		{
			p.x = vmval[0];
			p.y = vmval[1];
			if(dim>2) p.z = vmval[2];
		}
		return p;
	}
}

void textureMapper_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	point3d_t texpt;
	switch(tex_coords)
	{
		case TXC_UV: 	texpt = eval_uv(sp, vmap); break; //texpt = point3d_t(sp.U, sp.V, 0.f); break;
		case TXC_GLOB:	texpt = sp.P; break;
		case TXC_ORCO:  texpt = sp.orcoP; break;
		case TXC_TRAN:  texpt = mtx * sp.P; break;
		default: 		texpt = sp.P;
	}
	// map axis:
	PFLOAT texmap[4] = {0, texpt.x, texpt.y, texpt.z};
	texpt.x=texmap[map_x];
	texpt.y=texmap[map_y];
	texpt.z=texmap[map_z];
	// projection:
	switch(tex_maptype)
	{
		case TXP_TUBE: texpt = tubemap(texpt); break;
		case TXP_SPHERE: texpt = spheremap(texpt); break;
		case TXP_CUBE: texpt = cubemap(texpt, sp.Ng); break;
		case TXP_PLAIN: // nothing to do for "plain"
		default: break;
	}
	// scale and offset:
	texpt = mult((texpt+offset), scale);
	
	stack[this->ID] = nodeResult_t(tex->getColor(texpt), (doScalar) ? tex->getFloat(texpt) : 0.f );
}

//basically you shouldn't call this anyway, but for the sake of consistency, redirect:
void textureMapper_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const
{
	eval(stack, state, sp);
}

void textureMapper_t::evalDerivative(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	static bool debug=true;
	CFLOAT du, dv;
	if(tex_coords == TXC_UV)
	{
		point3d_t p1 = point3d_t(sp.U+deltaU, sp.V, 0.f);
		point3d_t p2 = point3d_t(sp.U-deltaU, sp.V, 0.f);
		CFLOAT dfdu = ( tex->getFloat(p1) - tex->getFloat(p2) ) / deltaU;
		p1 = point3d_t(sp.U, sp.V+deltaV, 0.f);
		p2 = point3d_t(sp.U, sp.V-deltaV, 0.f);
		CFLOAT dfdv = ( tex->getFloat(p1) - tex->getFloat(p2) ) / deltaV;
		// now we got the derivative in UV-space, but need it in shading space:
		vector3d_t vecU = /* deltaU * */ sp.dSdU;
		vector3d_t vecV = /* deltaV * */ sp.dSdV;
		vecU.z = dfdu;
		vecV.z = dfdv;
		// now we have two vectors NU/NV/df; Solve plane equation to get 1/0/df and 0/1/df (i.e. dNUdf and dNVdf)
		vector3d_t norm = vecU ^ vecV;
		if(fabs(norm.z) > 1e-30f)
		{
			PFLOAT NF = 1.0/norm.z * bumpStr;
			du = -norm.x*NF;
			dv = -norm.y*NF;
		}
		else du = dv = 0.f;
		if(debug)
		{
			std::cout << "deltaU:" << deltaU << ", deltaV:" << deltaV << std::endl;
			std::cout << "dfdu:" << dfdu << ", dfdv:" << dfdv << std::endl;
			std::cout << "vecU:" << vecU << ", vecV:" << vecV << ", norm:" << norm << std::endl;
			std::cout << "du:" << du << ", dv:" << dv << std::endl;
		}
	}
	else
	{
		/* point3d_t pT1, pT2;
		vector3d_t tNU = vecToTexspace(sp.P, sp.NU);
		vector3d_t tNV = vecToTexspace(sp.P, sp.NV);
		PFLOAT l1 = tNU.length();
		PFLOAT l2 = tNV.length();
		PFLOAT d1 = delta/l1, d2 = delta/l2;
		tNU *= d1;
		tNV *= d2; */
		// todo: handle out-of-range from doRawMapping!
		point3d_t texPt;
		//doMapping(...)
		//placeholder!
		texPt = sp.P;
		du = bumpStr * (  tex->getFloat(doMapping(texPt+delta*sp.NU, sp.Ng))
						- tex->getFloat(doMapping(texPt-delta*sp.NU, sp.Ng)) ) / delta;
		dv = bumpStr * (  tex->getFloat(doMapping(texPt+delta*sp.NV, sp.Ng))
						- tex->getFloat(doMapping(texPt-delta*sp.NV, sp.Ng)) ) / delta;
	}
	stack[this->ID] = nodeResult_t(colorA_t(du, dv, 0.f, 0.f), 0.f );
	debug=false;
}

shaderNode_t* textureMapper_t::factory(const paraMap_t &params,renderEnvironment_t &render)
{
	const texture_t *tex=0;
	const std::string *texname=0, *option=0;
	TEX_COORDS tc = TXC_GLOB;
	TEX_PROJ maptype = TXP_PLAIN;
	float bumpStr = 1.f;
	bool scalar=true;
	int vmap=0;
	int map[3] = { 1, 2, 3 };
	point3d_t offset(0.f), scale(1.f);
	matrix4x4_t mtx(1);
	if( !params.getParam("texture", texname) )
	{
		std::cerr << "[ERROR]: no texture given for texture mapper!";
		return 0;
	}
	tex = render.getTexture(*texname);
	if(!tex)
	{
		std::cerr << "[ERROR]: texture '"<<texname<<"' does not exist!";
		return 0;
	}
	textureMapper_t *tm = new textureMapper_t(tex);
	if(params.getParam("texco", option) )
	{
		if(*option == "uv") tc = TXC_UV;
		else if(*option == "global") tc = TXC_GLOB;
		else if(*option == "orco") tc = TXC_ORCO;
		else if(*option == "transformed") tc = TXC_TRAN;
	}
	if(params.getParam("mapping", option) )
	{
		if(*option == "plain") maptype = TXP_PLAIN;
		else if(*option == "cube") maptype = TXP_CUBE;
		else if(*option == "tube") maptype = TXP_TUBE;
		else if(*option == "sphere") maptype = TXP_SPHERE;
	}
	params.getParam("vmap", vmap);
	params.getMatrix("transform", mtx);
	params.getParam("scale", scale);
	params.getParam("offset", offset);
	params.getParam("do_scalar", scalar);
	params.getParam("bump_strength", bumpStr);
	params.getParam("proj_x", map[0]);
	params.getParam("proj_y", map[1]);
	params.getParam("proj_z", map[2]);
	for(int i=0; i<3; ++i) map[i] = std::min(3, std::max(0, map[i]));
	tm->tex_coords = tc;
	tm->tex_maptype = maptype;
	tm->map_x = map[0];
	tm->map_y = map[1];
	tm->map_z = map[2];
	tm->vmap = vmap;
	tm->scale = vector3d_t(scale);
	tm->offset = vector3d_t(offset);
	tm->doScalar = scalar;
	tm->bumpStr = 0.02f * bumpStr;
	tm->mtx = mtx;
	tm->setup();
	return tm;
}

/* ==========================================
/  The most simple node you could imagine...
/ ========================================== */

void valueNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	stack[this->ID] = nodeResult_t(color, value);
}

void valueNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const
{
	stack[this->ID] = nodeResult_t(color, value);
}

shaderNode_t* valueNode_t::factory(const paraMap_t &params,renderEnvironment_t &render)
{
	color_t col(1.f);
	float alpha=1.f;
	float val=1.f;
	params.getParam("color", col);
	params.getParam("alpha", alpha);
	params.getParam("scalar", val);
	return new valueNode_t(colorA_t(col, alpha), val);
}

/* ==========================================
/  A node that evaluates a vmap
/ ========================================== */

vcolorNode_t::vcolorNode_t(colorA_t dcol, int vmap_id): defcol(dcol), vmap(vmap_id) {}

void vcolorNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	float vcol[4];
	colorA_t col;
	int dim = sp.object->evalVMap(sp, vmap, vcol);
	if(dim == 3) col.set(vcol[0], vcol[1], vcol[2], 1.f);
	else if(dim == 4) col.set(vcol[0], vcol[1], vcol[2], vcol[3]);
	else col = defcol;
	stack[this->ID] = nodeResult_t(col, col.energy());
}
	
void vcolorNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const
{
	eval(stack, state, sp);
}

shaderNode_t* vcolorNode_t::factory(const paraMap_t &params,renderEnvironment_t &render)
{
	colorA_t default_col(1.f);
	int ID = 0;
	params.getParam("default_color", default_col);
	params.getParam("vmap", ID);
	
	return new vcolorNode_t(default_col, ID);
}

/* ==========================================
/  A simple mix node, could be used to derive other math nodes
/ ========================================== */

mixNode_t::mixNode_t(): cfactor(0.f), input1(0), input2(0), factor(0)
{}

mixNode_t::mixNode_t(float val): cfactor(val), input1(0), input2(0), factor(0)
{}

void mixNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
{
	float f2 = (factor) ? factor->getScalar(stack) : cfactor;
	float f1 = 1.f - f2, fin1, fin2;
	colorA_t cin1, cin2;
	if(input1)
	{
		cin1 = input1->getColor(stack);
		fin1 = input1->getScalar(stack);
	}
	else
	{
		cin1 = col1;
		fin1 = val1;
	}
	if(input2)
	{
		cin2 = input2->getColor(stack);
		fin2 = input2->getScalar(stack);
	}
	else
	{
		cin2 = col2;
		fin2 = val2;
	}
	
	colorA_t color = f1 * cin1 + f2 * cin2;
	float   scalar = f1 * fin1 + f2 * fin2;
	stack[this->ID] = nodeResult_t(color, scalar);
}

void mixNode_t::eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi)const
{
	eval(stack, state, sp);
}

bool mixNode_t::configInputs(const paraMap_t &params, const nodeFinder_t &find)
{
	const std::string *name=0;
	if( params.getParam("input1", name) )
	{
		input1 = find(*name);
		if(!input1){ std::cout << "mixNode_t::configInputs: couldn't get input1 " << *name << std::endl; return false; }
	}
	else if(!params.getParam("color1", col1)){ std::cout << "mixNode_t::configInputs: color1 not set\n"; return false; }
	
	if( params.getParam("input2", name) )
	{
		input2 = find(*name);
		if(!input2){ std::cout << "mixNode_t::configInputs: couldn't get input2 " << *name << std::endl; return false; }
	}
	else if(!params.getParam("color2", col2)){ std::cout << "mixNode_t::configInputs: color2 not set\n"; return false; }
	
	if( params.getParam("factor", name) )
	{
		factor = find(*name);
		if(!factor){ std::cout << "mixNode_t::configInputs: couldn't get factor " << *name << std::endl; return false; }
	}
	else if(!params.getParam("value", cfactor)){ std::cout << "mixNode_t::configInputs: value not set\n"; return false; }
	return true;
}

bool mixNode_t::getDependencies(std::vector<const shaderNode_t*> &dep) const
{
	if(input1) dep.push_back(input1);
	if(input2) dep.push_back(input2);
	if(factor) dep.push_back(factor);
	return !dep.empty();
}

class addNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			
			cin1 += f2 * cin2;
			fin1 += f2 * fin2;
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class multNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f1, f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			f1 = 1.f - f2;
			
			cin1 *= colorA_t(f1) + f2 * cin2;
			fin2 *= f1 + f2 * fin2;
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class subNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			
			cin1 -= f2 * cin2;
			fin1 -= f2 * fin2;
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class screenNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f1, f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			f1 = 1.f - f2;
			
			colorA_t color = colorA_t(1.f) - (colorA_t(f1) + f2 * (1.f - cin2)) * (1.f - cin1);
			CFLOAT scalar   = 1.0 - (f1 + f2*(1.f - fin2)) * (1.f -  fin1);
			stack[this->ID] = nodeResult_t(color, scalar);
		}
};

class diffNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f1, f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			f1 = 1.f - f2;
			
			cin1.R = f1*cin1.R + f2*std::fabs(cin1.R - cin2.R);
			cin1.G = f1*cin1.G + f2*std::fabs(cin1.G - cin2.G);
			cin1.B = f1*cin1.B + f2*std::fabs(cin1.B - cin2.B);
			cin1.A = f1*cin1.A + f2*std::fabs(cin1.A - cin2.A);
			fin1   = f1*fin1 + f2*std::fabs(fin1 - fin2);
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class darkNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			
			cin2 *= f2;
			if(cin2.R < cin1.R) cin1.R = cin2.R;
			if(cin2.G < cin1.G) cin1.G = cin2.G;
			if(cin2.B < cin1.B) cin1.B = cin2.B;
			if(cin2.A < cin1.A) cin1.A = cin2.A;
			fin2 *= f2;
			if(fin2 < fin1) fin1 = fin2;
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class lightNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			
			cin2 *= f2;
			if(cin2.R > cin1.R) cin1.R = cin2.R;
			if(cin2.G > cin1.G) cin1.G = cin2.G;
			if(cin2.B > cin1.B) cin1.B = cin2.B;
			if(cin2.A > cin1.A) cin1.A = cin2.A;
			fin2 *= f2;
			if(fin2 > fin1) fin1 = fin2;
			stack[this->ID] = nodeResult_t(cin1, fin1);
		}
};

class overlayNode_t: public mixNode_t
{
	public:
		virtual void eval(nodeStack_t &stack, const renderState_t &state, const surfacePoint_t &sp)const
		{
			float f1, f2, fin1, fin2;
			colorA_t cin1, cin2;
			getInputs(stack, cin1, cin2, fin1, fin2, f2);
			f1 = 1.f - f2;
			
			colorA_t color;
			color.R = (cin1.R < 0.5f) ? cin1.R * (f1 + 2.0f*f2*cin2.R) : 1.0 - (f1 + 2.0f*f2*(1.0 - cin2.R)) * (1.0 - cin1.R);
			color.G = (cin1.G < 0.5f) ? cin1.G * (f1 + 2.0f*f2*cin2.G) : 1.0 - (f1 + 2.0f*f2*(1.0 - cin2.G)) * (1.0 - cin1.G);
			color.B = (cin1.B < 0.5f) ? cin1.B * (f1 + 2.0f*f2*cin2.B) : 1.0 - (f1 + 2.0f*f2*(1.0 - cin2.B)) * (1.0 - cin1.B);
			color.A = (cin1.A < 0.5f) ? cin1.A * (f1 + 2.0f*f2*cin2.A) : 1.0 - (f1 + 2.0f*f2*(1.0 - cin2.A)) * (1.0 - cin1.A);
			CFLOAT scalar = (fin1 < 0.5f) ? fin1 * (f1 + 2.0f*f2*fin2) : 1.0 - (f1 + 2.0f*f2*(1.0 - fin2)) * (1.0 - fin1);
			stack[this->ID] = nodeResult_t(color, scalar);
		}
};


shaderNode_t* mixNode_t::factory(const paraMap_t &params,renderEnvironment_t &render)
{
	float val=0.5f;
	int mode=0;
	params.getParam("cfactor", val);
	params.getParam("mode", mode);
	
	switch(mode)
	{
		case MN_MIX: 		return new mixNode_t(val);
		case MN_ADD: 		return new addNode_t();
		case MN_MULT: 		return new multNode_t();
		case MN_SUB: 		return new subNode_t();
		case MN_SCREEN:		return new screenNode_t();
		case MN_DIFF:		return new diffNode_t();
		case MN_DARK:		return new darkNode_t();
		case MN_LIGHT:		return new lightNode_t();
		case MN_OVERLAY:	return new overlayNode_t();
	}
	return new mixNode_t(val);
}

// ==================

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("texture_mapper", textureMapper_t::factory);
		render.registerFactory("value", valueNode_t::factory);
		render.registerFactory("mix", mixNode_t::factory);
		render.registerFactory("layer", layerNode_t::factory);
		render.registerFactory("vcolor", vcolorNode_t::factory);
	}
}

__END_YAFRAY
