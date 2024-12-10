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
/// \file ecintegrator.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 15:12:2008   10:32
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecintegrator.h>
#include <eclipseray/utils.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/ecrenderenvironment.h>
#include <eclipseray/ecgeometry.h>


// -----------------------------------------------------------------------------
// Python module declaration
// -----------------------------------------------------------------------------

// Methods
PYTHON_MODULE_METHOD_VARARGS( integrators, directlight      );
PYTHON_MODULE_METHOD_VARARGS( integrators, photon           );
PYTHON_MODULE_METHOD_VARARGS( integrators, ambientocclusion );
// ...

// Method dictionary
START_PYTHON_MODULE_METHODS( integrators )
    ADD_MODULE_METHOD( integrators, directlight,      "Creates a direct lighting integrator",    METH_VARARGS ),
    ADD_MODULE_METHOD( integrators, photon,           "Creates a photon mapping integrator",     METH_VARARGS ),
    ADD_MODULE_METHOD( integrators, ambientocclusion, "Craates an ambient occlusion integrator", METH_VARARGS ),
    // ...
END_PYTHON_MODULE_METHODS();

// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( integrators, "Integrators" )
{
    CREATE_PYTHON_MODULE( integrators, "Surface integrators", pIntegratorsModule );
    return pIntegratorsModule;
}


// -----------------------------------------------------------------------------
// SurfaceIntegrator implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
SurfaceIntegrator::SurfaceIntegrator()
:EclipseObject( &m_PythonType ), 
 m_pIntegrator( NULL )
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
SurfaceIntegrator::~SurfaceIntegrator()
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
bool SurfaceIntegrator::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(SurfaceIntegrator) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT SurfaceIntegrator::PyAsString()
{
    return GetPyStringRep();
}

// -----------------------------------------------------------------------------
// SurfaceIntegrator python stuff
// -----------------------------------------------------------------------------
START_PYTHON_OBJECT_METHODS(SurfaceIntegrator)
    // ...
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( SurfaceIntegrator, "Surface integrator", "Integrator object", PYTHON_TYPE_FINAL );

