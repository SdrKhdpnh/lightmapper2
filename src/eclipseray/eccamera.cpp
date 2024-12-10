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
/// \file eccamera.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 1/13/2009
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray\eccamera.h>
#include <eclipseray\ecmesh.h>
#include <eclipseray\ecfilm.h>
#include <eclipseray\utils.h>

#include <yafraycore\triangle.h>

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

#define LMC_DEBUG_CODE( _code, _enable )    \
    if( _enable ){ _code }

// Epsilon for various calculations
#define LMC_EPSILON 0.0001f

// If enabled, we sort slab edges after triangle addition. If disabled,
// each edge is inserted on the right place during triangle addition.
// According to some experimental tests, the first option is slightly faster
#define LMC_SORT_EDGES_AFTER_TRIANGLES

// If enabled, the direction of V coordinates is flipped
#define LMC_FLIP_V

// Surface tolerance for the returned ray
#define LMC_RAY_SURFACE_TOLERANCE 0.05f

// Returns the correctly aligned V coordinate
#ifdef LMC_FLIP_V
    #define LMC_ALIGNED_V( _v, _vScale ) (_vScale - _v)
#else
    #define LMC_ALIGNED_V( _v, _uScale ) (_v)
#endif

// A class for several list-related utilities
class LMCUtilities
{

public:

    // Sorts uv pairs in ascending order
    static bool SortByUAscending( const YRuv& a, const YRuv& b );

    // Sorts slabs in ascending order
    static bool SortByFilmUAscending( const LightmapCamera::sSlab& a, const LightmapCamera::sSlab& b );


    // Assign the current slab
    static void SetCurrentSlab( LightmapCamera::sSlab* a_pSlab );

    // Sort layers on the current slab in ascending order
    static bool SortEdgesAscending( const LightmapCamera::sEdge& a, const LightmapCamera::sEdge& b );

    // Sets the curent search triangle
    static void SetCurrentEdge( int a_nP1, int a_nP2 );

    // Sets the current search point
    static void SetCurrentUV( YRuv& a_uv );

    // Finds the first edge that sits below the current V
    static bool FindFirstLowerEdge( const LightmapCamera::sEdge& a );

private:

    static int                      m_p1;
    static int                      m_p2;
    static YRuv                     m_uv;
    static LightmapCamera::sSlab*   m_pCurrentSlab;
    static float                    m_fMidpointU;

};

int                     LMCUtilities::m_p1              = 0;
int                     LMCUtilities::m_p2              = 0;
LightmapCamera::sSlab*  LMCUtilities::m_pCurrentSlab    = NULL;
float                   LMCUtilities::m_fMidpointU      = 0.0f;
YRuv                    LMCUtilities::m_uv(0.0f,0.0f);

