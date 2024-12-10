/*
	these are the triangle functions for the non-virtual triangle_t,
	since they are inlined but cannot be declared in triangle.h
	(=> cyclic inclusion because triangleObject_t needs to know triangle_t,
	and these functions need to know triangleObject_t)
	If you need triangles, please just include "object3d.h" rather than
	"triangle.h" and "triangle_inline.h" directly.
*/

inline bool triangle_t::intersect(const ray_t &ray, PFLOAT *t, void *userdata) const
{
#ifdef _OLD_TRIINTERSECT
	//"old" intersect code
	const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];
	const vector3d_t va=(a)-ray.from,vb=(b)-ray.from,vc=(c)-ray.from;
	vector3d_t r;
	if((ray.dir*vector3d_t(normal))<0) r=-ray.dir;
	else r=ray.dir;
	if( ((va^vb)*r)<0 ) return false;
	if( ((vb^vc)*r)<0 ) return false;
	if( ((vc^va)*r)<0 ) return false;
	*t = (vector3d_t(normal)*(a-ray.from))/(vector3d_t(normal)*ray.dir);
	return true;
#else
	//Tomas M�ller and Ben Trumbore ray intersection scheme
	const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];
	vector3d_t edge1, edge2, tvec, pvec, qvec;
	PFLOAT det, inv_det, u, v;
	edge1 = b - a;
	edge2 = c - a;
	pvec = ray.dir ^ edge2;
	det = edge1 * pvec;
	if (/*(det>-0.000001) && (det<0.000001)*/ det == 0.0) return false;
	inv_det = 1.0 / det;
	tvec = ray.from - a;
	u = (tvec*pvec) * inv_det;
	if (u < 0.0 || u > 1.0) return false;
	qvec = tvec^edge1;
	v = (ray.dir*qvec) * inv_det;
	if ((v<0.0) || ((u+v)>1.0) ) return false;
	*t = edge2 * qvec * inv_det;
	PFLOAT *dat = (PFLOAT*)userdata;
	dat[0]=u; dat[1]=v;
	return true;
#endif
}
inline bound_t triangle_t::getBound() const
{
	const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];
	point3d_t l, h;
	l.x = Y_MIN3(a.x, b.x, c.x);
	l.y = Y_MIN3(a.y, b.y, c.y);
	l.z = Y_MIN3(a.z, b.z, c.z);
	h.x = Y_MAX3(a.x, b.x, c.x);
	h.y = Y_MAX3(a.y, b.y, c.y);
	h.z = Y_MAX3(a.z, b.z, c.z);
	return bound_t(l, h);
}

inline bool triangle_t::intersectsBound(exBound_t &eb) const
{
	double tPoints[3][3];
	const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];
	for(int j=0; j<3; ++j)
	{
		tPoints[0][j] = a[j];
		tPoints[1][j] = b[j];
		tPoints[2][j] = c[j];
	}
	// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap(eb.center, eb.halfSize, tPoints);
}

inline void triangle_t::recNormal()
{
	const point3d_t &a=mesh->points[pa], &b=mesh->points[pb], &c=mesh->points[pc];
	normal = ((b-a)^(c-a)).normalize();
}