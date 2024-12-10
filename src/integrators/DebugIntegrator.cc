/****************************************************************************
 * 			directlight.cc: an integrator for direct lighting only
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

//#include <mcqmc.h>
#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <yafraycore/tiledintegrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>

__BEGIN_YAFRAY

enum SurfaceProperties {N = 1, dPdU = 2, dPdV = 3, NU = 4, NV = 5};

class YAFRAYPLUGIN_EXPORT DebugIntegrator : public tiledIntegrator_t
{
	public:
		DebugIntegrator(SurfaceProperties dt);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		std::vector<light_t*> lights;
		SurfaceProperties debugType;
};

DebugIntegrator::DebugIntegrator(SurfaceProperties dt)
{
	type = SURFACE;
	debugType = dt;
}

bool DebugIntegrator::preprocess()
{
	return true;
}

colorA_t DebugIntegrator::integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const
{
	color_t col(0.0);
	CFLOAT alpha=0.0;
	surfacePoint_t sp;
	void *o_udat = state.userdata;
	bool oldIncludeLights = state.includeLights;
	static int dbg=0;
//	std::cout << "directLighting::integrate()\n";
	//shoot ray into scene
	if(scene->intersect(ray, sp))
	{
		if (debugType == N)
			col = color_t((sp.N.x + 1.f) * .5f, (sp.N.y + 1.f) * .5f, (sp.N.z + 1.f) * .5f);
		else if (debugType == dPdU)
			col = color_t((sp.dPdU.x + 1.f) * .5f, (sp.dPdU.y + 1.f) * .5f, (sp.dPdU.z + 1.f) * .5f);
		else if (debugType == dPdV)
			col = color_t((sp.dPdV.x + 1.f) * .5f, (sp.dPdV.y + 1.f) * .5f, (sp.dPdV.z + 1.f) * .5f);
		else if (debugType == NU)
			col = color_t((sp.NU.x + 1.f) * .5f, (sp.NU.y + 1.f) * .5f, (sp.NU.z + 1.f) * .5f);
		else if (debugType == NV)
			col = color_t((sp.NV.x + 1.f) * .5f, (sp.NV.y + 1.f) * .5f, (sp.NV.z + 1.f) * .5f);
	}
	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;
	return colorA_t(col, alpha);
}

integrator_t* DebugIntegrator::factory(paraMap_t &params, renderEnvironment_t &render)
{
	int dt = 1;
	params.getParam("debugType", dt);
	std::cout << "debugType " << dt << std::endl;
	DebugIntegrator *inte = new DebugIntegrator((SurfaceProperties)dt);

	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("DebugIntegrator",DebugIntegrator::factory);
	}

}

__END_YAFRAY
