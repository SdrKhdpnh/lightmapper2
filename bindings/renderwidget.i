%module yafqt

%include "cpointer.i"
%pointer_functions(float, floatp);
%pointer_functions(int, intp);
%pointer_functions(unsigned int, uintp);

%include "carrays.i"
%array_functions(float, floatArray);


%{
#include <yafray_constants.h>
#include <src/gui/yafqtapi.h>
%}

void initGui();
int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX = 0, int bStartY = 0, const char * fileName = "");


