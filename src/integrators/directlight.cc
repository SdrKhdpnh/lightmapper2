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
#include <yafraycore/spectrum.h>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT directLighting_t: public tiledIntegrator_t
{
	public:
		directLighting_t(bool transpShad=false, int shadowDepth=4, int rayDepth=6);
		virtual bool preprocess();
		virtual colorA_t integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const;
		static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		color_t sampleAO(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const;
		background_t *background;
		bool trShad, caustics, do_AO;
		int sDepth, rDepth, cDepth;
		int nPhotons, nSearch, AO_samples;
		PFLOAT cRadius, AO_dist;
		color_t AO_col;
		std::vector<light_t*> lights;
		photonMap_t causticMap;
        bool monochrome;
};

directLighting_t::directLighting_t(bool transpShad, int shadowDepth, int rayDepth):
	trShad(transpShad), caustics(false), sDepth(shadowDepth), rDepth(rayDepth)
{
	type = SURFACE;
	// temporary defaults
	cRadius = 0.25;
	cDepth = 10;
	nPhotons = 100000;
	nSearch = 100;
}

bool directLighting_t::preprocess()
{
	background = scene->getBackground();
	lights.clear();
    lights = scene->getCurrentLightLayer();
	if(background)
	{
		light_t *bgl = background->getLight();
		if(bgl) lights.push_back(bgl);
	}
	//currently background lights cannot emit photons yet...so we create map here:
	if(caustics)
	{
		bool success = createCausticMap(*scene, lights, causticMap, cDepth, nPhotons);
	}
	return true;
}

colorA_t directLighting_t::integrate(renderState_t &state, diffRay_t &ray/*, sampler_t &sam*/) const
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
		// if camera ray:
		if(state.raylevel == 0)
		{
			state.includeLights = true;
			//...
		}
		//else state.includeLights = false;
		
		Halton hal3(3);
		unsigned char userdata[USER_DATA_SIZE+7];
		userdata[0] = 0;
		state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
//		std::cout << "directLighting: udat: " << state.userdata << "\n";
		BSDF_t bsdfs;
		//bool shadowed;
		//unsigned int l_offs = 0;
		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);
		vector3d_t wo = -ray.dir;
		ray_t lightRay;
		lightRay.from = sp.P;
		// contribution of light emitting surfaces
		col += material->emit(state, sp, wo);
		if(bsdfs & (BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE))
		{
			col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
		}
		if(bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY))
		{
			col += estimatePhotons(state, sp, causticMap, wo, nSearch, cRadius);
		}
		if( (bsdfs & BSDF_DIFFUSE) && do_AO) col += sampleAO(state, sp, wo);
		
		++state.raylevel;
		if(state.raylevel <= rDepth)
		{
			// dispersive effects with recursive raytracing:
			if( (bsdfs & BSDF_DISPERSIVE) && state.chromatic )
			{
				state.includeLights = true; //debatable...
				int dsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) dsam = std::max(1, dsam/oldDivision);
				state.rayDivision *= dsam;
				int branch = state.rayDivision*oldOffset;
				//int offs = gsam * state.pixelSample + state.samplingOffs;
				float d_1 = 1.f/(float)dsam;
				float ss1 = RI_S(state.pixelSample + state.samplingOffs);
				color_t dcol(0.f);
				vector3d_t wi;
				for(int ns=0; ns<dsam; ++ns)
				{
					state.wavelength = (ns + ss1)*d_1;
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					if(oldDivision > 1)	state.wavelength = addMod1(state.wavelength, old_dc1);
					state.rayOffset = branch;
					++branch;
					sample_t s(0.5f, 0.5f, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_DISPERSIVE);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1.0e-6f && (s.sampledFlags & BSDF_DISPERSIVE))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						color_t wl_col;
						wl2rgb(state.wavelength, wl_col);
						state.chromatic = false;
						diffRay_t refRay(sp.P, wi, 0.0005);
						dcol += (color_t)integrate(state, refRay) * mcol * wl_col;
						state.chromatic = true;
					}
				}
				col += dcol * d_1;
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			// glossy reflection with recursive raytracing:
			if( bsdfs & BSDF_GLOSSY )
			{
				state.includeLights = false;
				int gsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) gsam = std::max(1, gsam/oldDivision);
				state.rayDivision *= gsam;
				int branch = state.rayDivision*oldOffset;
				int offs = gsam * state.pixelSample + state.samplingOffs;
				float d_1 = 1.f/(float)gsam;
				color_t gcol(0.f);
				vector3d_t wi;
				for(int ns=0; ns<gsam; ++ns)
				{
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					state.rayOffset = branch;
					++branch;
					float s1 = RI_vdC(offs + ns);
					float s2 = scrHalton(2, offs + ns);
					if(oldDivision > 1) // create generalized halton sequence
					{
						s1 = addMod1(s1, old_dc1);
						s2 = addMod1(s2, old_dc2);
					}
					sample_t s(s1, s2, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_GLOSSY);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1.0e-6f && (s.sampledFlags & BSDF_GLOSSY))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						diffRay_t refRay(sp.P, wi, 0.0005);
						gcol += (color_t)integrate(state, refRay) * mcol;
					}
				}
				col += gcol * d_1;
				//restore renderstate
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			
			//...perfect specular reflection/refraction with recursive raytracing...
			
			bool reflect=false, refract=false;
			state.includeLights = true;
			vector3d_t dir[2];
			color_t rcol[2], vcol;
			material->getSpecular(state, sp, wo, reflect, refract, &dir[0], &rcol[0]);
			if(reflect)
			{
				diffRay_t refRay(sp.P, dir[0], 0.0005);
				color_t integ = color_t(integrate(state, refRay) );
				integ *= scene->volIntegrator->transmittance(state, refRay); // T
				integ += scene->volIntegrator->integrate(state, refRay); // L_v
				// account for volumetric effects:
				if(bsdfs&BSDF_VOLUMETRIC && material->volumeTransmittance(state, sp, refRay, vcol))
				{	integ *= vcol;	}
				col += color_t(integ) * rcol[0];
			}
			if(refract)
			{
				diffRay_t refRay(sp.P, dir[1], 0.0005);
				colorA_t integ = integrate(state, refRay);
				// account for volumetric effects:
				if(bsdfs&BSDF_VOLUMETRIC && material->volumeTransmittance(state, sp, refRay, vcol))
				{	integ *= vcol;	}
				col += color_t(integ) * rcol[1];
				alpha = integ.A;
			}
		}
		--state.raylevel;
		/* // account for volumetric effects:
		color_t vcol;
		if(material->volumeTransmittance(state, sp, ray, vcol))
		{
			col *= vcol;
		} */
		// else <use "global" volume integrator, if existant (and raylevel>1?)
		
		CFLOAT m_alpha = material->getAlpha(state, sp, wo);
		alpha = m_alpha + (1.f-m_alpha)*alpha;
	}
	else //nothing hit, return background
	{
		if(background)
		{
			col += (*background)(ray, state, false);
		}
	}
	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;

    if (monochrome && (col.R > 0.0f || col.G > 0.0f || col.B > 0.0f))
    {
        col.set(1.0f, 1.0f, 1.0f);
    }
	return colorA_t(col, alpha);
}

