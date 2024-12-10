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
/// \file ecmaterial.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 12:12:2008   15:01
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecmaterial.h>
#include <eclipseray/utils.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/ecgeometry.h>
#include <eclipseray/ecrenderenvironment.h>


// -----------------------------------------------------------------------------
// Material module definition
// -----------------------------------------------------------------------------

// Methods
PYTHON_MODULE_METHOD_VARARGS( materials, shinydiffuse );
// ...

// Method dictionary
START_PYTHON_MODULE_METHODS( materials )
    ADD_MODULE_METHOD( materials, shinydiffuse, "Creates a new shiny diffuse material", METH_VARARGS ),    
    // ...
END_PYTHON_MODULE_METHODS();


// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( materials, "Materials" )
{
    CREATE_PYTHON_MODULE( materials, "Material object collection", pMaterialModule );
    return pMaterialModule;
}



// -----------------------------------------------------------------------------
// Material definition
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
Material::Material()
:EclipseObject( &m_PythonType ),
 m_pMaterial( NULL )
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
Material::~Material()
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Material::PyTypeCheck(PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Material) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Material::PyAsString()
{
    return GetPyStringRep();   
}


// -----------------------------------------------------------------------------
// Material Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Material )
    // ...
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Material, "Material", "Material node", PYTHON_TYPE_FINAL );

// -----------------------------------------------------------------------------
// ShinyDiffuseMaterial implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
ShinyDiffuseMaterial::ShinyDiffuseMaterial(const char* a_sID,
    YRColorRGB& a_color, float a_fTransparency /* = 0.0f */, 
    float a_fTranslucency /* = 0.0f */, float a_fDiffuseReflect /* = 1.0f */, 
    float a_fSpecularReflect /* = 0.0f */, float a_fEmisiveness /* = 0.0f */, 
    float a_fFresnel /* = 1.33f  */, float a_fONSigma /* = 0.0f */ )
{
    YRParameterMap params;

    params[ "type"  ] = YRParameter( std::string("shinydiffusemat") );
    params[ "color" ] = YRParameter( a_color );
    params[ "transparency" ] = YRParameter( a_fTransparency );
    params[ "translucency" ] = YRParameter( a_fTranslucency );
    params[ "diffuse_reflect" ]  = YRParameter( a_fDiffuseReflect );
    params[ "specular_reflect" ] = YRParameter( a_fSpecularReflect );
    params[ "emit" ] = YRParameter( a_fEmisiveness );    

    // Fresnel params
    if( a_fFresnel > 0.0f )
    {
        params[ "IOR" ]  = YRParameter(a_fFresnel );
        params[ "fresnel_effect" ] = YRParameter(true);
    }

    // Oren-nayar
    if( a_fONSigma > 0.0f )
    {
        params["diffuse_brdf"] = YRParameter( std::string( "oren_nayar" ));
        params["sigma"]        = YRParameter( a_fONSigma );
    }

    // Create the actual material
    std::list<yafaray::paraMap_t> dummy;

    Utils::PrintMessage("Creating material [%s]", a_sID);
    GetYRMaterialRef() = RenderEnvironment::GetREObject()->createMaterial( a_sID, params, dummy );    

    if( GetYRMaterialRef() )
        SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
ShinyDiffuseMaterial::~ShinyDiffuseMaterial()
{
    Utils::PrintMessage("Deleting shiny diffuse material");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
void ShinyDiffuseMaterial::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT ShinyDiffuseMaterial::GetPyStringRep()
{
    return PyString_FromString("Shiny diffuse material"); 
}




// -----------------------------------------------------------------------------
// Python module methods implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
//
//  Expected parameters:
//      - id    String
//      - color Vector3d
//      - transparency, translucency, dffusereflect, specular reflect, emisiveness,
//        fresnel, sigma: floats.
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( materials, shinydiffuse )
{
    char* sID       = NULL;
    PYOBJECT pColor = NULL;
    float fTransparency, fTranslucency, fDifReflect, fSpecReflect, fEmisiveness,
        fFresnel, fSigma;

    // Obtain expected parameters
    if( !PyArg_ParseTuple( args, "sOfffffff", &sID, &pColor, &fTransparency, &fTranslucency,
            &fDifReflect, &fSpecReflect, &fEmisiveness, &fFresnel, &fSigma)) 
    {
        PYTHON_ERROR( "Wrong parameters. Expected id, color, transparency, translucency, diff, spec, emisiveness, fresnel, sigma" );
    }

    // Make sure our objects are of the expected kind
    if( !Vector3D::PyTypeCheck(pColor) ){
        PYTHON_ERROR( "Color should be a 3dvector" );
    }

    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );

    // Create our actual material    
    return new ShinyDiffuseMaterial( sID, color, fTransparency, fTranslucency, 
        fDifReflect, fSpecReflect, fEmisiveness, fFresnel, fSigma );
}
