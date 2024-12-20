
#ifndef Y_IMAGESPLITTER_H
#define Y_IMAGESPLITTER_H

#include <yafray_config.h>

#include <vector>

__BEGIN_YAFRAY


struct renderArea_t
{
	renderArea_t(int x,int y,int w,int h):X(x),Y(y),W(w),H(h),
		realX(x),realY(y),realW(w),realH(h),depth(w*h),resample(w*h)
	{};
	renderArea_t() {};

	void set(int x,int y,int w,int h)
	{
		realX=X=x;
		realY=Y=y;
		realW=W=w;
		realH=H=h;
//		image.resize(w*h);
		depth.resize(w*h);
		resample.resize(w*h);
	}
	void setReal(int x,int y,int w,int h)
	{
		realX=x;
		realY=y;
		realW=w;
		realH=h;
	}
	bool checkResample(CFLOAT threshold);
//	bool out(colorOutput_t &o);

//	colorA_t & imagePixel(int x,int y) {return image[(y-Y)*W+(x-X)];};
	PFLOAT & depthPixel(int x,int y)   {return depth[(y-Y)*W+(x-X)];};
	bool  resamplePixel(int x,int y)  {return resample[(y-Y)*W+(x-X)];};

	int X,Y,W,H,realX,realY,realW,realH;
	int sx0, sx1, sy0, sy1; //!< safe area, i.e. region unaffected by samples outside (needs to be set by ImageFilm_t)
//	std::vector<colorA_t> image;
	std::vector<PFLOAT> depth;
	std::vector<bool> resample;
};

/*!	Splits the image to be rendered into pieces, e.g. "buckets" for
	different threads.
	CAUTION! Some methods need to be thread save!
*/
class imageSpliter_t
{
	public:
		imageSpliter_t(int w, int h, int x0,int y0, int bsize);
		/* return the n-th area to be rendered.
			\return false if n is out of range, true otherwise
		*/
		bool getArea(int n, renderArea_t &area);

		bool empty()const {return regions.empty();};
		int size()const {return regions.size();};

	protected:
		struct region_t
		{
			int x,y,w,h;
//			int rx,ry,rw,rh;
		};
		int width,height,blocksize;
		std::vector<region_t> regions;
};

__END_YAFRAY

#endif // Y_IMAGESPLITTER_H