color_t directLighting_t::sampleAO(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const
{
	color_t col(0.f);
	bool shadowed;
	const material_t *material = sp.material;
	Halton hal3(3);
	ray_t lightRay;
	lightRay.from = sp.P;
	
	int n = AO_samples;
	if(state.rayDivision > 1) n = std::max(1, n/state.rayDivision);
	unsigned int offs = n * state.pixelSample + state.samplingOffs;
	hal3.setStart(offs-1);
	
	for(int i=0; i<n; ++i)
	{
		float s1 = RI_vdC(offs+i);
		float s2 = hal3.getNext();
		if(state.rayDivision > 1)
		{
			s1 = addMod1(s1, state.dc1);
			s2 = addMod1(s2, state.dc2);
		}
		lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is still bad...
		lightRay.tmax = AO_dist;
		
		sample_t s(s1, s2, /* BSDF_GLOSSY |  */ BSDF_DIFFUSE | BSDF_REFLECT );
		color_t surfCol = material->sample(state, sp, wo, lightRay.dir, s);
		if(s.pdf > 1.0e-6f)
		{
			shadowed = /* (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : */ scene->isShadowed(state, lightRay);
			if(!shadowed)
			{
				CFLOAT cos = std::fabs(sp.N*lightRay.dir);
				col += AO_col * surfCol * cos / s.pdf;
			}
		}
	}
	return col * ( (CFLOAT)1.0 / (CFLOAT)n );
}

integrator_t* directLighting_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	bool transpShad=false;
	bool caustics=false;
	bool do_AO=false;
	int shadowDepth=5;
	int raydepth=5, cDepth=10;
	int search=100, photons=500000;
	int AO_samples = 32;
	double cRad = 0.25;
	double AO_dist = 1.0;
	color_t AO_col(1.f);
    bool monochrome=false;
	
	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("caustics", caustics);
	params.getParam("photons", photons);
	params.getParam("caustic_mix", search);
	params.getParam("caustic_depth", cDepth);
	params.getParam("caustic_radius", cRad);
	params.getParam("do_AO", do_AO);
	params.getParam("AO_samples", AO_samples);
	params.getParam("AO_distance", AO_dist);
	params.getParam("AO_color", AO_col);
    params.getParam("monochrome", monochrome);
	
	directLighting_t *inte = new directLighting_t(transpShad, shadowDepth, raydepth);
	// caustic settings
	inte->caustics = caustics;
	inte->nPhotons = photons;
	inte->nSearch = search;
	inte->cDepth = cDepth;
	inte->cRadius = cRad;
	// AO settings
	inte->do_AO = do_AO;
	inte->AO_samples = AO_samples;
	inte->AO_dist = (PFLOAT)AO_dist;
	inte->AO_col = AO_col;
    inte->monochrome = monochrome;
	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("directlighting",directLighting_t::factory);
	}

}

__END_YAFRAY
