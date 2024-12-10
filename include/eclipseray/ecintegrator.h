///////////////////////////////////////////////////////////////////////////////
//
// ecintegrator
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
/// \file ecintegrator.h
/// \brief Defines interfaces for surface integrators
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 15:12:2008   9:53
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SURFACEINTEGRATOR_H
#define _SURFACEINTEGRATOR_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class SurfaceIntegrator
/// \author Dan Torres
/// \created 2008/12/15
/// \brief Base class for all supported surface integrators
//
/// An abstract interface for all other integrators. This class encapsulates
/// the YR integrator interface, and provides Python support. All concrete
/// implementations are simply specialized construction interfaces.
//
////////////////////////////////////////////////////////////////////////////
class SurfaceIntegrator : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:


    // -------------------------------------------------------------------------
    // Functions and utilities
    // -------------------------------------------------------------------------

    /*!
     *	Access our yafray integrator
     */
    inline YRSurfaceIntegrator* GetIntegrator(){ return m_pIntegrator; }

    // -------------------------------------------------------------------------
    // Python stuff
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates an integrator
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
    *  Python text representation method
    *  This function must be implemented by all children
    *  @return A python string object with a description of ourselves 
    */
    virtual PYOBJECT PyAsString();

protected:

    // Only concrete children can instantiate integrators
    SurfaceIntegrator();

    // Only concrete children can delete this class
    virtual ~SurfaceIntegrator();

    // Access our integrator
    inline YRSurfaceIntegrator*& GetYRIntegrator(){ return m_pIntegrator; }

    // Access the child description as a python object
    virtual PYOBJECT GetPyStringRep() ECLIPSE_PURE;

private:

    YRSurfaceIntegrator*        m_pIntegrator;  ///< Integrator interface

};


////////////////////////////////////////////////////////////////////////////
//
/// \class DirectLightingIntegrator
/// \author Dan Torres
/// \created 2008/12/15
/// \brief Defines a simple direct lighting surface integrator 
//
////////////////////////////////////////////////////////////////////////////
class DirectLightingIntegrator : public SurfaceIntegrator
{
public:

    /*!
     *  Simple constructor
     *  Direct lighting integrators have more parameters that are disabled if you
     *  use this constructor. More specifically, anything that has to do with caustics.
     *  If you need to enable caustics, use (or write) a different constructor
     *  @param a_sID Unique integrator's id
     *  @param a_nRayDepth  Maximum depth for recursive raytracing
     *  @param a_nPhotons Number of photons to be shot
     *  @param a_nTransparentShadowDepth Depth for transparent shadows. Disabled if zero
     */
	DirectLightingIntegrator( const char* a_sID, int a_nRayDepth, int a_nPhotons, 
        int a_nTransparentShadowDepth, bool a_bMonochrome );

protected:

    // Access the child description as a python object
    virtual PYOBJECT GetPyStringRep();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

private:

    // Only our internal functions can delete this object
    ~DirectLightingIntegrator();

};


////////////////////////////////////////////////////////////////////////////
//
/// \class PhotonIntegrator
/// \author Dan Torres
/// \created 12/30/2008
/// \brief Creates and maintains a photon surface integrator
//
////////////////////////////////////////////////////////////////////////////
class PhotonIntegrator : public SurfaceIntegrator
{
public:

    /*!
     *	Simple constructor
     *  @param a_sID Unique integrator name
     *  @param a_nRayDepth Maximum depth for recursive raytracing
     *  @param a_nShadowDepth Depth for transparent shadows, if greater than zero
     *  @param a_nPhotons Number of emitted photons
     *  @param a_fDiffuseRadius Radius to search for non-caustic photons
     *  @param a_nSearch Max number of non-caustic photons to be filtered
     *  @param a_nCausticMix Max. number of caustic photons to be filtered
     *  @param a_nBounces Max. number of scattering events for photons
     *  @param a_bUseBackground If true, background contributes to direct lighting
     *  @param a_nFGSamples Number of samples for final gathering
     *  @param a_nFGBounces Allow gather rays to extend to paths of this length
     */
	PhotonIntegrator( const char* a_sID, int a_nRayDepth, int a_nShadowDepth, int a_nPhotons,
        float a_fDiffuseRadius, int a_nSearch, int a_nCausticMix, int a_nBounces, 
        bool a_bUseBackground, int a_nFGSamples, int a_nFGBounces );

protected:

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

    // Access the child description as a python object
    virtual PYOBJECT GetPyStringRep();


private:

    // Restringed destructor
    ~PhotonIntegrator();
};

////////////////////////////////////////////////////////////////////////////
//
/// \class AmbientOcclusionIntegrator
/// \author Dan Torres
/// \created 1/12/2009
/// \brief An integrator that generates ambient occlusion information
//
////////////////////////////////////////////////////////////////////////////
class AmbientOcclusionIntegrator : public SurfaceIntegrator
{
public:

    /*!
     *	Basic constructor
     *  @param a_sID Unique integrator name
     *  @param a_nSamples Number of rays used to detect if an object is occluded
     *  @param a_fDistance Length of occlusion rays
     *  @param a_color Color for ambient occlusion rays
     */
	AmbientOcclusionIntegrator( const char* a_sID, int a_nSamples, float a_fDistance, YRColorRGB a_color );

protected:

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject(){ delete this; }

    // Access the child description as a python object
    virtual PYOBJECT GetPyStringRep();


private:

    // Restringed destructor
    ~AmbientOcclusionIntegrator();
};


#endif