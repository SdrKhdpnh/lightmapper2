
#ifndef Y_MESHTYPES_H
#define Y_MESHTYPES_H

#include <core_api/object3d.h>
#include <yafraycore/triangle.h>
#include <yafraycore/vmap.h>


__BEGIN_YAFRAY


struct uv_t
{
	uv_t(GFLOAT _u = 0.0f, GFLOAT _v = 0.0f ): u(_u), v(_v) {};
	GFLOAT u, v;
};

class triangle_t;
class vTriangle_t;

/*!	meshObject_t holds various polygonal primitives
*/

class YAFRAYCORE_EXPORT meshObject_t: public object3d_t
{
	friend class vTriangle_t;
	friend class bsTriangle_t;
	friend class scene_t;
	public:
		meshObject_t(int ntris, bool hasUV=false, bool hasOrco=false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		int numPrimitives() const { return triangles.size() + s_triangles.size(); }
		int getPrimitives(const primitive_t **prims) const;
		
		primitive_t* addTriangle(const vTriangle_t &t);
		primitive_t* addBsTriangle(const bsTriangle_t &t);
		
		void setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n);
		void setLight(const light_t *l){ light=l; }
		void finish();
	protected:
		std::vector<vTriangle_t> triangles;
		std::vector<bsTriangle_t> s_triangles;
		std::vector<point3d_t>::iterator points;
		std::vector<normal_t>::iterator normals;
		std::vector<int> uv_offsets;
		std::vector<uv_t> uv_values;
		std::map<int, vmap_t> vmaps;
		bool has_orco;
		bool has_uv;
		bool has_vcol;
		bool is_smooth;
		const light_t *light;
		const matrix4x4_t world2obj; //!< transformation from world to object coordinates
};

/*!	This is a special version of meshObject_t!
	The only difference is that it returns a triangle_t instead of vTriangle_t,
	see declaration if triangle_t for more details!
*/

class YAFRAYCORE_EXPORT triangleObject_t: public object3d_t
{
	friend class triangle_t;
	friend class scene_t;
	public:
		triangleObject_t(int ntris, bool hasUV=false, bool hasOrco=false);
		/*! the number of primitives the object holds. Primitive is an element
			that by definition can perform ray-triangle intersection */
		virtual int numPrimitives() const { return triangles.size(); }
		/*! cannot return primitive_t...yet */
		virtual int getPrimitives(const primitive_t **prims) const{ return 0; }
		int getPrimitives(const triangle_t **prims);
		virtual int evalVMap(const surfacePoint_t &sp, unsigned int ID, float *val) const;
		
		triangle_t* addTriangle(const triangle_t &t);
		
		void setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n);
		void finish();

        // EclipseRay: Access our list of uv values
        inline std::vector<uv_t>& getUVValues(){ return uv_values; }

        // EclipseRay: Direct access to our triangle list
        inline std::vector<triangle_t>& getTriangles(){ return triangles; }

        // EclipseRay: Direct access to the UV offset array
        inline std::vector<int>& getUVOffsets(){ return uv_offsets; }

        // EclipseRay: Direct access to our point iterator
        inline std::vector<point3d_t>::const_iterator getPointsIterator(){ return points; }

        // EclipseRay: Tells if we have uv coordinates
        inline bool hasUVCoord(){ return has_uv; }

	protected:
		std::vector<triangle_t> triangles;
		std::vector<point3d_t>::iterator points;
		std::vector<normal_t>::iterator normals;
		std::vector<int> uv_offsets;
		std::vector<uv_t> uv_values;
		std::map<int, vmap_t> vmaps;
		bool has_orco;
		bool has_uv;
		bool has_vcol;
		bool is_smooth;
		const matrix4x4_t world2obj; //!< transformation from world to object coordinates
};

#include <yafraycore/triangle_inline.h>

__END_YAFRAY


#endif //Y_MESHTYPES_H

