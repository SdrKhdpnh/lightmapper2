#include <yafray_config.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
//#include <textures/noise.h>
//#include <textures/basictex.h>
#include <utilities/mcqmc.h>


#include <cmath>

__BEGIN_YAFRAY

class renderState_t;
class pSample_t;

class NoiseVolume : public DensityVolume {
	public:
	
		NoiseVolume(color_t sa, color_t ss, color_t le, float gg, point3d_t pmin, point3d_t pmax, int attgridScale, texture_t* noise) :
			DensityVolume(sa, ss, le, gg, pmin, pmax, attgridScale) {
			texDistNoise = noise;
			std::cout << "NoiseVolume vol: " << s_a << " " << s_s << " " << l_e << " " << attgridScale << std::endl;
		}
		
		virtual float Density(point3d_t p);
				
		static VolumeRegion* factory(paraMap_t &params, renderEnvironment_t &render);
	
	protected:

		texture_t* texDistNoise;
};

float NoiseVolume::Density(const point3d_t p) {
	//return 1.f;

	float d = texDistNoise->getColor(p  * 0.1f ).energy();
	float cloudCover = .5f; 
	float cloudSharpness = .4f;
	d -= cloudCover;

	if (d < 0.f) return 0.f;
	/*
	point3d_t pNormed = p - bBox.a;
	pNormed.x /= bBox.longX() * 0.5f;
	pNormed.y /= bBox.longY() * 0.5f;
	pNormed.z /= bBox.longZ() * 0.5f;
	pNormed.x -= 1.f;
	pNormed.y -= 1.f;
	pNormed.z -= 1.f;

	float l = pNormed.length();
*/
	//d = 1.0f - powf(cloudSharpness, d);
	//d *= 15.f;
/*
	if (l > 0.8f) {
		d *= (1.f - l) * 5.f;
	}
*/	
	if (d < 0.f) return 0.f;
	
	return d;
}

VolumeRegion* NoiseVolume::factory(paraMap_t &params,renderEnvironment_t &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int attSc = 1;
	
	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	params.getParam("attgridScale", attSc);
	
	texture_t* noise = render.getTexture("TEmytex");
	
	if (noise == 0) std::cout << "mytex not found" << std::endl;
	
	NoiseVolume *vol = new NoiseVolume(color_t(sa), color_t(ss), color_t(le), g,
						point3d_t(min[0], min[1], min[2]), point3d_t(max[0], max[1], max[2]), attSc, noise);
	return vol;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("NoiseVolume", NoiseVolume::factory);
	}
}

__END_YAFRAY
