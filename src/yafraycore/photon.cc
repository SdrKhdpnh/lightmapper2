
#include <yafraycore/photon.h>

__BEGIN_YAFRAY

YAFRAYCORE_EXPORT dirConverter_t dirconverter;

dirConverter_t::dirConverter_t()
{
	for(int i=0;i<255;++i)
	{
		PFLOAT angle=(PFLOAT)i*(1.0/255.0)*M_PI;
		costheta[i]=cos(angle);
		sintheta[i]=sin(angle);
	}
	for(int i=0;i<256;++i)
	{
		PFLOAT angle=(PFLOAT)i*(1.0/256.0)*2.0*M_PI;
		cosphi[i]=cos(angle);
		sinphi[i]=sin(angle);
	}
}

photonGather_t::photonGather_t(u_int32 mp, const point3d_t &P): p(P)
{
	photons = 0;
	nLookup = mp;
	foundPhotons = 0;
}

void photonGather_t::operator()(const photon_t *photon, PFLOAT dist2, PFLOAT &maxDistSquared) const
{
	// Do usual photon heap management
	if (foundPhotons < nLookup) {
		// Add photon to unordered array of photons
		photons[foundPhotons++] = foundPhoton_t(photon, dist2);
		if (foundPhotons == nLookup) {
				std::make_heap(&photons[0], &photons[nLookup]);
				maxDistSquared = photons[0].distSquare;
		}
	}
	else {
		// Remove most distant photon from heap and add new photon
		std::pop_heap(&photons[0], &photons[nLookup]);
		photons[nLookup-1] = foundPhoton_t(photon, dist2);
		std::push_heap(&photons[0], &photons[nLookup]);
		maxDistSquared = photons[0].distSquare;
	}
}

void photonMap_t::updateTree()
{
	if(tree) delete tree;
	if(photons.size() > 0)
	{
		tree = new kdtree::pointKdTree<photon_t>(photons);
		updated = true;
	}
	else tree=0;
}

/*
void photonMap_t::gather(const point3d_t &P, std::vector< foundPhoton_t > &found, unsigned int K, PFLOAT &sqRadius) const
{
	photonGather_t proc(K, P);
	proc.photons = new foundPhoton_t[K];
//	std::cout << "performing lookup\n";
	tree->lookup(P, proc, sqRadius);
	for(u_int32 i=0; i<proc.foundPhotons; ++i) found.push_back(proc.photons[i]);
	delete[] proc.photons;
}*/

int photonMap_t::gather(const point3d_t &P, foundPhoton_t *found, unsigned int K, PFLOAT &sqRadius) const
{
	photonGather_t proc(K, P);
	proc.photons = found;
	tree->lookup(P, proc, sqRadius);
	return proc.foundPhotons;
}

const photon_t* photonMap_t::findNearest(const point3d_t &P, const vector3d_t &n, PFLOAT dist) const
{
	nearestPhoton_t proc(P, n);
	//PFLOAT dist=std::numeric_limits<PFLOAT>::infinity(); //really bad idea...
	tree->lookup(P, proc, dist);
	return proc.nearest;
}

__END_YAFRAY
