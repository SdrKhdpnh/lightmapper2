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
/// \file eclight.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 10:12:2008   16:13
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/eclight.h>
#include <eclipseray/pymacros.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/yrtypes.h>
#include <eclipseray/ecgeometry.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecrenderenvironment.h>

#include <core_api/scene.h>
#include <core_api/bound.h>

// -----------------------------------------------------------------------------
// Light module creation
// -----------------------------------------------------------------------------

// Methods
PYTHON_MODULE_METHOD_VARARGS( lights, point       );
PYTHON_MODULE_METHOD_VARARGS( lights, ambient     );
PYTHON_MODULE_METHOD_VARARGS( lights, spot        );
PYTHON_MODULE_METHOD_VARARGS( lights, area        );
PYTHON_MODULE_METHOD_VARARGS( lights, directional );

// Method dictionary
START_PYTHON_MODULE_METHODS( lights )
    ADD_MODULE_METHOD( lights, point,   "Creates a new point light",   METH_VARARGS ),
    ADD_MODULE_METHOD( lights, ambient, "Creates a new ambient light", METH_VARARGS ),
    ADD_MODULE_METHOD( lights, spot,    "Creates a new spot light",    METH_VARARGS ),
    ADD_MODULE_METHOD( lights, area,    "Creates a new area light",    METH_VARARGS ),
    ADD_MODULE_METHOD( lights, directional, "Creates a new directional light",    METH_VARARGS ),
END_PYTHON_MODULE_METHODS();

// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( lights, "Lights" )
{
    CREATE_PYTHON_MODULE( lights, "lighting object collection", pLightingModule );

    // Append light layer types into this module
    PYOBJECT pDictionary = PyModule_GetDict( pLightingModule );
    
    PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, yafaray::scene_t::LIGHT_LAYER_EMPTY,   "Layer_Empty"   );
    PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, yafaray::scene_t::LIGHT_LAYER_DEFAULT, "Layer_Default" );
    PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, yafaray::scene_t::LIGHT_LAYER_1,       "Layer_1"       );
    PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, yafaray::scene_t::LIGHT_LAYER_2,       "Layer_2"       );
    PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, yafaray::scene_t::LIGHT_LAYER_3,       "Layer_3"       );

    return pLightingModule;
}

// -----------------------------------------------------------------------------
// AmbientLight class definition
// -----------------------------------------------------------------------------

// These definitions are only needed here, so don't put them in yrtypes
typedef yafaray::lSample_t          YRSample;
typedef yafaray::surfacePoint_t     YRSurfacePoint;
typedef yafaray::bound_t            YRBound;

////////////////////////////////////////////////////////////////////////////
//
/// \class AmbientLightInternal
/// \author Dan Torres
/// \created 1/20/2009
/// \brief Defines a light that generically adds its contribution to everything
//
////////////////////////////////////////////////////////////////////////////
class AmbientLightInternal : public yafaray::light_t
{
public:

    // The only thing we need for an ambient light is color. Intensity is intrinsic
    AmbientLightInternal( YRColorRGB& a_color ):m_color(a_color){};
    ~AmbientLightInternal(){};

    // -----------------------------------------------------------------------------
    // Virtual interface
    // -----------------------------------------------------------------------------

    virtual void init(YRScene &scene);
    virtual YRColorRGB totalEnergy() const { return m_fWorldVolume * m_color; }
    virtual YRColorRGB emitPhoton(float s1, float s2, float s3, float s4, YRRay &ray, float &ipdf) const;
    virtual YRColorRGB emitSample(YRVector3D &wo, YRSample &s) const;

    virtual bool diracLight() const { return false; }
    virtual bool ambientLight() const { return true; }
    virtual bool illumSample(const YRSurfacePoint &sp, YRSample &s, YRRay &wi) const;
    virtual bool illuminate(const YRSurfacePoint &sp, YRColorRGB &col, YRRay &wi)const { return false; }

    static YRLight *factory(YRParameterMap &params, YRRenderEnvironment &render);

private:

    YRColorRGB      m_color;
    float           m_fWorldVolume;

};

