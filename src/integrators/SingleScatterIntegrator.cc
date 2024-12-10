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
#include <stack>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT SingleScatterIntegrator : public volumeIntegrator_t {
	private:
		bool adaptive;
		bool optimize;

	public:
	SingleScatterIntegrator(float sSize, bool adapt, bool opt) {	
		adaptive = adapt;
		stepSize = sSize;
		optimize = opt;
		std::cout << "scatterint, ss: " << stepSize << " adaptive: " << adaptive << " optimize: " << optimize << std::endl;
	}

	virtual bool preprocess() {
		std::cout << "Preprocessing SingleScatterIntegrator" << std::endl;
		
		if (optimize) {
			std::vector<VolumeRegion*> listVR = scene->getVolumes();
			for (unsigned int i = 0; i < listVR.size(); i++) {
			//std::cout << "using vr" << std::endl;
				VolumeRegion* vr = listVR.at(i);
				bound_t bb = vr->getBB();

				int xSize = vr->attGridX;
				int ySize = vr->attGridY;
				int zSize = vr->attGridZ;

				float xSizeInv = 1.f/(float)xSize;
				float ySizeInv = 1.f/(float)ySize;
				float zSizeInv = 1.f/(float)zSize;

				std::cout << "volume, attGridMaps with size: " << xSize << " " << ySize << " " << xSize << std::endl;
                std::vector<light_t*>& sceneLights = scene->getCurrentLightLayer();
			
				for(std::vector<light_t *>::const_iterator l=sceneLights.begin(); l!=sceneLights.end(); ++l) {
					color_t lcol(0.0);

					float* attenuationGrid = (float*)malloc(xSize * ySize * zSize * sizeof(float));
					vr->attenuationGridMap[(*l)] = attenuationGrid;

					for (int z = 0; z < zSize; ++z) {
						for (int y = 0; y < ySize; ++y) {
							for (int x = 0; x < xSize; ++x) {
								std::cout << "volume " << x << " " << y << " " << x << std::endl;
								// generate the world position inside the grid
								point3d_t p(bb.longX() * xSizeInv * x + bb.a.x,
											bb.longY() * ySizeInv * y + bb.a.y,
											bb.longZ() * zSizeInv * z + bb.a.z);

								surfacePoint_t sp;
								sp.P = p;
								
								ray_t lightRay;

								lightRay.from = sp.P;

								// handle lights with delta distribution, e.g. point and directional lights
								if( (*l)->diracLight() ) {
									bool ill = (*l)->illuminate(sp, lcol, lightRay);
									lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.

									// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
									//color_t lightstepTau = vr->tau(lightRay, stepSize, 0.5f /* (*state.prng)() */);

									color_t lightstepTau(0.f);
									if (ill) {
										for (unsigned int j = 0; j < listVR.size(); j++) {
											VolumeRegion* vr2 = listVR.at(j);
											//float t0Tmp = -1, t1Tmp = -1;
											//if (vr2->intersect(lightRay, t0Tmp, t1Tmp)) {
											std::cout << "tau " << lightRay.dir << " " << lightRay.from << " ill: " << ill << std::endl;
											lightstepTau += vr2->tau(lightRay, stepSize, 0.0f);
											//}
										}
									}

									float lightTr = exp(-lightstepTau.energy());
									attenuationGrid[x + y * xSize + ySize * xSize * z] = lightTr;

									//std::cout << "gridpos: " << (x + y * xSize + ySize * xSize * z) << " " << lightstepTau.energy() << " " << lightRay.dir << " " << lightRay.tmax << " lightTr: " << lightTr << std::endl;

								}
								else // area light and suchlike
								{
									float lightPdf;
									float lightTr = 0;
									int n = (int)((*l)->nSamples() / 2) + 1;
									lSample_t ls;
									for(int i=0; i<n; ++i)
									{
										ls.s1 = 0.5f; //(*state.prng)();
										ls.s2 = 0.5f; //(*state.prng)();

										(*l)->illumSample(sp, ls, lightRay);
										lightRay.tmin = 0.0005;

										// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
										//color_t lightstepTau = vr->tau(lightRay, stepSize, 0.5f);
										color_t lightstepTau(0.f);
										for (unsigned int j = 0; j < listVR.size(); j++) {
											VolumeRegion* vr2 = listVR.at(j);
											//float t0Tmp = -1, t1Tmp = -1;
											//if (vr2->intersect(lightRay, t0Tmp, t1Tmp)) {
												lightstepTau += vr2->tau(lightRay, stepSize, 0.0f);
											//}
										}
										lightTr += exp(-lightstepTau.energy());
									}

									attenuationGrid[x + y * xSize + ySize * xSize * z] = lightTr / (float)n;
								}
							}
						}
					}
				}
			}
		}

		return true;
	}
	

	// optical thickness, absorption, attenuation, extinction
	virtual colorA_t transmittance(renderState_t &state, ray_t &ray) const {
		colorA_t Tr(1.f);
		//return result;
		
		std::vector<VolumeRegion*> listVR = scene->getVolumes();
		
		if (listVR.size() == 0) return Tr;
		
		//std::cout << "ray.tmax: " << ray.tmax << std::endl;

		for (unsigned int i = 0; i < listVR.size(); i++) {
			VolumeRegion* vr = listVR.at(i);
			float t0 = -1, t1 = -1;
			if (vr->intersect(ray, t0, t1)) {
				float random = (*state.prng)();
				color_t opticalThickness = listVR.at(i)->tau(ray, stepSize, random);
				Tr *= colorA_t(exp(-opticalThickness.energy()));
			}
		}
		
		//std::cout << "res: " << result << std::endl;
		
		//result = colorA_t(exp(-result.getR()), exp(-result.getG()), exp(-result.getB()));
		
		return Tr;
	}
	
	// emission and in-scattering
	virtual colorA_t integrate(renderState_t &state, ray_t &ray) const {
		float t0 = 1e10f, t1 = -1e10f;

		colorA_t result(0.f);
		//return result;
				
		std::vector<VolumeRegion*> listVR = scene->getVolumes();
		
		if (listVR.size() == 0) return result;
		
		bool hit = (ray.tmax > 0.f);

		// find min t0 and max t1
		for (unsigned int i = 0; i < listVR.size(); i++) {
			float t0Tmp, t1Tmp;
			VolumeRegion* vr = listVR.at(i);

			if (!vr->intersect(ray, t0Tmp, t1Tmp)) continue;

			if (hit && ray.tmax < t0Tmp) continue;

			if (t0Tmp < 0.f) t0Tmp = 0.f;

			if (hit && ray.tmax < t1Tmp) t1Tmp = ray.tmax;

			if (t1Tmp > t1) t1 = t1Tmp;
			if (t0Tmp < t0) t0 = t0Tmp;
		}

		float step = stepSize; // length between two sample points
		float dist = t1-t0;
		float pos = t0 + (*state.prng)() * step;
		color_t Tr(1.f);
		color_t trTmp(1.f);
		colorA_t resultPrev(0.f);

		float sigmaEnergyPrev = 0.f;

		//std::cout << "singleScat, dist: " << (t1 - t0) << " " << t0 << " " << t1 << std::endl;

		while (pos < t1) {
			colorA_t resultTmp(0.f);
			ray_t stepRay(ray.from + (ray.dir * (pos - step)), ray.dir, 0, step, 0);

			//std::cout << "i: " << i << "/" << N << " stepTau: " << stepRay.from << std::endl;
			// this should take a single sample from tau

			//color_t stepTau = vr->tau(stepRay, step, (*state.prng)());
			// replaced by
			color_t stepTau(0.f);
			for (unsigned int i = 0; i < listVR.size(); i++) {
				VolumeRegion* vr = listVR.at(i);
				float t0Tmp = -1, t1Tmp = -1;
				if (vr->intersect(stepRay, t0Tmp, t1Tmp)) {
					stepTau += vr->tau(stepRay, step, (*state.prng)());
				}
			}
			
			trTmp *= exp(-stepTau.energy());

			if (optimize && trTmp.energy() < 1e-3f) {
				float random = (*state.prng)();
				if (random < 0.5f) break;
				trTmp = trTmp / random;
			}

			//std::cout << "emission: " << stepRay.from << std::endl;
			//resultTmp = trTmp * vr->emission(stepRay.from, stepRay.dir);

			//color_t sigma_s = vr->sigma_s(stepRay.from, stepRay.dir);
			// replaced by
			color_t sigma_s(0.f);
			for (unsigned int i = 0; i < listVR.size(); i++) {
				VolumeRegion* vr = listVR.at(i);
				float t0Tmp = -1, t1Tmp = -1;
				if (listVR.at(i)->intersect(stepRay, t0Tmp, t1Tmp)) {
					sigma_s += vr->sigma_s(stepRay.from, stepRay.dir);
				}
			}
			
			float sigmaEnergy = sigma_s.energy();

			//std::cout << "ss: " << sigma_s << std::endl;

			// with a sigma_s close to 0, no light can be scattered -> computation can be skipped

			if (optimize && sigma_s.energy() < 1e-3) {
				float random = (*state.prng)();
				if (random < 0.5f) {
					//std::cout << "throwing away because ss < 1e-3" << std::endl;
					pos += step;
					continue;
				}
				sigma_s = sigma_s / random;
			}

			//std::cout << "in-scatter by light sources" << std::endl;

			surfacePoint_t sp;
			sp.P = stepRay.from;

			ray_t lightRay;
			lightRay.from = sp.P;
            std::vector<light_t*>& sceneLights = scene->getCurrentLightLayer();

			for(std::vector<light_t *>::const_iterator l=sceneLights.begin(); l!=sceneLights.end(); ++l) {
				color_t lcol(0.0);

					// handle lights with delta distribution, e.g. point and directional lights
					if( (*l)->diracLight() ) {
						if( (*l)->illuminate(sp, lcol, lightRay) )
						{
							// ...shadowed...
							lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
							bool shadowed = scene->isShadowed(state, lightRay);
							if (!shadowed)
							{
								float lightTr = 1;
								// replace lightTr with precalculated attenuation
								if (optimize) {
									//lightTr = vr->attenuation(sp.P, (*l));
									// replaced by
									for (unsigned int i = 0; i < listVR.size(); i++) {
										VolumeRegion* vr = listVR.at(i);
										float t0Tmp = -1, t1Tmp = -1;
										if (vr->intersect(lightRay, t0Tmp, t1Tmp)) {
											//lightTr *= vr->attenuation(sp.P, (*l));
											lightTr = vr->attenuation(sp.P, (*l));
											break;
										}
									}
								}
								else {
									//color_t lightstepTau = vr->tau(lightRay, step * 4.f,(*state.prng)());
									// replaced by
									color_t lightstepTau(0.f);
									for (unsigned int i = 0; i < listVR.size(); i++) {
										VolumeRegion* vr = listVR.at(i);
										float t0Tmp = -1, t1Tmp = -1;
										if (listVR.at(i)->intersect(lightRay, t0Tmp, t1Tmp)) {
											lightstepTau += vr->tau(lightRay, step * 4.f, (*state.prng)());
										}
									}
									// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
									lightTr = exp(-lightstepTau.energy());
								}

								resultTmp += trTmp * lightTr * sigma_s * lcol; // * vr->p(lightRay.dir, -ray.dir);
							}
						}
					}
					else // area light and suchlike
					{
						float lightPdf;
						int n = 1;
						color_t ccol(0.0);
						float lightTr = 0;
						lSample_t ls;
						for(int i=0; i<n; ++i)
						{
							// ...get sample val...
							ls.s1 = (*state.prng)();
							ls.s2 = (*state.prng)();

							if((*l)->illumSample(sp, ls, lightRay))
							{
								// ...shadowed...
								lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
								bool shadowed = scene->isShadowed(state, lightRay);
								if(!shadowed) {
									ccol += ls.col / ls.pdf;
									if (!optimize) {
										//color_t lightstepTau = vr->tau(lightRay, step * 4.f,(*state.prng)());
										// replaced by
										color_t lightstepTau(0.f);
										for (unsigned int i = 0; i < listVR.size(); i++) {
											VolumeRegion* vr = listVR.at(i);
											float t0Tmp = -1, t1Tmp = -1;
											if (listVR.at(i)->intersect(lightRay, t0Tmp, t1Tmp)) {
												lightstepTau += vr->tau(lightRay, step * 4.f, (*state.prng)());
											}
										}
										// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
										lightTr += exp(-lightstepTau.energy());
									}
								}
							}
						}

						// replace lightTr with precalculated attenuation
						if (optimize) {
							//lightTr = vr->attenuation(sp.P, (*l));
							// replaced by
							for (unsigned int i = 0; i < listVR.size(); i++) {
								VolumeRegion* vr = listVR.at(i);
								float t0Tmp = -1, t1Tmp = -1;
								if (vr->intersect(lightRay, t0Tmp, t1Tmp)) {
									//lightTr *= vr->attenuation(sp.P, (*l));
									lightTr = vr->attenuation(sp.P, (*l));
									break;
								}
							}
						}
						else {
							lightTr /= (float)n;
						}

						ccol = ccol /* * vr->p(lightRay.dir, -ray.dir) */ / (float)n;
						resultTmp += trTmp * lightTr * sigma_s * ccol;
					}

			}


			//bool adaptive = true;

			if (adaptive) {

				float epsilon = 0.01f;

				//std::cout << "light now: " << resultTmp.energy() << " light prev: " << resultPrev.energy() << std::endl;
				//std::cout << "N: " << N << " sigmaEnergyPrev: " << sigmaEnergyPrev << " sigmaEnergy: " << sigmaEnergy << std::endl;

				//if ((fabs(resultTmp.energy() - resultPrev.energy()) > epsilon) && (step > .5f)) {
				if ((fabs(sigmaEnergyPrev - sigmaEnergy) > epsilon) && (step > .5f)) {
					pos -= step;
					step /= 2.f;
					//std::cout << "decrease ss: " << step << " back to: " << pos + step << std::endl;
					trTmp = Tr;
				}
				else {
					if (step < stepSize) {
						step *= 2.f;
						//std::cout << "increase ss: " << step << std::endl;
					}
					result += resultTmp;
					//resultPrev = resultTmp;
					Tr *= trTmp;
					sigmaEnergyPrev = sigmaEnergy;
				}

				/*
				if ((fabs(resultTmp.energy() - result.energy()) > epsilon) && (step > .1f) ) {
					pos -= step;
					step /= 2.f;
					resultStack.push(resultTmp);
					trStack.push(trTmp);
					++recurse;
					//std::cout << "recursion: " << recurse << " step: " << step << std::endl;
				}
				else {
					if (recurse == 0) {
						result = resultTmp;
						Tr = trTmp;
					}
					else {
						result = resultStack.top();
						Tr = trStack.top();
						resultStack.pop();
						trStack.pop();
						step *= 2.f;
						--recurse;
					}
				}
				*/

				//std::cout << "stacksize: " << resultStack.size() << std::endl;
			}
			else {
				result += resultTmp * step;
				//Tr *= trTmp;
			}

			pos += step;
		}

		return result;
	}
	
	static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render)
	{
		bool adapt = false;
		bool opt = false;
		float sSize = 1.f;
		params.getParam("stepSize", sSize);
		params.getParam("adaptive", adapt);
		params.getParam("optimize", opt);
		SingleScatterIntegrator* inte = new SingleScatterIntegrator(sSize, adapt, opt);
		return inte;
	}

	float stepSize;

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("SingleScatterIntegrator", SingleScatterIntegrator::factory);
	}

}

__END_YAFRAY
