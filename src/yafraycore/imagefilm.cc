#include <core_api/imagefilm.h>
#include <yafraycore/monitor.h>
#include <utilities/math_utils.h>
//#include <utilities/tiled_array.h>
#include <yafraycore/timer.h>
#include <yaf_revision.h>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <iomanip>

#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "font_frankbook.cc"

__BEGIN_YAFRAY

#define FILTER_TABLE_SIZE 16
#define MAX_FILTER_SIZE 8

#if HAVE_FREETYPE
void imageFilm_t::drawFontBitmap( FT_Bitmap* bitmap, int x, int y)
{
	int i, j, p, q;
	int x_max = x + bitmap->width;
	int y_max = y + bitmap->rows;

	for ( i = x, p = 0; i < x_max; i++, p++ )
	{
		for ( j = y, q = 0; j < y_max; j++, q++ )
		{
			if ( i >= w || j >= h )
				continue;

			pixel_t pix;
			int tmpBuf = bitmap->buffer[q * bitmap->width + p];
			if (tmpBuf > 0) {
				pix = (*image)(i, j);
				float alpha = (float)tmpBuf/255.0;
				pix.col = ((*image)(i, j).col * (1.0 - alpha)) + (pix.weight * alpha * colorA_t(1.0, 1.0, 1.0));
				(*image)(i, j) = pix;
			}
		}
	}
}


void imageFilm_t::drawRenderSettings()
{
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Vector pen; // untransformed origin
	FT_Error error;

	std::stringstream ss;
	ss << std::setprecision(4);
	double times = gTimer.getTime("rendert");
	int timem, timeh;
	gTimer.splitTime(times, &times, &timem, &timeh);
	ss << "render time:";
	if (timeh > 0) ss << " " << timeh << "h";
	if (timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";
//	env->addToParamsString(ss.str().c_str());

	std::string str(env->getParamsString());
	std::string::size_type p = str.find("$REVISION", 0);
	if (p != std::string::npos) {
		std::string revStr(YAF_SVN_REV);
		// 9 == length of "$REVISION"
		str.replace(p, 9, revStr);
		env->clearParamsString();
		env->addToParamsString(str.c_str());
	}

	str = std::string(env->getParamsString());
	p = str.find("$TIME", 0);
	if (p != std::string::npos) {
		// 5 == length of "$TIME"
		str.replace(p, 5, ss.str());
		env->clearParamsString();
		env->addToParamsString(str.c_str());
	}

	const char* text = env->getParamsString();

	int num_chars = strlen( text );

	std::cout << "render settings\n" << text;

	error = FT_Init_FreeType( &library ); // initialize library
	if ( error ) { std::cout << "lib error\n"; return; }

	error = FT_New_Memory_Face( library, (const FT_Byte*)font_ttf, font_ttf_size, 0, &face ); // create face object
	if ( error ) { std::cout << "face error\n"; return; }

	// use 10pt at 100dpi
	float fontsize = 8.5f;
	error = FT_Set_Char_Size( face, int(fontsize * 64), 0, 100, 0 ); // set character size
	if ( error ) { std::cout << "char size error\n"; return; }

	slot = face->glyph;

	// the pen position in 26.6 cartesian space coordinates
	pen.x = 10 * 64;
	pen.y = ( 29 ) * 64;
	//pen.y = ( h - 20 ) * 64;

	// dark bar at the bottom
	for ( int x = 0; x < w; x++ )
	{
		for ( int y = h - 40; y < h; y++ )
		{
			pixel_t pix;
			pix = (*image)(x, y);
			pix.col = pix.col * 0.4 ;
			(*image)(x, y) = pix;
		}
	}

	for ( int n = 0; n < num_chars; n++ )
	{
		// set transformation
		FT_Set_Transform( face, 0, &pen );

		if (text[n] == '\n') {
			pen.x = 10 * 64;
			pen.y -= ( 12 ) * 64;
			continue;
		}

		// load glyph image into the slot (erase previous one)
		error = FT_Load_Char( face, text[n], FT_LOAD_DEFAULT );
		if ( error ) {
			std::cout << "char error: " << text[n] << "\n";
			continue;
		}
		FT_Render_Glyph( slot, FT_RENDER_MODE_LIGHT );

		// now, draw to our target surface (convert position)
		drawFontBitmap( &slot->bitmap, slot->bitmap_left, h - slot->bitmap_top);

		// increment pen position
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}
	FT_Done_Face    ( face );
	FT_Done_FreeType( library );
}
#endif


typedef float filterFunc(float dx, float dy);

float Box(float dx, float dy){ return 1.f; }

float Mitchell(float dx, float dy)
{
	const float B = 0.33333333;
	const float C = 0.33333333;
	float val;
	float x = 2.f * sqrt(dx*dx + dy*dy);
	if(x>2.f) return (0.f);
	float x2 = x*x;
	if(x>1.f)
	{
		val = ((-B - 6 * C) * x * x2 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C)) * 0.1666666;
	}
	else
	{
		val = ((12 - 9 * B - 6 * C) * x * x2 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B)) * 0.1666666;
	}
	return (val);
}

