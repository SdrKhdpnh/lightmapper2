#ifndef Y_BLENDMAT_H
#define Y_BLENDMAT_H

#include <yafray_config.h>
#include <yafraycore/nodematerial.h>
#include <core_api/shader.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

/*! A material that blends the properties of two materials
	note: if both materials have specular reflection or specular transmission
	components, recursive raytracing will not work properly!
	Sampling will still work, but possibly be inefficient
*/

class blendMat_t: public nodeMaterial_t
{
	public:
		blendMat_t(const material_t *m1, const material_t *m2, CFLOAT blendv);
		virtual ~blendMat_t();
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual bool isTransparent() const;
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual color_t volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp1, const surfacePoint_t &sp2) const;
		virtual color_t emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
								bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const;
		
		static material_t* factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &render);
	protected:
		const material_t *mat1, *mat2;
		shaderNode_t *blendS; //!< the shader node used for blending the materials
		CFLOAT blendVal;
		BSDF_t cFlags[2];
		float mVal[4];
		size_t mmem1, mmem2;
};


__END_YAFRAY

#endif // Y_BLENDMAT_H
