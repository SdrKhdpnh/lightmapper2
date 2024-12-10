///////////////////////////////////////////////////////////////////////////////
//
// eccamera
//
// Copyright (C) 2009 BioWare
//
// This file is part of EclipseRay.
// 
// EclipseRay is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// EclipseRay is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with EclipseRay.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////
//
/// \file eccamera.h
/// \brief Defines cameras specific to EclipseRay
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 1/13/2009
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ECCAMERA_H
#define _ECCAMERA_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

class Mesh;
class Film;

////////////////////////////////////////////////////////////////////////////
//
/// \class LightmapCamera
/// \author Dan Torres
/// \created 1/13/2009
/// \brief Defines a special purpose camera for rendering lightmaps
//
///  To render lightmaps, a planar version of the mesh is "unwrapped" in uv
///  space (existing uv coordinates are used for this) To quickly find triangle
///  intersections, vertical lines are traced on each vertex position to create
///  'slabs', and each slab contains a list of ordered lines that cross it. It is
///  assumed that all mappings form a convex hull, and there is no uv overlapping.
///
///  To query this map, two binary search operations are needed: One to find the
///  right slab, and another one to find the right edge (basically, a trapezoidal
///  map) To deduct a position in 3D space from the planar uv space of the triangle, the 
///  barycentric components of the intersected triangle are used. To this end, each 
///  triangle pre-calculates its transposed barycentric matrix, so finding a 3D point is 
///  quick and efficient. For more information on this, read [1] and [2]
///
///  References:
///
///  [1] Berg, M. et.al. Computational Geometry. pg. 121 and following.
///  [2] Mobius, August. F. Der Barycentrische Calcul. 1827.
//
////////////////////////////////////////////////////////////////////////////
class LightmapCamera : public YRCamera, public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    /*!
     *	Constructor
     *  @param a_pFilm Film used for rendering with this camera
     *  @param a_pMesh Mesh to render lightmaps for
     */
	LightmapCamera( Film* a_pFilm, Mesh* a_pMesh );

    // -------------------------------------------------------------------------
    // Python interface
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates a camera object
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
    *  Python text representation method
    *  This function must be implemented by all children
    *  @return A python string object with a description of ourselves 
    */
    virtual PYOBJECT PyAsString();


    // -------------------------------------------------------------------------
    // Raytracer interface
    // -------------------------------------------------------------------------

    virtual YRRay shootRay( YRPFloat px, YRPFloat py, float u, float v, YRPFloat &wt) const;
    virtual int resX() const { return m_nFilmWidth;  }
    virtual int resY() const { return m_nFilmHeight; }
    virtual bool sampleLense() const {return false; }
    Mesh* GetMesh() { return m_pMesh; }

    // -------------------------------------------------------------------------
    // Special structures
    // -------------------------------------------------------------------------

    struct sEdge
    {
        float           LineEquation[2];    // Equation for this edge [slope, slope*u1 + v1]
        int             Triangle;           // Triangle that lies above the edge (index into triangle array)
        int             Points[2];          // Indexes to the points that form this edge
    };

    struct sSlab
    {
        sSlab( float a_fU ):FilmU(a_fU){}   // utility constructor
        std::vector<sEdge> Edges;           // Edges for this slab
        float              FilmU;           // Vertical cutting plane in film space
        float              NextSlabU;       // Vertical plane of the next slab
    };

    struct sTriangle
    {
        float           BMatrix[4];         // Barycentric matrix
        int             Points[3];          // Indexes for the actual points inside the trimesh
        YRuv            UV;                 // UV for one point, in film space
        YRVector3D      Normal;             // Known normal for this triangle
    };


protected:

    // Destructor
    ~LightmapCamera();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

private:

    // Creates a spatial map for querying mesh faces
    bool PreProcessMesh();

    // Gets the index of a slab that contains the provided u coordinate in film space
    int GetSlab( float a_fFilmU ) const;

    // Configures an edge with the information obtainable by the provided triangle indexes
    // Returns false if the edge happens to be vertical
    bool ConfigureEdge( sEdge& a_edge, YRuv& a_uv1, YRuv& a_uv2, int a_nFirstTriangle, int a_nP1, int a_nP2, YRuv& a_corner );

    // Creates a triangle with our own data to keep in this list
    sTriangle CalculateBarycentricTriangle( YRVector3D& a_vNormal, int a_p1, int a_p2, int a_p3, YRuv& a_uv1, YRuv& a_uv2, YRuv& a_uv3 );

    // Inserts the provided edge into our slab list
    void InsertEdge( int a_nTriangleIndex, int a_nP1, int a_nP2, YRuv a_uv1, YRuv a_uv2, YRuv& a_corner );

    // Queries for the position of a point/normal in the mesh, given its uv coords.
    // Returns false if no face is intersected by the provided uv coordinates.
    bool QueryMap( YRPFloat a_fU, YRPFloat a_fV, YRPoint3D& a_vPoint, YRVector3D& a_vNormal ) const;

    std::vector<sSlab>      m_slabs;            ///< List of slabs in our uv map
    std::vector<sTriangle>  m_triangles;        ///< List of triangles, 1-to-1 correspondance to the trimesh
    int                     m_nFilmWidth;       ///< Film width in pixels
    int                     m_nFilmHeight;      ///< Film height in pixels
    Mesh*                   m_pMesh;            ///< Mesh to lightmap

    static const int    INVALID_ID = -1;    // Value of an invalid index
};


#endif