float Gauss(float dx, float dy)
{
	float r2 = dx*dx + dy*dy;
	const float expo = exp(-6.f);
	return std::max(0.f, float(exp(-6 * r2) - expo));
}

imageFilm_t::imageFilm_t (int width, int height, int xstart, int ystart, colorOutput_t &out, float filterSize, filterType filt, renderEnvironment_t *e):
	flags(0), w(width), h(height), cx0(xstart), cy0(ystart), gamma(1.0), filterw(filterSize*0.5), output(&out),
	clamp(false), split(true), interactive(true), abort(false), correctGamma(false), estimateDensity(false), numSamples(0),
	splitter(0), pbar(0), env(e)
{
	cx1 = xstart + width;
	cy1 = ystart + height;
	filterTable = new float[FILTER_TABLE_SIZE * FILTER_TABLE_SIZE];
	// allocate image, the pixels are NOT YET set black! See init()...
	image = new tiledArray2D_t<pixel_t, 3>(width, height, false);
	// fill filter table:
	float *fTp = filterTable;
	float scale = 1.f/(float)FILTER_TABLE_SIZE;
	
	filterFunc *ffunc=0;
	switch(filt)
	{
		case MITCHELL: ffunc = Mitchell; filterw *= 2.6f; break;
		case GAUSS: ffunc = Gauss; filterw *= 2.f; break;
		case BOX:
		default:	ffunc = Box;
	}
	filterw = std::max(0.501, filterw); // filter needs to cover at least the area of one pixel!
	if(filterw > 0.5*MAX_FILTER_SIZE) filterw = 0.5*MAX_FILTER_SIZE;
	for(int y=0; y<FILTER_TABLE_SIZE; ++y)
	{
		for(int x=0; x<FILTER_TABLE_SIZE; ++x)
		{
			*fTp = ffunc((x+.5f)*scale, (y+.5f)*scale);
			++fTp;
		}
	}
	
	tableScale = 0.9999 * FILTER_TABLE_SIZE/filterw;
	area_cnt = 0;
	_n_unlocked = _n_locked = 0;
//	std::cout << "==ctor: cx0 "<<cx0<<", cx1 "<<cx1<<" cy0, "<<cy0<<", cy1 "<<cy1<<"\n";
	pbar = NULL;//new ConsoleProgressBar_t(80);
}

void imageFilm_t::setGamma(float gammaVal, bool enable)
{
	correctGamma = enable;
	if(gammaVal > 0) gamma = 1.f/gammaVal; //gamma correction means applying gamma curve with 1/gamma
}

void imageFilm_t::setDensityEstimation(bool enable)
{
	if(enable) densityImage.resize(w, h, false);
	estimateDensity = enable;
}


void imageFilm_t::init()
{
	unsigned int size = image->size();
	pixel_t *pixels = image->getData();
	std::memset(pixels, 0, size * sizeof(pixel_t));
	//clear density image
	if(estimateDensity)
	{
		std::memset(densityImage.getData(), 0, densityImage.size()*sizeof(color_t));
	}
	//clear custom channels:
	for(unsigned int i=0; i<channels.size(); ++i)
	{
		tiledArray2D_t<float, 3> &chan = channels[i];
		float *cdata = chan.getData();
		std::memset(cdata, 0, chan.size() * sizeof(float));
	}
	
	if(split)
	{
		next_area = 0;
		splitter = new imageSpliter_t(w, h, cx0, cy0, 32);
		area_cnt = splitter->size();
	}
	else area_cnt = 1;
	if(pbar) pbar->init(area_cnt);
	abort = false;
	completed_cnt = 0;
}

