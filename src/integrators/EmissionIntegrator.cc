#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <vector>
#include <cmath>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT EmissionIntegrator : public volumeIntegrator_t {
	public:
	EmissionIntegrator() {}

	// optical thickness, absorption, attenuation, extinction
	virtual colorA_t transmittance(renderState_t &state, ray_t &ray) const {
		colorA_t result(1.f);
		
		bool hit = ray.tmax > 0.f;
		
		std::vector<VolumeRegion*> listVR = scene->getVolumes();
		
		//std::cout << "ray.tmax: " << ray.tmax << std::endl;
		
		for (unsigned int i = 0; i < listVR.size(); i++) {
			//std::cout << "using vr" << std::endl;
			/*
			if (hit)
				listVR.at(i)->setMaxT(ray.tmax);
			else
				listVR.at(i)->setMaxT(1e10);
			*/
			result *= listVR.at(i)->tau(ray, 0, 0);
		}
		
		//std::cout << "res: " << result << std::endl;
		
		result = colorA_t(exp(-result.getR()), exp(-result.getG()), exp(-result.getB()));
		
		return result;
	}
	
	// emission part
	virtual colorA_t integrate(renderState_t &state, ray_t &ray) const {
		float stepSize;
		float t0, t1;
		int N = 10; // samples + 1 on the ray inside the volume
		
		colorA_t result(0.f);
		//return result;
		
		bool hit = ray.tmax > 0.f;
		
		std::vector<VolumeRegion*> listVR = scene->getVolumes();
				
		for (unsigned int i = 0; i < listVR.size(); i++) {
			//std::cout << "using vr" << std::endl;
			VolumeRegion* vr = listVR.at(i);
			
			if (!vr->intersect(ray, t0, t1)) continue;
			
			if (hit && ray.tmax < t0) continue;
			
			if (hit && ray.tmax < t1) {
				//vr->setMaxT(ray.tmax);
				t1 = ray.tmax;
			}
			/*
			else
				vr->setMaxT(1e10);
			*/
			
			//int N = (t1-t0) / stepSize
			float step = (t1 - t0) / (float)N; // length between two sample points
			--N;
			float pos = t0 + 0.5 * step;
			point3d_t p(ray.from);
			color_t Tr(1.f);
			
			for (int i = 0; i < N; ++i) {
				ray_t stepRay(ray.from + (ray.dir * pos), ray.dir, 0, step, 0);
				color_t stepTau = vr->tau(stepRay, 0, 0);
				Tr *= colorA_t(exp(-stepTau.getR()), exp(-stepTau.getG()), exp(-stepTau.getB()));
				result += Tr * vr->emission(stepRay.from, stepRay.dir);
				pos += step;
			}
			
			result *= step;
			
			/* // likely correct result
			float dist = t1 - t0;
			color_t Tr(1.f);
			color_t stepTau = vr->tau(ray, 0, 0);
			Tr *= colorA_t(exp(-stepTau.getR()), exp(-stepTau.getG()), exp(-stepTau.getB()));
			result += Tr * .01f * dist; // * vr->emission(ray.from, ray.dir) * dist;
			*/
			
		}
		
		return result;
	}
	
	static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render)
	{
		EmissionIntegrator* inte = new EmissionIntegrator();
		return inte;
	}

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render) {
		render.registerFactory("EmissionIntegrator", EmissionIntegrator::factory);
	}

}

__END_YAFRAY