// -----------------------------------------------------------------------------
// DirectLightingIntegrator implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
DirectLightingIntegrator::DirectLightingIntegrator(const char* a_sID, int a_nRayDepth, 
    int a_nPhotons, int a_nTransparentShadowDepth, bool a_bMonochrome )
{
    YRParameterMap params;

    params[ "type"     ] = YRParameter( std::string("directlighting") );
    params[ "raydepth" ] = YRParameter( a_nRayDepth );
    params[ "photons"  ] = YRParameter( a_nPhotons );
    params[ "monochrome" ] = YRParameter( a_bMonochrome );

    if( a_nTransparentShadowDepth > 0 )
    {
        params[ "transpShad"  ] = YRParameter( true );
        params[ "shadowDepth" ] = YRParameter( a_nTransparentShadowDepth );
    }

    GetYRIntegrator() = (YRSurfaceIntegrator*)RenderEnvironment::GetREObject()->createIntegrator( a_sID, params );

    if( GetYRIntegrator() )
    {
        Utils::PrintMessage( "Creating direct lighting integrator [%s] ", a_sID );
        SetIsValid( true );
    }
    else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Params, this, 
            "Failed to create direct light integrator [%s]", a_sID );
    }

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
DirectLightingIntegrator::~DirectLightingIntegrator()
{
    Utils::PrintMessage("Deleting direct lighting integrator");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
void DirectLightingIntegrator::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT DirectLightingIntegrator::GetPyStringRep()
{
    return PyString_FromString("Direct lighting integrator");
}


// -----------------------------------------------------------------------------
// Photon integrator
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/30/2008
////////////////////////////////////////////////////////////////////////////////
PhotonIntegrator::PhotonIntegrator( const char* a_sID, 
                                   int a_nRayDepth, int a_nShadowDepth, int a_nPhotons, 
                                   float a_fDiffuseRadius, int a_nSearch, 
                                   int a_nCausticMix, int a_nBounces, 
                                   bool a_bUseBackground, 
                                   int a_nFGSamples, int a_nFGBounces )
{
    YRParameterMap params;

    params[ "type"          ] = YRParameter( std::string("photonmapping") );
    params[ "raydepth"      ] = YRParameter( a_nRayDepth );

    if( a_nShadowDepth )
    {
        params[ "transpShad"  ] = YRParameter( true );
        params[ "shadowDepth" ] = YRParameter( a_nShadowDepth );
    }

    params[ "photons"        ] = YRParameter( a_nPhotons );
    params[ "diffuseRadius"  ] = YRParameter( a_fDiffuseRadius );
    params[ "search"         ] = YRParameter( a_nSearch );
    params[ "caustic_mix"    ] = YRParameter( a_nCausticMix );
    params[ "bounces"        ] = YRParameter( a_nBounces );
    params[ "use_background" ] = YRParameter( a_bUseBackground );
    params[ "finalGather"    ] = YRParameter( true );
    params[ "fg_samples"     ] = YRParameter( a_nFGSamples );
    params[ "fg_bounces"     ] = YRParameter( a_nFGBounces );

    GetYRIntegrator() = (YRSurfaceIntegrator*)RenderEnvironment::GetREObject()->createIntegrator( a_sID, params );

    if( GetYRIntegrator() )
    {
        Utils::PrintMessage( "Creating photon integrator [%s] ", a_sID );
        SetIsValid( true );
    }
    else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Params, this, 
            "Failed to create photon integrator [%s]", a_sID );
    }

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/30/2008
////////////////////////////////////////////////////////////////////////////////
PhotonIntegrator::~PhotonIntegrator()
{
    Utils::PrintMessage("Deleting photon integrator");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/30/2008
////////////////////////////////////////////////////////////////////////////////
void PhotonIntegrator::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT PhotonIntegrator::GetPyStringRep()
{
    return PyString_FromString("Photon integrator");
}

// -----------------------------------------------------------------------------
// Ambient occlusion integrator
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/12/2009
////////////////////////////////////////////////////////////////////////////////
AmbientOcclusionIntegrator::AmbientOcclusionIntegrator( const char* a_sID, int a_nSamples, 
    float a_fDistance, YRColorRGB a_color )
{
    YRParameterMap params;

    params[ "type"        ] = YRParameter( std::string("directlighting") );
    params[ "do_AO"       ] = YRParameter( true );
    params[ "AO_samples"  ] = YRParameter( a_nSamples );
    params[ "AO_distance" ] = YRParameter( a_fDistance );
    params[ "AO_color"    ] = YRParameter( a_color );

    GetYRIntegrator() = (YRSurfaceIntegrator*)RenderEnvironment::GetREObject()->createIntegrator( a_sID, params );

    if( GetYRIntegrator() )
    {
        Utils::PrintMessage( "Creating ambient occlusion integrator [%s] ", a_sID );
        SetIsValid( true );
    }
    else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Params, this, 
            "Failed to create ambient occlusion integrator [%s]", a_sID );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/12/2009
////////////////////////////////////////////////////////////////////////////////
AmbientOcclusionIntegrator::~AmbientOcclusionIntegrator()
{
    Utils::PrintMessage("Deleting ambient occlusion integrator");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT AmbientOcclusionIntegrator::GetPyStringRep()
{
    return PyString_FromString("Ambient occlusion integrator");
}

// -----------------------------------------------------------------------------
// Python SurfaceIntegrator interface
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
//
//  Expected parameters
//      - (string)id
//      - (int)rayDepth 
//      - (int)photons
//      - (int)transparent shadow depth
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( integrators, directlight )
{
    char* sID = NULL;
    int nRayDepth, nPhotons, nTShadowDepth, nMonochrome;

    // Parse parameters
    if( !PyArg_ParseTuple( args, "siiii", &sID, &nRayDepth, &nPhotons, &nTShadowDepth, &nMonochrome ) ){
        PYTHON_ERROR( "Wrong parameters. Expected <id> <int> <int> <int>" );
    }

    // Create the new object, and return
    return new DirectLightingIntegrator( sID, nRayDepth, nPhotons, nTShadowDepth, nMonochrome != 0 );

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/30/2008
//
//  Expected parameters
//      - (string) id
//      - (int) rayDepth, shadowDepth, photons
//      - (float) diffuse radius
//      - (int) search, caustic_mix, bounces
//      - (int 0/1) use background
//      - (int) fg_samples, fg_bounces
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( integrators, photon )
{
    char* sID = NULL;
    int nRayDepth, nShadowDepth, nPhotons, nSearch, nCausticMix, nBounces, nFGSamp, nFGBounces;
    float fDiffuseRadius;
    int nUseBackground;

    // Parse (a lot of) arguments
    if( !PyArg_ParseTuple(args,"siiifiiiiii",&sID,&nRayDepth,&nShadowDepth,&nPhotons,
        &fDiffuseRadius,&nSearch,&nCausticMix,&nBounces,&nUseBackground,&nFGSamp,&nFGBounces))
    {
        PYTHON_ERROR("Wrong number of parameters. Check function documentation");
    }

    // Create the new object, and return
    return new PhotonIntegrator( sID, nRayDepth,nShadowDepth, nPhotons, 
        fDiffuseRadius,nSearch,nCausticMix,nBounces, (nUseBackground==1)?true:false,
        nFGSamp,nFGBounces);

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/12/2009
//
//  Expected parameters
//      - (string) id
//      - (int) number of samples
//      - (float) sample length
//      - (3dvector) sample color
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( integrators, ambientocclusion )
{
    char* sID = NULL;
    PYOBJECT pColor = NULL;
    int nSamples = 0;
    float fLength = 0.0f;

    // parse required arguments
    if( !PyArg_ParseTuple(args,"sifO", &sID, &nSamples, &fLength, &pColor) || 
        !Vector3D::PyTypeCheck(pColor) ){
        PYTHON_ERROR("Expected: id, samples, length, and color. Color should be a 3dvector object");
    }

    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );

    // Create and return the new integrator
    return new AmbientOcclusionIntegrator( sID, nSamples, fLength, color );
}