// currently the splitter only gives tiles in scanline order...
bool imageFilm_t::nextArea(renderArea_t &a)
{
	if(abort) return false;
	int ifilterw = int(ceil(filterw));
	if(split)
	{
		int n;
		splitterMutex.lock();
		n = next_area++;
		splitterMutex.unlock();
		if(	splitter->getArea(n, a) )
		{
			a.sx0 = a.X + ifilterw;
			a.sx1 = a.X + a.W - ifilterw;
			a.sy0 = a.Y + ifilterw;
			a.sy1 = a.Y + a.H - ifilterw;
			return true;
		}
	}
	else
	{
		if(area_cnt) return false;
		a.X = cx0;
		a.Y = cy0;
		a.W = w;
		a.H = h;
		a.sx0 = a.X + ifilterw;
		a.sx1 = a.X + a.W - ifilterw;
		a.sy0 = a.Y + ifilterw;
		a.sy1 = a.Y + a.H - ifilterw;
		++area_cnt;
		return true;
	}
	return false;
}

// !todo: make output optional, and maybe output surrounding pixels influenced by filter too
void imageFilm_t::finishArea(renderArea_t &a)
{
	outMutex.lock();
	int end_x = a.X+a.W-cx0, end_y = a.Y+a.H-cy0;
	for(int j=a.Y-cy0; j<end_y; ++j)
	{
		for(int i=a.X-cx0; i<end_x; ++i)
		{
			pixel_t &pixel = (*image)(i, j);
			colorA_t col;
			if(pixel.weight>0.f)
			{
				col = pixel.col*(1.f/pixel.weight);
				col.clampRGB0();
			}
			else col = 0.0;
			// !!! assume color output size matches width and height of image film, probably (likely!) stupid!
			if(correctGamma) col.gammaAdjust(gamma);
			float fb[5];
			fb[0] = col.R, fb[1] = col.G, fb[2] = col.B, fb[3] = col.A, fb[4] = 0.f;
			if( !output->putPixel(i, j, fb, 4 ) ) abort=true;
			//if( !output->putPixel(i, j, col, col.getA()) ) abort=true;
		}
	}
	if(interactive) output->flushArea(a.X-cx0, a.Y-cy0, end_x, end_y);
	if(pbar)
	{
		if(++completed_cnt == area_cnt) pbar->done();
		else pbar->update(1);
	}
	outMutex.unlock();
}

/* CAUTION! Implemantation of this function needs to be thread safe for samples that
	contribute to pixels outside the area a AND pixels that might get
	contributions from outside area a! (yes, really!) */
void imageFilm_t::addSample(const colorA_t &c, int x, int y, float dx, float dy, const renderArea_t *a)
{
	colorA_t col=c;
	if(clamp) col.clampRGB01();
//	static int bleh=0;
	int dx0, dx1, dy0, dy1, x0, x1, y0, y1;
	// get filter extent:
	dx0 = Round2Int( (double)dx - filterw);
	dx1 = Round2Int( (double)dx + filterw - 1.0); //uhm why not kill the -1 and make '<=' a '<' in loop ?
	dy0 = Round2Int( (double)dy - filterw);
	dy1 = Round2Int( (double)dy + filterw - 1.0);
	// make sure we don't leave image area:
//	if(bleh<3)
//		std::cout <<"x:"<<x<<" y:"<<y<< "dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	dx0 = std::max(cx0-x, dx0);
	dx1 = std::min(cx1-x-1, dx1);
	dy0 = std::max(cy0-y, dy0);
	dy1 = std::min(cy1-y-1, dy1);
//	if(bleh<3)
//		std::cout << "camp: dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	// get indizes in filter table
	double x_offs = dx - 0.5;
	int xIndex[MAX_FILTER_SIZE+1], yIndex[MAX_FILTER_SIZE+1];
	for (int i=dx0, n=0; i <= dx1; ++i, ++n) {
		double d = fabs( (double(i) - x_offs) * tableScale);
		xIndex[n] = Floor2Int(d);
		if(xIndex[n] > FILTER_TABLE_SIZE-1)
		{
			std::cout << "filter table x error!\n";
			std::cout << "x: "<<x<<" dx: "<<dx<<" dx0: "<<dx0 <<" dx1: "<<dx1<<"\n";
			std::cout << "tableScale: "<<tableScale<<" d: "<<d<<" xIndex: "<<xIndex[n]<<"\n";
			throw std::logic_error("addSample error");
		}
	}
	double y_offs = dy - 0.5;
	for (int i=dy0, n=0; i <= dy1; ++i, ++n) {
		float d = fabsf( (double(i) - y_offs) * tableScale);
		yIndex[n] = Floor2Int(d);
		// yIndex[n] = std::min(Floor2Int(d), FILTER_TABLE_SIZE-1);
		if(yIndex[n] > FILTER_TABLE_SIZE-1)
		{
			std::cout << "filter table y error!\n";
			std::cout << "y: "<<y<<" dy: "<<dy<<" dy0: "<<dy0 <<" dy1: "<<dy1<<"\n";
			std::cout << "tableScale: "<<tableScale<<" d: "<<d<<" yIndex: "<<yIndex[n]<<"\n";
			throw std::logic_error("addSample error");
		}
	}
//	if(bleh<3)
//		std::cout << "double-check: dx0 "<<dx0<<", dx1 "<<dx1<<" dy0, "<<dy0<<", dy1 "<<dy1<<"\n";
	x0 = x+dx0; x1 = x+dx1;
	y0 = y+dy0; y1 = y+dy1;
//	if(bleh<3)
//		std::cout << "x0 "<<x0<<", x1 "<<x1<<", y0 "<<y0<<", y1 "<<y1<<"\n";
	// check if we need to be thread-safe, i.e. add outside safe area (4 ugly conditionals...can't help it):
	bool locked=false;
	if(!a || x0 < a->sx0 || x1 > a->sx1 || y0 < a->sy0 || y1 > a->sy1)
	{
		imageMutex.lock();
		locked=true;
		++_n_locked;
	}
	else ++_n_unlocked;
	for (int j = y0; j <= y1; ++j)
		for (int i = x0; i <= x1; ++i) {
			// get filter value at pixel (x,y)
			int offset = yIndex[j-y0]*FILTER_TABLE_SIZE + xIndex[i-x0];
			float filterWt = filterTable[offset];
			// update pixel values with filtered sample contribution
			pixel_t &pixel = (*image)(i - cx0, j - cy0);
			pixel.col += (col * filterWt);
			pixel.weight += filterWt;
			/*if(i==0 && j==129) std::cout<<"col: "<<col<<" pcol: "<<
			pixel.col<<" pw: "<<pixel.weight<<" x:"<<x<<"y:"<<y<<"\n";*/
		}
	if(locked) imageMutex.unlock();
//	++bleh;
}