// -----------------------------------------------------------------------------
// Light implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
Light::Light()
:EclipseObject(&m_PythonType ),
 m_nShadowSamples( 1 ),
 m_pYRLight( NULL )
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
Light::~Light()
{
    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Light::PyTypeCheck(PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Light) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Light::PyAsString()
{
    return GetPyStringRep();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
bool Light::GenerateLight( const char* a_sID, YRParameterMap& a_params )
{
    m_pYRLight = RenderEnvironment::GetREObject()->createLight( a_sID, a_params );

    if( m_pYRLight != NULL )
        return true;
    else
        return false;
}

// Declare our python object methods. 
START_PYTHON_OBJECT_METHODS( Light )
// ...
END_PYTHON_OBJECT_METHODS();


// Declare our python type
DECLARE_PYTHON_TYPE( Light, "Light", "Light object", PYTHON_TYPE_FINAL );

// -----------------------------------------------------------------------------
// Point light
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
PointLight::PointLight( const char* a_sID, const YRPoint3D& a_position, const YRColorRGB& a_color, 
    float a_fBrightness, const YRColorRGB& a_shadowColor, int a_nShadowSamples, float a_fRadius )
{
    // Create the actual point light using the provided parameters
    YRParameterMap params;

    params[ "type"  ] = YRParameter( std::string("pointlight") );
    params[ "from"  ] = YRParameter( a_position );
    params[ "color" ] = YRParameter( a_color );
    params[ "power" ] = YRParameter( a_fBrightness );
    params[ "radius"] = YRParameter( a_fRadius );

    // Currently neither shadow color nor shadow samples are supported as light
    // parameters, but keep them around for when we can use them
    ShadowSamples() = a_nShadowSamples;
    ShadowColor()   = a_shadowColor;

    // sanity check
    if( GenerateLight( a_sID, params ))
    {
        Utils::PrintMessage( "Creating point light [%s]", a_sID );
        SetIsValid( true );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
PointLight::PointLight(const char* a_sID, const YRPoint3D& a_position, 
    const YRColorRGB& a_color, float a_fBrightness, float a_fRadius )
{
    // Create the actual point light using the provided parameters
    YRParameterMap params;

    params[ "type"  ] = YRParameter( std::string("pointlight") );
    params[ "from"  ] = YRParameter( a_position );
    params[ "color" ] = YRParameter( a_color );
    params[ "power" ] = YRParameter( a_fBrightness );
    params[ "radius"] = YRParameter( a_fRadius );

    if( GenerateLight( a_sID, params ) )
    {
        Utils::PrintMessage( "Creating point light [%s]", a_sID );
        SetIsValid( true );
    }

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
PointLight::~PointLight()
{
    Utils::PrintMessage("Deleting point light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 10:12:2008
////////////////////////////////////////////////////////////////////////////////
void PointLight::DeleteObject()
{
    // This is called when the reference count is less than zero
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT PointLight::GetPyStringRep()
{
    return PyString_FromString("Point light");
}


// -----------------------------------------------------------------------------
// Area light implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/2/2009
////////////////////////////////////////////////////////////////////////////////
AreaLight::AreaLight(const char* a_sID, const YRColorRGB& a_color, const YRPoint3D& a_corner, 
    const YRPoint3D& a_point1, const YRPoint3D& a_point2, float a_fPower, int a_nSamples)
{
    YRParameterMap params;

    params[ "type"   ] = YRParameter( std::string("arealight") );
    params[ "color"  ] = YRParameter( a_color );
    params[ "corner" ] = YRParameter( a_corner );
    params[ "point1" ] = YRParameter( a_point1 );
    params[ "point2" ] = YRParameter( a_point2 );
    params[ "power"  ] = YRParameter( a_fPower );
    params[ "samples"] = YRParameter( a_nSamples );

    if( GenerateLight( a_sID, params ) )
    {
        Utils::PrintMessage("Creating area light [%s]", a_sID );
        SetIsValid( true );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/2/2009
////////////////////////////////////////////////////////////////////////////////
AreaLight::~AreaLight()
{
    Utils::PrintMessage("Deleting area light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/2/2009
////////////////////////////////////////////////////////////////////////////////
void AreaLight::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT AreaLight::GetPyStringRep()
{
    return PyString_FromString("Area light");
}


// -----------------------------------------------------------------------------
// AmbientLight implementation
// -----------------------------------------------------------------------------

bool AmbientLight::m_bFactoryRegistered = false;

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
AmbientLight::AmbientLight( const char* a_sID, YRColorRGB& a_color )
{

    // Make sure the environment knows how to create ambient lights
    if( !m_bFactoryRegistered )
    {
        RenderEnvironment::GetREObject()->registerFactory( "ambientlight", AmbientLightInternal::factory );
        m_bFactoryRegistered = true;
    }

    YRParameterMap params;

    params[ "type"  ] = YRParameter( std::string("ambientlight") );
    params[ "color" ] = YRParameter( a_color );    

    if( GenerateLight( a_sID, params ) )
    {
        SetIsValid( true );
        Utils::PrintMessage("Created ambient light [%s]", a_sID );
    }

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
AmbientLight::~AmbientLight()
{
    Utils::PrintMessage("Destroying ambient light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT AmbientLight::GetPyStringRep()
{
    return PyString_FromString("Ambient light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
void AmbientLight::DeleteObject()
{
    delete this;
}

// -----------------------------------------------------------------------------
// AmbientLightInternal implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
void AmbientLightInternal::init(YRScene &scene)
{
    YRBound sceneBounds = scene.getSceneBound();
    m_fWorldVolume      = sceneBounds.vol();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
//
//  Ambient lights are problematic because they are not easy to measure physically,
//  as they are supposed to emit light in 'all' directions, from a happy and
//  unknown point in space. We would need to know the surface normal of each light
//  target. All these functions are therefore just approximations.
//
////////////////////////////////////////////////////////////////////////////////
YRColorRGB AmbientLightInternal::emitPhoton(float s1, float s2, float s3, float s4, YRRay &ray, float &ipdf) const 
{
    // pdf is the probability distribution function for this light
    ipdf = 1.0f;
    ray.from.set(0.0f, 0.0f, 0.0f);
    ray.dir.set( 0.0f,-1.0f, 0.0f);

    return m_color;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
YRColorRGB AmbientLightInternal::emitSample(YRVector3D &wo, YRSample &s) const
{
    s.dirPdf = 1.0f;
    s.areaPdf = 1.0f;
    s.col = m_color;
    s.flags = yafaray::LIGHT_NONE;    

    return m_color;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
bool AmbientLightInternal::illumSample(const YRSurfacePoint &sp, YRSample &s, YRRay &wi) const 
{
    s.pdf = 1.0f;
    s.col = m_color;
    s.flags = yafaray::LIGHT_NONE;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
YRLight* AmbientLightInternal::factory(YRParameterMap &params, YRRenderEnvironment &render)
{
    YRColorRGB lightColor(1.0f);

    params.getParam("color",lightColor);
    return new AmbientLightInternal( lightColor );
}

// -----------------------------------------------------------------------------
// Spotlight implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
SpotLight::SpotLight( const char* a_sID, YRColorRGB a_color, float a_fConeAngle, 
                     YRPoint3D a_from, YRPoint3D a_to, float a_fPower, 
                     float a_fFalloff, float a_fRadius )
{

    YRParameterMap params;

    params[ "type"  ] = YRParameter( std::string("spotlight"));
    params[ "blend" ] = YRParameter( a_fFalloff );
    params[ "color" ] = YRParameter( a_color );
    params[ "cone_angle" ] = YRParameter( a_fConeAngle );
    params[ "from"  ] = YRParameter( a_from );
    params[ "to"    ] = YRParameter( a_to   );
    params[ "power" ] = YRParameter( a_fPower );
    params[ "radius"] = YRParameter( a_fRadius );

    if( GenerateLight(a_sID, params) )
    {
        Utils::PrintMessage("Creating spotlight [%s]", a_sID );
        SetIsValid( true );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
SpotLight::~SpotLight()
{
    Utils::PrintMessage("Destroying spot light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT SpotLight::GetPyStringRep()
{
    return PyString_FromString("Spot light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
////////////////////////////////////////////////////////////////////////////////
void SpotLight::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author James Goldman
/// \date Mar 19, 2009
////////////////////////////////////////////////////////////////////////////////
DirectionalLight::DirectionalLight(const char* a_sID, const YRColorRGB& a_color, 
    float a_fBrightness, bool a_bInfinite, const YRPoint3D& a_vFrom, float a_fRadius)
{
    YRParameterMap params;

    params["type"] = YRParameter(std::string("directional"));
    params["color"] = YRParameter(a_color);
    params["power"] = YRParameter(a_fBrightness);
	params["infinite"] = YRParameter(a_bInfinite);
    params["from"] = YRParameter(a_vFrom);
    params["radius"] = YRParameter(a_fRadius);

    if (GenerateLight(a_sID, params))
    {
        Utils::PrintMessage("Creating directional light [%s]", a_sID);
        SetIsValid(true);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author James Goldman
/// \date Mar 19, 2009
////////////////////////////////////////////////////////////////////////////////
DirectionalLight::~DirectionalLight()
{
    Utils::PrintMessage("Deleting directional light");
}

////////////////////////////////////////////////////////////////////////////////
/// \author James Goldman
/// \date Mar 19, 2009
////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::DeleteObject()
{
    // This is called when the reference count is less than zero
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author James Goldman
/// \date Mar 19, 2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT DirectionalLight::GetPyStringRep()
{
    return PyString_FromFormat("Directional light instance at <%x>", this);
}

// -----------------------------------------------------------------------------
// Python module implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
//
// Expected parameters:
//      - id                    string
//      - position              Vector3D
//      - color                 Vector3D
//      - brightness            float
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( lights, point )
{
    char* sID          = NULL;    
    PYOBJECT pPosition = NULL;
    PYOBJECT pColor    = NULL;
    float fBrightness  = 0.0f;
    float fRadius      = -1.0f;

    // Obtain all necessary parameters
    if( !PyArg_ParseTuple( args, "sOOff", &sID, &pPosition, &pColor, &fBrightness, &fRadius )) {
        PYTHON_ERROR( "Wrong parameters. Expected <id>, <environment>, <position>, <color>, <brightness>, <radius>" );
    }

    // Type checks
    if( !Vector3D::PyTypeCheck(pPosition) || !Vector3D::PyTypeCheck(pColor) ) {
        PYTHON_ERROR( "Wrong types. Position and color are vectors" );
    }

    // Create the actual light
    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );
    PointLight* pNewLight = new PointLight( sID, ((Vector3D*)pPosition)->AsYRPoint3D(), color, fBrightness, fRadius );
    return pNewLight;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
//
//  Expected:
//      - id            string
//      - color         Vector3D
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( lights, ambient )
{
    char* sID = NULL;
    PYOBJECT pColor = NULL;

    // Obtain parameters
    if( !PyArg_ParseTuple(args, "sO", &sID, &pColor) || !Vector3D::PyTypeCheck(pColor) ){
        PYTHON_ERROR("Wrong parameters: Expected id and color");
    }

    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );
    return new AmbientLight( sID, color );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
//
//  Expected:
//      - id
//      - color
//      - cone angle
//      - from
//      - to
//      - power
//      - falloff
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( lights, spot )
{
    char* sID = NULL;
    PYOBJECT pColor = NULL;
    PYOBJECT pFrom  = NULL;
    PYOBJECT pTo    = NULL;
    float fAngle, fPower, fFalloff, fRadius;

    // Parse params
    if( !PyArg_ParseTuple( args, "sOfOOfff", &sID, &pColor, &fAngle, &pFrom, &pTo, &fPower, &fFalloff, &fRadius ) ){
        PYTHON_ERROR("Wrong parameters! Check function documentation");
    }

    // Validation
    if( !Vector3D::PyTypeCheck(pColor) || !Vector3D::PyTypeCheck(pFrom) || !Vector3D::PyTypeCheck(pTo)  ){
        PYTHON_ERROR("Wrong object types: color, from, and to, are vectors");
    }

    // Creation
    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );
    return new SpotLight( sID, color, fAngle, ((Vector3D*)pFrom)->AsYRPoint3D(), ((Vector3D*)pTo)->AsYRPoint3D(), 
        fPower, fFalloff, fRadius );
    
}


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/2/2009
//
//  Expected:
//      - id
//      - position          vector3d
//      - corner 1          vector3d
//      - corner 2          vector3d
//      - color             vector3d
//      - power             float
//      - samples           int
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( lights, area )
{
    PYOBJECT pColor, pPos, pCorner1, pCorner2;
    char* sID = NULL;
    float fPower = 1.0f;
    int nSamples = 1;

    // Obtain all necessary parameters
    if( !PyArg_ParseTuple( args, "sOOOOfi", &sID, &pPos, &pCorner1, &pCorner2, &pColor, &fPower, &nSamples )) {
        PYTHON_ERROR( "Wrong parameters. Check function documentation" );
    }

    // Make sure the environment is really an environment, and the vectors are really vectors
    if( !Vector3D::PyTypeCheck(pColor)   || !Vector3D::PyTypeCheck(pPos) || 
        !Vector3D::PyTypeCheck(pCorner1) || !Vector3D::PyTypeCheck(pCorner2)) {
        PYTHON_ERROR( "Wrong types. Position and color are vectors" );
    }

    // Color
    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );

    // light
    return new AreaLight( sID, color,((Vector3D*)pPos)->AsYRPoint3D(),
        ((Vector3D*)pCorner1)->AsYRPoint3D(), ((Vector3D*)pCorner2)->AsYRPoint3D(), 
        fPower, nSamples );
}

////////////////////////////////////////////////////////////////////////////////
/// \author James Goldman
/// \date Mar 19, 2009
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS(lights, directional)
{
    PYOBJECT pColour, pFrom;
    char* sID = NULL;
    float fPower = 1.0f;
    float fRadius = -1.0f;

    if (!PyArg_ParseTuple(args, "sOf|Of", &sID, &pColour, &fPower, &pFrom, &fRadius)) {
        PYTHON_ERROR ("Directional light: expected <id> <colour> <power> [<from>, <radius>]");
    }

    if (!Vector3D::PyTypeCheck(pColour))
    {
        PYTHON_ERROR("Wrong type: <colour> must be a vector.");
    }

    bool bInfinite = fRadius < 0.0f;
    if (!bInfinite && !Vector3D::PyTypeCheck(pFrom))
    {
        PYTHON_ERROR("Wrong type: <from> must be a vector.");
    }

    YRColorRGB colour (((Vector3D*)pColour)->GetComponentsPtr());

    return new DirectionalLight(sID, colour, fPower, bInfinite, ((Vector3D*)pFrom)->AsYRPoint3D(), fRadius);
}

