#ifndef Y_QTAPI_H
#define Y_QTAPI_H

#ifdef BUILDING_QTPLUGIN
	#define YAF_QT_EXPORT YF_EXPORT
#else
	#define YAF_QT_EXPORT YF_IMPORT
#endif

#include <interface/yafrayinterface.h>
#include <string>

namespace yafaray
{
	class yafrayInterface_t;
}

extern "C"
{
	YAF_QT_EXPORT void initGui();
	YAF_QT_EXPORT int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX = 0, int bStartY = 0, const char* fileName = "");
}

#endif // Y_QTAPI_H