void imageFilm_t::addDensitySample(const color_t &c, int x, int y, float dx, float dy, const renderArea_t *a)
{
	if(!estimateDensity) return;
	
	int dx0, dx1, dy0, dy1, x0, x1, y0, y1;
	// get filter extent:
	dx0 = Round2Int( (double)dx - filterw);
	dx1 = Round2Int( (double)dx + filterw - 1.0); //uhm why not kill the -1 and make '<=' a '<' in loop ?
	dy0 = Round2Int( (double)dy - filterw);
	dy1 = Round2Int( (double)dy + filterw - 1.0);
	// make sure we don't leave image area:
	dx0 = std::max(cx0-x, dx0);
	dx1 = std::min(cx1-x-1, dx1);
	dy0 = std::max(cy0-y, dy0);
	dy1 = std::min(cy1-y-1, dy1);
	
	double x_offs = dx - 0.5;
	int xIndex[MAX_FILTER_SIZE+1], yIndex[MAX_FILTER_SIZE+1];
	for (int i=dx0, n=0; i <= dx1; ++i, ++n) {
		double d = fabs( (double(i) - x_offs) * tableScale);
		xIndex[n] = Floor2Int(d);
		if(xIndex[n] > FILTER_TABLE_SIZE-1)
		{
			throw std::logic_error("addSample error");
		}
	}
	double y_offs = dy - 0.5;
	for (int i=dy0, n=0; i <= dy1; ++i, ++n) {
		float d = fabsf( (double(i) - y_offs) * tableScale);
		yIndex[n] = Floor2Int(d);
		if(yIndex[n] > FILTER_TABLE_SIZE-1)
		{
			throw std::logic_error("addSample error");
		}
	}
	x0 = x+dx0; x1 = x+dx1;
	y0 = y+dy0; y1 = y+dy1;
	
	
	densityImageMutex.lock();
	for (int j = y0; j <= y1; ++j)
		for (int i = x0; i <= x1; ++i) {
			// get filter value at pixel (x,y)
			int offset = yIndex[j-y0]*FILTER_TABLE_SIZE + xIndex[i-x0];
			float filterWt = filterTable[offset];
			// update pixel values with filtered sample contribution
			color_t &pixel = densityImage(i - cx0, j - cy0);
			pixel += c * filterWt;
		}
	++numSamples;
	densityImageMutex.unlock();
}

// warning! not really thread-safe currently!
// although this is write-only and overwriting the same pixel makes little sense...
void imageFilm_t::setChanPixel(float val, int chan, int x, int y)
{
	channels[chan](x-cx0, y-cy0) = val;
}

