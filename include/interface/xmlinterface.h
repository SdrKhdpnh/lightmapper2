
#ifndef Y_XMLINTERFACE_H
#define Y_XMLINTERFACE_H

#include <interface/yafrayinterface.h>
#include <map>
#include <iostream>
#include <fstream>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT xmlInterface_t: public yafrayInterface_t
{
	public:
		xmlInterface_t();
		// directly related to scene_t:
		virtual void loadPlugins(const char *path);
		virtual bool startGeometry();
		virtual bool endGeometry();
		virtual bool startTriMesh(unsigned int &id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual bool startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV=false, int type=0);
		virtual bool endTriMesh();
		virtual int  addVertex(double x, double y, double z);
		virtual int  addVertex(double x, double y, double z, double ox, double oy, double oz);
		virtual bool addTriangle(int a, int b, int c, const material_t *mat);
		virtual bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat);
		virtual int  addUV(float u, float v);
		virtual bool startVmap(int id, int type, int dimensions);
		virtual bool endVmap();
		virtual bool addVmapValues(float *val);
		virtual bool smoothMesh(unsigned int id, double angle);
		
		// functions directly related to renderEnvironment_t
		virtual light_t* 		createLight		(const char* name);
		virtual texture_t* 		createTexture	(const char* name);
		virtual material_t* 	createMaterial	(const char* name);
		virtual camera_t* 		createCamera	(const char* name);
		virtual background_t* 	createBackground(const char* name);
		virtual integrator_t* 	createIntegrator(const char* name);
		virtual unsigned int 	createObject	(const char* name);
		virtual void clearAll(); //!< clear the whole environment + scene, i.e. free (hopefully) all memory.
		virtual void render(colorOutput_t &output); //!< render the scene...
		virtual bool startScene(int type=0); //!< start a new scene; Must be called before any of the scene_t related callbacks!
		
		virtual void setOutfile(const char *fname);
	protected:
		void writeParamMap(const paraMap_t &pmap, int indent=1);
		void writeParamList(int indent);
		
		std::map<const material_t *, std::string> materials;
		std::ofstream xmlFile;
		std::string xmlName;
		const material_t *last_mat;
		size_t nmat;
		int n_uvs;
		unsigned int nextObj;
};

typedef xmlInterface_t * xmlInterfaceConstructor();

__END_YAFRAY

#endif // Y_XMLINTERFACE_H