// -----------------------------------------------------------------------------
// LightmapCamera implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
LightmapCamera::LightmapCamera( Film* a_pFilm, Mesh* a_pMesh )
:EclipseObject( &m_PythonType ),
 m_nFilmWidth( 32 ),
 m_nFilmHeight( 32 ),
 m_pMesh( a_pMesh )
{    
    if( a_pFilm && m_pMesh && m_pMesh->IsValid() )
    {
        m_nFilmWidth  = a_pFilm->GetWidth();
        m_nFilmHeight = a_pFilm->GetHeight();

        if( PreProcessMesh() )
        {
            Utils::PrintMessage("Created lightmap camera");
            m_pMesh->AddRef();
            SetIsValid( true );        
        }
        else
        {
            Utils::ErrorManager::Report( 
                Utils::ErrorManager::ErrorType_Params, this,
                "Failed to process mesh for lightmapping. Make sure its UV map is correct!"
                );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
LightmapCamera::~LightmapCamera()
{
    if( IsValid() )
    {
        m_pMesh->ReleaseRef();        
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
bool LightmapCamera::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject && a_pObject->ob_type == &PYTHON_TYPE(LightmapCamera) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT LightmapCamera::PyAsString()
{
    return PyString_FromString("Lightmap camera object");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
void LightmapCamera::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
////////////////////////////////////////////////////////////////////////////////
YRRay LightmapCamera::shootRay(YRPFloat px, YRPFloat py, float u, float v, YRPFloat &wt) const
{
    YRRay ray;

    if( IsValid() )
    {
        // Find the point and normal for the intersected triangle
        YRPoint3D point;
        YRVector3D normal;

        LMC_DEBUG_CODE( Utils::PrintMessage("Testing %.2f,%.2f",px,py);, false );
        if( QueryMap(px, LMC_ALIGNED_V(py,m_nFilmHeight), point, normal) )
        {        
            LMC_DEBUG_CODE( Utils::PrintMessage("\t\t\tHITS");, false );

            ray.from = point + LMC_RAY_SURFACE_TOLERANCE * normal;
            ray.dir  = -normal;
            wt       = 1;

            //float fY = -6.49 + 12.0f * px / m_nFilmWidth;
            //ray.from.set( -1.24, fY, 10.0 );
            //ray.dir.set(0.0, 0.0, -1.0 );        

        }
        else
        {
            LMC_DEBUG_CODE( Utils::PrintMessage("\t\t\t\t\tMisses");,  false );
            wt = 0;
        }

    }
    else
    {
        wt = 0;
    }

    return ray;

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/14/2009
////////////////////////////////////////////////////////////////////////////////
bool LightmapCamera::PreProcessMesh()
{
    LMC_DEBUG_CODE( Utils::PrintMessage("------------------------------------ <SLABS> "); , false );
    YRTriangleObject* pTrimesh = m_pMesh->GetYRTrimesh();      

    if( pTrimesh->hasUVCoord() )
    {
        // Copy the list of uv's, as we are going to change their order
        std::vector<YRuv> uvValues = pTrimesh->getUVValues();        

        // 1. Generate a list of U segments and make a slab set
        std::sort( uvValues.begin(), uvValues.end(), LMCUtilities::SortByUAscending );

        int nArraySize = uvValues.size();
        float nLastU   = -1.0f;
        float nThisU   = 0.0f;
        
        for( int i = 0; i < nArraySize; i++ )
        {
            nThisU = uvValues[i].u * (float)m_nFilmWidth;
            
            if( fabs(nThisU - nLastU) > LMC_EPSILON && (nThisU < m_nFilmWidth))
            {
                if( m_slabs.size() > 0 )
                {                    
                    m_slabs.back().NextSlabU = nThisU;
                }
                
                nLastU = nThisU;
                m_slabs.push_back( sSlab(nLastU) );
                LMC_DEBUG_CODE( Utils::PrintMessage("Creating slab @ %f", nLastU);, false );
            }
        }

        // If we generated no slabs, return here
        if( m_slabs.size() == 0 )
        {
            LMC_DEBUG_CODE( Utils::PrintMessage("Can't generate uv maps! Returning");, true );
            return false;
        }

        m_slabs.back().NextSlabU = m_nFilmWidth;

        // 2. Generate edges and distribute them on slabs
        std::vector<yafaray::triangle_t>& triangles = pTrimesh->getTriangles();
        std::vector<YRuv>& uvValuesRef              = pTrimesh->getUVValues();
        std::vector<int>& uvOffsetsRef              = pTrimesh->getUVOffsets();
        nArraySize = triangles.size();
        int nA, nB, nC;
        int nUVa, nUVb, nUVc;
        YRuv uva(0,0), uvb(0,0), uvc(0,0);

        LMC_DEBUG_CODE( Utils::PrintMessage("Adding %d triangles", nArraySize); , false );

        // To do: This cycle could be paralellized
        for( int i = 0; i < nArraySize; i++ )
        {
            // To do: Create our own triangles, with our own data on them

            // Get the triangle points
            triangles[i].getTrianglePointIndices( nA, nB, nC );            
            LMC_DEBUG_CODE( Utils::PrintMessage("Triangle %d with points %d, %d, %d", i, nA, nB, nC);, false );
            LMC_DEBUG_CODE
            ( 
                YRVector3D normal = triangles[i].getNormal();
                Utils::PrintMessage("Normal for this triangle [%.2f, %.2f, %.2f]", normal[0], normal[1], normal[2] );,
                false
            )

            // Get the actual UV values for this triangle. Move them to film space.
            std::vector<int>::const_iterator uvi = uvOffsetsRef.begin() + 3 * i;
            nUVa = *uvi; nUVb = *(uvi+1); nUVc = *(uvi+2);

            uva.u = uvValuesRef[nUVa].u * m_nFilmWidth;
            uva.v = uvValuesRef[nUVa].v * m_nFilmHeight;

            uvb.u = uvValuesRef[nUVb].u * m_nFilmWidth;
            uvb.v = uvValuesRef[nUVb].v * m_nFilmHeight;
            
            uvc.u = uvValuesRef[nUVc].u * m_nFilmWidth;
            uvc.v = uvValuesRef[nUVc].v * m_nFilmHeight;

            // Insert all three edges it into our map
            InsertEdge( i, nA, nB, uva, uvb, uvc );
            InsertEdge( i, nA, nC, uva, uvc, uvb );
            InsertEdge( i, nB, nC, uvb, uvc, uva );

            // Calculate the transposed barycentric matrix for this triangle
            m_triangles.push_back( CalculateBarycentricTriangle( triangles[i].getNormal(), nA, nB, nC, uva, uvb, uvc ) );
        }

        LMC_DEBUG_CODE( Utils::PrintMessage("------------------------------------ <%d SLABS> ", m_slabs.size());, false );

        // Sort edges on all slabs. Don't do this if we insert edges in order
        // at the InsertEdge call
#ifdef LMC_SORT_EDGES_AFTER_TRIANGLES
        nArraySize = m_slabs.size();
        for( int i = 0; i < nArraySize; i ++ )
        {
            LMCUtilities::SetCurrentSlab( &m_slabs[i] );
            std::sort( m_slabs[i].Edges.begin(), m_slabs[i].Edges.end(), LMCUtilities::SortEdgesAscending );
        }
#endif

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/14/2009
// Make copies of a_uv1 and a_uv2, as we might shift them anyways.
////////////////////////////////////////////////////////////////////////////////
void LightmapCamera::InsertEdge( int a_nTriangleIndex, int a_nP1, int a_nP2, YRuv a_uv1, YRuv a_uv2, YRuv& a_corner )
{
    // Make sure the edges are within bounds
    if( a_uv1.u <= m_nFilmWidth && a_uv2.u <= m_nFilmWidth )
    {
        // Reverse order if necessary
        if( a_uv2.u < a_uv1.u )
        {
            int tP = a_nP1;
            a_nP1  = a_nP2;
            a_nP2  = tP;

            YRuv tuv = a_uv1;
            a_uv1    = a_uv2;
            a_uv2    = tuv;
        }

        // Reset an edge with these values. Make sure its not a vertical edge
        sEdge edge;
        if( ConfigureEdge( edge, a_uv1, a_uv2, a_nTriangleIndex, a_nP1, a_nP2, a_corner ))
        {
            // Fit the edge in the slab set.
            int nStart = GetSlab( a_uv1.u );
            int nEnd   = GetSlab( a_uv2.u );

            // Sanity check
            if (nStart < 0 || nEnd < 0 || nStart >= m_slabs.size() || nEnd >= m_slabs.size() || nStart > nEnd)
            {
                //std::cout << "Failed sanity check in InsertEdge: nStart:" << nStart << " nEnd:" << nEnd << " m_slabs.size():" << m_slabs.size() << "\n";
                return;
            }

            std::vector<sEdge>::iterator previousEdge;

            LMC_DEBUG_CODE(
                Utils::PrintMessage("\tu1(%.2f) @ slab %d, u2(%.2f) @ slab %d:", a_uv1.u, nStart, a_uv2.u, nEnd );,
                false );

            int i = nStart;
            do
            {
               LMC_DEBUG_CODE( Utils::PrintMessage("\t\tAdding edge from %d(%.2f,%.2f) to %d(%.2f,%.2f) in slab %d", 
                    a_nP1, a_uv1.u,a_uv1.v,a_nP2,a_uv2.u,a_uv2.v, i);,
                    false );

               // Instead of sorting the whole thing later, be smart as to where this will go
#ifndef LMC_SORT_EDGES_AFTER_TRIANGLES
               LMCUtilities::SetCurrentSlab( &m_slabs[i] );
               std::vector<sEdge>::iterator insertPoint = std::lower_bound( m_slabs[i].Edges.begin(),
                   m_slabs[i].Edges.end(), edge, LMCUtilities::SortEdgesAscending );

               if( insertPoint != m_slabs[i].Edges.end() )
                   m_slabs[i].Edges.insert( insertPoint, edge );
               else
                   m_slabs[i].Edges.push_back(edge);
#else
               m_slabs[i].Edges.push_back(edge);
#endif
                  
            }
            while (++i < nEnd);
        }
        else
        {
            LMC_DEBUG_CODE( 
                Utils::PrintMessage("\tRejected vertical edge: u(%.2f, %.2f)", a_uv1.u, a_uv2.u );,
                false );
        }
    }
    else
    {
        LMC_DEBUG_CODE( 
            Utils::PrintMessage("\tRejected edge: u(%.2f, %.2f)", a_uv1.u, a_uv2.u );,
            false );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/14/2009
////////////////////////////////////////////////////////////////////////////////
bool LightmapCamera::ConfigureEdge( sEdge& a_edge, YRuv& a_uv1, YRuv& a_uv2, int a_nFirstTriangle, int a_nP1, int a_nP2, YRuv& a_corner )
{
    // Make sure this is not a vertical edge
    float fDu = a_uv2.u - a_uv1.u;
    if( fabs(fDu) > LMC_EPSILON )
    {
        a_edge.Points[0]       = a_nP1;
        a_edge.Points[1]       = a_nP2;        
        a_edge.Triangle        = INVALID_ID;
        a_edge.LineEquation[0] = (a_uv2.v - a_uv1.v) / fDu;                     // slope
        a_edge.LineEquation[1] = a_uv1.v - a_edge.LineEquation[0] * a_uv1.u;    // rest of the equation

        // Only record the triangle if the edge lies below it
        float fD = a_corner.v -  (a_edge.LineEquation[0] * a_corner.u + a_edge.LineEquation[1]);
        if(  fD <= 0.0f )
        {
            a_edge.Triangle = a_nFirstTriangle;

            LMC_DEBUG_CODE(
                Utils::PrintMessage("\t--Edge (%.2f, %.2f) on the negative side (%.2f), ZTS = true", a_corner.u, a_corner.v, fD );,
                false );
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/14/2009
////////////////////////////////////////////////////////////////////////////////
int LightmapCamera::GetSlab( float a_fFilmU ) const
{
    LMC_DEBUG_CODE( Utils::PrintMessage("GetSlab for %.2f", a_fFilmU);, false );

    if( a_fFilmU <= m_nFilmWidth )
    {
        std::vector<sSlab>::const_iterator lower;
        sSlab dummy(a_fFilmU);

        // Find first slab whose boundary is equal or bigger than the requested one
        lower = std::lower_bound( m_slabs.begin(), m_slabs.end(), dummy, LMCUtilities::SortByFilmUAscending );

        // If we could find a match...
        if( lower != m_slabs.end() )
        {
            int index = lower - m_slabs.begin();

            // We are either right on this slab, or we found the next bigger one
            if ( fabs((*lower).FilmU - a_fFilmU) > LMC_EPSILON )
            {
                LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Found: %d", index-1);, false );
                return index - 1;
            }
            else
            {
                LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Exact: %d", index);, false );
                return index;
            }
        }
        else
        {
            // It could either be that we have no slabs (unlikely), or that we 
            // find a match between the last one, and the film boundary
            if( m_slabs.size() > 0 )
            {
                LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Edge: %d", m_slabs.size() - 1);, false );
                return m_slabs.size() - 1;
            }
            else
            {
                LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Invalid: No slabs");, false );
                return INVALID_ID;
            }
        }
    }
    else
    {
        // When the requested slab exceeds our map, return the last available slab
        if( m_slabs.size() > 0 )
        {
            LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Edge: %d", m_slabs.size() - 1);, false );
            return m_slabs.size() - 1;
        }
        else            
        {
            LMC_DEBUG_CODE( Utils::PrintMessage("\t---> Invalid: No slabs");, false );
            return INVALID_ID;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/16/2009
////////////////////////////////////////////////////////////////////////////////
LightmapCamera::sTriangle LightmapCamera::CalculateBarycentricTriangle( YRVector3D& a_vNormal, int a_p1, int a_p2, int a_p3, 
    YRuv& a_uv1, YRuv& a_uv2, YRuv& a_uv3 )
{
    sTriangle t;

    // Calculate the inverse barycentric matrix based on our points. This is useful for
    // later finding the barycentric coordinates of a point inside a triangle, and thus
    // projecting into 3d space.
    float fx1x3 = a_uv1.u - a_uv3.u;
    float fy2y3 = a_uv2.v - a_uv3.v;
    float fx2x3 = a_uv2.u - a_uv3.u;
    float fy1y3 = a_uv1.v - a_uv3.v;
    float fx3x2 = a_uv3.u - a_uv2.u;
    float fy3y1 = a_uv3.v - a_uv1.v;
    float K     = 1.0f / (fx1x3 * fy2y3 - fx2x3 * fy1y3);

    // Complete our triangle data. Sacrifice space for performance
    t.Points [0] = a_p1;
    t.Points [1] = a_p2;
    t.Points [2] = a_p3;

    t.BMatrix[0] = fy2y3 * K;
    t.BMatrix[1] = fx3x2 * K;
    t.BMatrix[2] = fy3y1 * K;
    t.BMatrix[3] = fx1x3 * K;

    t.UV         = a_uv3;
    t.Normal     = a_vNormal;

    return t;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
bool LightmapCamera::QueryMap( YRPFloat a_fU, YRPFloat a_fV, YRPoint3D& a_vPoint, YRVector3D& a_vNormal) const 
{
    // Get a slab index based on u
    int nSlab = GetSlab( a_fU );
    if( nSlab != INVALID_ID )
    {

        //LMC_DEBUG_CODE
        //(
        //    int i = 0;
        //    Utils::PrintMessage("Testing uv(%.2f, %.2f) on slab %d with %d edges", a_fU, a_fV, nSlab, m_slabs[nSlab].Edges.size());
        //    for (std::vector<sEdge>::const_iterator edge = m_slabs[nSlab].Edges.begin(); edge != m_slabs[nSlab].Edges.end(); edge++ ){
        //        float fX = (*edge).LineEquation[0] * a_fU + (*edge).LineEquation[1];
        //        Utils::PrintMessage("\t\tEdge %d from points %d to %d: %.2f units away", i, (*edge).Points[0],(*edge).Points[1], a_fV - fX );
        //        i++;
        //    },
        //    false
        //)

        // Find the first edge that sits below the requested v
        LMCUtilities::SetCurrentUV( YRuv(a_fU,a_fV) );
        std::vector<sEdge>::const_iterator edge = std::find_if( m_slabs[nSlab].Edges.begin(), m_slabs[nSlab].Edges.end(),
            LMCUtilities::FindFirstLowerEdge );

        if( edge != m_slabs[nSlab].Edges.end() )
        {
            // An invalid ID would mean we hit no triangle on this part of our uv map
            if( edge->Triangle != INVALID_ID )
            {
                // Now that we have a triangle, rasterize a position. Since we have our
                // inverse baricentric matrix, its easy to find our three barycentric
                // coordinates and thus infer the interpolated point in 3D space:

                const sTriangle& t = m_triangles[edge->Triangle];
                float fU  = a_fU - t.UV.u;
                float fV  = a_fV - t.UV.v;

                float fL1 = t.BMatrix[0]*fU + t.BMatrix[1]*fV;
                float fL2 = t.BMatrix[2]*fU + t.BMatrix[3]*fV;
                float fL3 = 1.0f - fL1 - fL2;

                // Calculate the actual point in 3D space
                YRTriangleObject* pTrimesh = m_pMesh->GetYRTrimesh();      
                std::vector<YRPoint3D>::const_iterator points = pTrimesh->getPointsIterator();

                YRPoint3D p1 = *(points + t.Points[0]);
                YRPoint3D p2 = *(points + t.Points[1]);
                YRPoint3D p3 = *(points + t.Points[2]);

                a_vPoint  = p1 * fL1 + p2 * fL2 + p3 * fL3;
                a_vNormal = t.Normal;

                return true;
            }
            else
            {
                LMC_DEBUG_CODE( Utils::PrintMessage("---- No hit");, false );
                return false;
            }
        }

    }

    return false;

}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( LightmapCamera )
    // ...
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( LightmapCamera, "LightmapCamera", "Lightmap camera object", PYTHON_TYPE_FINAL );

// -----------------------------------------------------------------------------
// Utility implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
bool LMCUtilities::SortByUAscending( const YRuv& a, const YRuv& b )
{
    return a.u < b.u;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
bool LMCUtilities::SortByFilmUAscending( const LightmapCamera::sSlab& a, const LightmapCamera::sSlab& b )
{
    return a.FilmU < b.FilmU;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
void LMCUtilities::SetCurrentEdge( int a_nP1, int a_nP2 )
{
    m_p1 = a_nP1;
    m_p2 = a_nP2;    
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
void LMCUtilities::SetCurrentUV( YRuv& a_uv )
{
    m_uv = a_uv;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/15/2009
////////////////////////////////////////////////////////////////////////////////
bool LMCUtilities::FindFirstLowerEdge( const LightmapCamera::sEdge& a )
{
    // Distance to the edge must be positive
    float fX = a.LineEquation[0] * m_uv.u + a.LineEquation[1];
    if( m_uv.v - fX <= 0.0f )
        return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/16/2009
////////////////////////////////////////////////////////////////////////////////
void LMCUtilities::SetCurrentSlab( LightmapCamera::sSlab* a_pSlab )
{
    m_pCurrentSlab = a_pSlab;
    m_fMidpointU   = m_pCurrentSlab->FilmU + ( m_pCurrentSlab->NextSlabU - m_pCurrentSlab->FilmU ) * 0.5f;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/16/2009
////////////////////////////////////////////////////////////////////////////////
bool LMCUtilities::SortEdgesAscending( const LightmapCamera::sEdge& a, const LightmapCamera::sEdge& b )
{
    // Return TRUE if A sits below B
    float fA = a.LineEquation[0] * m_fMidpointU + a.LineEquation[1];
    float fB = b.LineEquation[0] * m_fMidpointU + b.LineEquation[1];
    if (fabs(fA - fB) < LMC_EPSILON)
    {
        // NB: This only works if there are no overlapping triangles! - JamesG
        return a.Triangle != -1;
    }
    return fA < fB;
}