void imageFilm_t::nextPass(bool adaptive_AA)
{
	int n_resample=0;
	
	splitterMutex.lock();
	next_area = 0;
	splitterMutex.unlock();
	if(flags) flags->clear();
	else flags = new tiledBitArray2D_t<3>(w, h, true);
	if(adaptive_AA && AA_thesh>0.f) for(int y=0; y<h-1; ++y)
	{
		for(int x=0; x<w-1; ++x)
		{
			bool needAA=false;
			colorA_t c=(*image)(x, y).normalized();
			if( (c-(*image)(x+1, y).normalized()).abscol2bri() >= AA_thesh )
			{
				needAA=true; flags->setBit(x+1, y);
			}
			if( (c-(*image)(x, y+1).normalized()).abscol2bri() >= AA_thesh )
			{
				needAA=true; flags->setBit(x, y+1);
			}
			if( (c-(*image)(x+1, y+1).normalized()).abscol2bri() >= AA_thesh )
			{
				needAA=true; flags->setBit(x+1, y+1);
			}
			if(needAA)
			{
				flags->setBit(x, y);
				// color all pixels to be resampled:
				float fb[5];
				fb[0] =  fb[1] = fb[2] = fb[3] = 1.f; fb[4] = 0.f;
				if(interactive) output->putPixel(x, y, fb, 4 );
				//if(interactive) output->putPixel(x, y, color_t(1.f), 1.f);
				++n_resample;
			}
		}
	}
	if(interactive) output->flush();
	//std::cout << "imageFilm_t::nextPass: resampling "<<n_resample<<" pixels!\n";
	if(pbar) pbar->init(area_cnt);
	completed_cnt = 0;
}

void imageFilm_t::flush(int flags, colorOutput_t *out)
{
	//std::cout << "flushing imageFilm buffer\n";
	colorOutput_t *colout = out ? out : output;
#if HAVE_FREETYPE
	if (env && env->getDrawParams()) {
		drawRenderSettings();
	}
#else
	if (env && env->getDrawParams()) std::cout << "info: compiled without freetype; overlay feature not available" << std::endl;
#endif
	int n = channels.size();
	float *fb = (float *)alloca( (n+5) * sizeof(float) );
	//if this is a density image (light tracing, metropolis whatever...)
	/* if(estimateDensity)
	{
		float multi = float(w*h)/(float)numSamples;
		for(int j=0; j<h; ++j)
		{
			for(int i=0; i<w; ++i)
			{
				pixel_t &pixel = (*image)(i, j);
				colorA_t col;
				col = pixel.col*multi;
				col.clampRGB0();
				// !!! assume color output size matches width and height of image film, probably (likely!) stupid!
				if(correctGamma) col.gammaAdjust(gamma);
				
				fb[0] = col.R, fb[1] = col.G, fb[2] = col.B, fb[3] = col.A, fb[4] = 0.f;
				for(int k=0; k<n; ++k) fb[k+4] = channels[k](i, j);
				output->putPixel(i, j, fb, 4+n );
			}
		}
	}
	else */
	float multi = float(w*h)/(float)numSamples;
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			pixel_t &pixel = (*image)(i, j);
			colorA_t col;
			if((flags & IF_IMAGE) && pixel.weight>0.f)
			{
				col = pixel.col/pixel.weight;
				col.clampRGB0();
			}
			else col = 0.0;
			if(estimateDensity && (flags & IF_DENSITYIMAGE))
			{
				col += densityImage(i, j) * multi;
				col.clampRGB0();
			}
			// !!! assume color output size matches width and height of image film, probably (likely!) stupid!
			if(correctGamma) col.gammaAdjust(gamma);
			
			fb[0] = col.R, fb[1] = col.G, fb[2] = col.B, fb[3] = col.A, fb[4] = 0.f;
			for(int k=0; k<n; ++k) fb[k+4] = channels[k](i, j);
			colout->putPixel(i, j, fb, 4+n );
			//output->putPixel(i, j, col, col.getA());
		}
	}
	
	colout->flush();
}

bool imageFilm_t::doMoreSamples(int x, int y) const
{
	return (AA_thesh>0.f) ? flags->getBit(x-cx0, y-cy0) : true;
}

int imageFilm_t::addChannel(const std::string &name)
{
	channels.push_back( tiledArray2D_t<float, 3>() );
	tiledArray2D_t<float, 3> &chan = channels.back();
	chan.resize(w, h, false);
	
	return channels.size();
}

imageFilm_t::~imageFilm_t ()
{
	delete image;
	delete[] filterTable;
	if(splitter) delete splitter;
	if(pbar) delete pbar; //remove when pbar no longer created by imageFilm_t!!
	//std::cout << "** imageFilter stats: unlocked adds: "<<_n_unlocked<<" locked adds: " <<_n_locked<<"\n";
}

__END_YAFRAY
