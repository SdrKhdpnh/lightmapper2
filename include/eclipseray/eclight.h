///////////////////////////////////////////////////////////////////////////////
//
// eclight
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
/// \file eclight.h
/// \brief Defines our basic light objects for lightmapping operations
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 10:12:2008   15:19
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LIGHT_H
#define _LIGHT_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class Light
/// \author Dan Torres
/// \created 2008/12/10
/// \brief Base light class for light definitions coming from the DA toolset
//
/// This class contains base information for lights, as specified on the
/// DragonAge's toolset. It also provides means for representing them
/// internally in our raytraced scene.
/// 
/// Concrete implementations of this class are essentially managers for the
/// different types of lights supported by the raytracer.
/// 
/// Yafray Lights are implicitly owned by their environment
//
////////////////////////////////////////////////////////////////////////////
class Light : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:
  

    // -------------------------------------------------------------------------
    // Construction / destruction
    // -------------------------------------------------------------------------

    /// Destructor
	virtual ~Light();

    // -------------------------------------------------------------------------
    // Setters, getters
    // -------------------------------------------------------------------------

    /*!
     *	Access the YR light object
     */
    inline YRLight* GetLight(){ return m_pYRLight; }

    // -------------------------------------------------------------------------
    // Python stuff
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates a light
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

    /// Default constructor. Usable by children only.
    Light();

    /// Provides access to our raytracer light
    inline YRLight*& GetYRLight(){ return m_pYRLight; }

    /// Access to our shadow samples
    inline int& ShadowSamples(){ return m_nShadowSamples; }

    /// Access our shadow color
    inline YRColorRGB& ShadowColor(){ return m_nShadowColor; }

    // Children can use this utility function to generate lights based on the provided params
    bool GenerateLight( const char* a_sID, YRParameterMap& a_params );

    /// Provide a string representation of this child
    virtual PYOBJECT GetPyStringRep() ECLIPSE_PURE;

private:

    int                 m_nShadowSamples;   ///< Shadow sample count
    YRColorRGB          m_nShadowColor;     ///< Color used for shadows caused by this light
    YRLight*            m_pYRLight;         ///< Effective raytracing light
};


////////////////////////////////////////////////////////////////////////////
//
/// \class PointLight
/// \author Dan Torres
/// \created 2008/12/10
/// \brief Defines a basic omnidirectional light
//
/// Basic omnidirectional light. This class provides a wrapper around
/// the actual light as defined by the raytracer, and implements the 
/// python object interface.
//
////////////////////////////////////////////////////////////////////////////
class PointLight : public Light
{

public:

    /*!
     *  Complete constructor
     *  @param a_sID Unique light ID
     *  @param a_position Spatial localization of this light
     *  @param a_color Color emitted by the light
     *  @param a_fBrightness Base strength or intensity for this light
     *  @param a_shadowColor Color of the shadow produced by this light
     *  @param a_nShadowSamples Number of shadow samples caused by this light
     */
	PointLight( const char* a_sID, const YRPoint3D& a_position, const YRColorRGB& a_color, 
        float a_fBrightness, const YRColorRGB& a_shadowColor, int a_nShadowSamples, float a_fRadius );

    /*!
    *  Simplified constructor
    *  @param a_sID Unique light ID
    *  @param a_position Spatial localization of this light
    *  @param a_color Color emitted by the light
    *  @param a_fBrightness Base strength or intensity for this light
    */
    PointLight( const char* a_sID, const YRPoint3D& a_position, const YRColorRGB& a_color, 
        float a_fBrightness, float a_fRadius );


protected:

    /// Simple destructor
    ~PointLight();

    // Provide a representation of this light
    virtual PYOBJECT GetPyStringRep();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

};

////////////////////////////////////////////////////////////////////////////
//
/// \class AreaLight
/// \author Dan Torres
/// \created 1/2/2009
/// \brief Defines a light that spawns a given area
/// 
/// Base area light. Irradiates over the designated quad, as specified by
/// the base and two reference points
//
////////////////////////////////////////////////////////////////////////////
class AreaLight : public Light
{
public:

    /*!
     *	Complete constructor
     *  @param a_sID Unique light ID
     *  @param a_color Color emitted by the light
     *  @param a_corner One corner of the rectangular light shape
     *  @param a_point1 One edge
     *  @param a_point2 The other edge
     *  @param a_fPower Light intensity
     *  @param a_nSamples Number of samples to be taken for lighting
     */
	AreaLight( const char* a_sID, const YRColorRGB& a_color, const YRPoint3D& a_corner, 
        const YRPoint3D& a_point1, const YRPoint3D& a_point2, float a_fPower, int a_nSamples);

protected:

    // Destruction
    ~AreaLight();

    // Provide a representation of this light
    virtual PYOBJECT GetPyStringRep();


    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

};


////////////////////////////////////////////////////////////////////////////
//
/// \class AmbientLight
/// \author Dan Torres
/// \created 1/20/2009
/// \brief Simulates a light that comes from all directions
//
////////////////////////////////////////////////////////////////////////////
class AmbientLight : public Light
{
public:
    /*!
     *	Complete constructor
     *  @param a_sID Unique ID for this light
     *  @param a_color Color for this light
     */
	AmbientLight( const char* a_sID, YRColorRGB& a_color );

protected:

    // Destruction
    ~AmbientLight();

    // Provide a representation of this light
    virtual PYOBJECT GetPyStringRep();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

private:

    static bool     m_bFactoryRegistered;   // True once the ambient light factory has been registered

};


////////////////////////////////////////////////////////////////////////////
//
/// \class SpotLight
/// \author Dan Torres
/// \created 1/20/2009
/// \brief A light that irradiates in a cone-shape way
//
////////////////////////////////////////////////////////////////////////////
class SpotLight : public Light
{
public:

    /*!
     *	Complete constructor
     *  @param a_sID Unique ID
     *  @param a_color Color for this light
     *  @param a_fConeAngle Cone angle in degrees, from 0 to 180
     *  @param a_from Position for this light
     *  @param a_to Target point (NOT a directional vector) for this light
     *  @param a_fPower Intensity multiplier
     *  @param a_fFalloff width of falloff effect, from 0 to 1
     */
	SpotLight( const char* a_sID, YRColorRGB a_color, float a_fConeAngle, YRPoint3D a_from, 
        YRPoint3D a_to, float a_fPower, float a_fFalloff, float a_fRadius );
	

protected:

    // Destructor
    ~SpotLight();

    // Provide a representation of this light
    virtual PYOBJECT GetPyStringRep();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

};

////////////////////////////////////////////////////////////////////////////
//
/// \class DirectionalLight
/// \author James Goldman
/// \created 3/19/2009
/// \brief A light that illuminates from a single direction
//
////////////////////////////////////////////////////////////////////////////
class DirectionalLight : public Light
{
public:

    /*!
     *	Complete constructor
     *  @param a_sID Unique ID
     *  @param a_color Color
     *  @param a_fBrightness Power
     *  @param a_bInfinite Whether or not the light is bounded
     *  @param a_from If not infinite, the source position of the light
     *  @param a_fConeRadius If not infinite, the radius of the spot cone
     */
    DirectionalLight(const char* a_sID, const YRColorRGB& a_color, float a_fBrightness, 
        bool a_bInfinite, const YRPoint3D& a_from, float a_fConeRadius);

protected:

    ~DirectionalLight();

    virtual PYOBJECT GetPyStringRep();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();
};









#endif