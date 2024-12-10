///////////////////////////////////////////////////////////////////////////////
//
// ecmaterial
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
/// \file ecmaterial.h
/// \brief Defines the main material interface, and some known concrete materials
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 12:12:2008   14:41
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class Material
/// \author Dan Torres
/// \created 2008/12/12
/// \brief Material interface class
//
/// This class serves as an abstract interface for all other materials.
/// It contains the YR material object and conveys it to other processes in
/// the render cycle. Concrete classes are in charge of creating-configuring
/// the material object.
/// 
/// The material object itself is owned by the render environment that creates it.
//
////////////////////////////////////////////////////////////////////////////
class Material : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:
   

    // -------------------------------------------------------------------------
    // Utilities and functions
    // -------------------------------------------------------------------------
   
    /*!
     *	Public access to our Yafaray material object
     *  @return A pointer to a Yafaray material_t object
     */
    inline YRMaterial* GetYRMaterial() const { return m_pMaterial; }
    

    // -------------------------------------------------------------------------
    // Python stuff
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates an material
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

    // Only concrete classes can instantiate new materials
    Material();

    /// Virtual destructor
    virtual ~Material();

    // Access our actual material object
    inline YRMaterial*& GetYRMaterialRef(){ return m_pMaterial; }

    // Delegate the description to our children
    virtual PYOBJECT GetPyStringRep() ECLIPSE_PURE;

private:

    YRMaterial*         m_pMaterial;    ///< Actual material

};


////////////////////////////////////////////////////////////////////////////
//
/// \class ShinyDiffuseMaterial
/// \author Dan Torres
/// \created 2008/12/12
/// \brief Defines a conventional material with specular and diffuse properties
//
////////////////////////////////////////////////////////////////////////////
class ShinyDiffuseMaterial : public Material
{
public:

    /*!
     *  Simplified constructor
     *  The material itself supports more parameters than these, but this simplified
     *  function includes the ones that are frequently more important. Others use default values.
     *  @param a_sID Unique ID for this material
     *  @param a_color Diffuse color
     *  @param a_fTransparency Material transparency
     *  @param a_fTranslucency Amount of diffuse transmission
     *  @param a_fDiffuseReflect Amount of diffuse reflection
     *  @param a_fSpecularReflect Amount of perfec specular reflection ("mirror")
     *  @param a_fEmisiveness Self-emissive term
     *  @param a_fFresnel Refraction index for fresnel effect. Set to zero to disable fresnel.
     *  @param a_fONSigma Sigma value for Oren-Nayar BRDF. Set to zero to disable.
     */
	ShinyDiffuseMaterial( const char* a_sID,
        YRColorRGB& a_color, 
        float a_fTransparency    = 0.0f, 
        float a_fTranslucency    = 0.0f, 
        float a_fDiffuseReflect  = 1.0f,
        float a_fSpecularReflect = 0.0f,
        float a_fEmisiveness     = 0.0f,
        float a_fFresnel         = 1.33f,
        float a_fONSigma         = 0.0f );

protected:

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

    // Return a description of this material
    PYOBJECT GetPyStringRep();


private:

    // Only our internal functions can delete this material
    ~ShinyDiffuseMaterial();
};


#endif