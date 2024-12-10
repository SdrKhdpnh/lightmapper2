///////////////////////////////////////////////////////////////////////////////
//
// ecscene
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
/// \file ecscene.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 12:12:2008   14:30
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecscene.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecintegrator.h>
#include <eclipseray/eclight.h>
#include <eclipseray/ecrenderenvironment.h>
#include <eclipseray/settings.h>
#include <eclipseray/ecfilm.h>
#include <eclipseray/eccamera.h>
#include <eclipseray/ecmesh.h>

#include <yafraycore/builtincameras.h>
#include <core_api/light.h>
#include <vector>

// -----------------------------------------------------------------------------
// Scene implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
Scene::Scene()
:EclipseObject( &m_PythonType ),
 m_nAASamples( 8 ),
 m_nAAPasses( 1 ),
 m_nAAIncSamples( 1 ),
 m_fAAThreshold( 0.05 ),
 m_bFirstRender(true)
{
    m_backgroundColor.set( 0.0f, 0.0f, 0.8f );

    Utils::PrintMessage( "Creating scene" );
    SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
Scene::~Scene()
{
    // Release references to the objects we still have
    while( m_lstOwnedReferences.size() > 0 )
    {
        EclipseObject* pObj = m_lstOwnedReferences.front();    
        pObj->ReleaseRef();
        m_lstOwnedReferences.pop_front();
    }

    Utils::PrintMessage( "Deleting scene" );

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Scene::PyTypeCheck(PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Scene) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Scene::PyAsString()
{
    // Simply return the kind of thing we are
    return PyString_FromString( "Scene object" );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12:12:2008
////////////////////////////////////////////////////////////////////////////////
void Scene::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
void Scene::AddIntegrator( SurfaceIntegrator* a_pIntegrator )
{
    if( a_pIntegrator->IsValid() )
    {
        m_scene.setSurfIntegrator( a_pIntegrator->GetIntegrator() );
        Utils::PrintMessage("Added surface integrator to scene");
    }
    else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, this,
            "Can't add integrator to scene: Invalid integrator" );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
void Scene::AddLight( Light* a_pLight )
{
    if( a_pLight->IsValid() )
    {
        m_scene.addLight( a_pLight->GetLight() );
        Utils::PrintMessage("Added light to scene");

        // Own a reference to this light
        a_pLight->AddRef();
        m_lstOwnedReferences.push_back( a_pLight );
    }
    else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, this,
            "Can't add light to scene: Invalid light" );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
void Scene::SetAntialias( int a_nSamples, int a_nPasses, int a_nIncSamples, double a_fThreshold )
{
    m_nAASamples    = a_nSamples;
    m_nAAPasses     = a_nPasses;
    m_nAAIncSamples = a_nIncSamples;
    m_fAAThreshold  = a_fThreshold;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
void Scene::Render( Film* a_pFilm, LightmapCamera* a_pCamera /* = NULL */ )
{
    YRCamera* pCamera = a_pCamera;
    std::vector<yafaray::light_t*> vAllLights = m_scene.getCurrentLightLayer(); // Copy
    std::vector<yafaray::light_t*>& vLights = m_scene.getCurrentLightLayer();

    if( a_pCamera == NULL )
    {
        // Create a simple camera. Yafaray cameras can't be modified once created, so
        // we must do this every time. Luckily, we are not a runtime graphics system.
        // Also, all 3D variants are expected to be points. The actual vectors are
        // obtained within the camera constructor
        pCamera = (YRCamera*) new yafaray::perspectiveCam_t(
            m_vCameraPosition.AsYRPoint3D(),    
            m_vCameraLookAt.AsYRPoint3D(),
            m_vCameraUp.AsYRPoint3D(),
            a_pFilm->GetWidth(), a_pFilm->GetHeight());
    }
    else
    {
        // Clear the currently selected lights
        vLights.clear();

        yafaray::point3d_t center;
        float radius;
        Mesh* pMesh = a_pCamera->GetMesh();
        pMesh->GetBoundingSphere(center, radius);

        //std::cout << "Mesh at: (" << center.x << "," << center.y << "," << center.z << ") radius: " << radius << "\n";
        for (std::vector<yafaray::light_t*>::iterator ix = vAllLights.begin(); ix != vAllLights.end(); ++ix)
        {
            yafaray::point3d_t lpos = (*ix)->getPosition();
            //std::cout << "Light at: (" << lpos.x << "," << lpos.y << "," << lpos.z << ") with radius: " << (*ix)->radius;
            if ((*ix)->radius < 0.0f)
            {
                // If no radius was specified, it's infinite.
                //std::cout << " was included (infinite).\n";
                vLights.push_back(*ix);
                continue;
            }

            float flen = yafaray::vector3d_t(lpos - center).length();

            if (flen < (*ix)->radius + radius)
            {
                //std::cout << " was included.\n";
                vLights.push_back(*ix);
            }
            //else std::cout << " was excluded.\n";
        }
        //std::cout << "Using " << vLights.size() << " of " << vAllLights.size() << "\n";
    }
    std::cout.flush();
    yafaray::background_t *myBack             = NULL; 
    yafaray::volumeIntegrator_t* pVIntegrator = NULL;

    // The following only needs to happen once
    if(m_bFirstRender)
    {
        // Make a background
        YRParameterMap params;
        params.clear();

        params["type"]  = YRParameter(std::string("constant"));
        params["color"] = YRParameter(m_backgroundColor);    
        myBack = RenderEnvironment::GetREObject()->createBackground("myBack", params);
        m_scene.setBackground(myBack);

        // volume integrators
        params.clear();
        params["type"] = YRParameter(std::string("EmissionIntegrator"));
        pVIntegrator   = (yafaray::volumeIntegrator_t*)RenderEnvironment::GetREObject()->createIntegrator("vInt",params);
        m_scene.setVolIntegrator(pVIntegrator);

        m_bFirstRender = false;
    }

    // Decide on a number of cores to use
    int nThreads = (Settings::GetGlobalSettings() != NULL)? 
        Settings::GetGlobalSettings()->Get( Settings::Setting_CPUCores ): 
        1;

    // configuration
    m_scene.depthChannel(true);
    m_scene.setImageFilm( a_pFilm->GetYRFilm() );
    m_scene.setCamera(pCamera);
    m_scene.setAntialiasing(m_nAASamples,m_nAAPasses,m_nAAIncSamples,m_fAAThreshold);
    m_scene.setNumThreads(nThreads);

    // Update manually. This currently doesn't need to be done. scene_t::render does it
    //if (m_bFirstRender)
    //{
    //    if( !m_scene.update() )
    //    {
    //        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, NULL, 
    //            "failed to update scene" );
    //        delete pCamera;
    //        return;
    //    }
    //    m_bFirstRender = false;
    //}

    // Render
    Utils::PrintMessage("Rendering scene...");
    if( !m_scene.render() )
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, NULL, 
            "failed to render scene" );
    }

    // If we used our own camera, deal with it
    if( a_pCamera == NULL )
    {
        delete pCamera;
    }
    else
    {
        // Restore the lights
        vLights = vAllLights;
    }

    // save the tga file:
    a_pFilm->GetYRFilm()->flush();

}

// -----------------------------------------------------------------------------
// Python object implementation
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Scene )
    ADD_OBJECT_METHOD( Scene, addObject             ),
    ADD_OBJECT_METHOD( Scene, render                ),
    ADD_OBJECT_METHOD( Scene, setAntialias          ),
    ADD_OBJECT_METHOD( Scene, setCamera             ),
    ADD_OBJECT_METHOD( Scene, setActiveLightLayer   ),
    ADD_OBJECT_METHOD( Scene, setBackgroundColor    ),
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Scene, "Scene", "A container of geometry and render components", 
    PYTHON_TYPE_FINAL );

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
//
//  This function expects any object of the following types:
//      - surface integrator
//      - light
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, addObject, "Appends an object into our scene" )
{
    PYOBJECT pObject = NULL;

    if(!PyArg_ParseTuple( a_pArgs, "O", &pObject )){
        PYTHON_ERROR( "Expected a valid object" );
    }

    Scene* pSelf = (Scene*)a_pSelf;

    // Attempt to add a surface integrator
    if( SurfaceIntegrator::PyTypeCheck( pObject ) )
    {
        pSelf->AddIntegrator( (SurfaceIntegrator*)pObject );
    }

    // or a light
    else if( Light::PyTypeCheck(pObject) )
    {
        pSelf->AddLight( (Light*)pObject );
    }

    // or...
    else
    {
        PYTHON_ERROR("Unsupported type");
    }

    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
// 
//  Expects:
//      - film object
//      - (optional) camera
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, render, "Triggers the main render procedure" )
{
    PYOBJECT pFilm   = NULL;
    PYOBJECT pCamera = NULL;
    Scene* pSelf     = (Scene*)a_pSelf;

    // We must have received the render environment
    if(!PyArg_ParseTuple( a_pArgs, "O|O", &pFilm, &pCamera )){
        PYTHON_ERROR( "Expected <film> [camera]" );
    }

    // Type check
    if( !Film::PyTypeCheck(pFilm)){
        PYTHON_ERROR("Expected a film object");
    }

    // If the camera is provided, check it as well
    if( pCamera && !LightmapCamera::PyTypeCheck( pCamera ) ){
        PYTHON_ERROR("Expected a lightmap camera object");
    }

    pSelf->Render( (Film*)pFilm, (LightmapCamera*)pCamera );
    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
//
//  Expected:
//      - (int) samples
//      - (int) passes
//      - (int) inc. samples
//      - (double) threshold
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, setAntialias, "Sets antialias parameters")
{
    Scene* pSelf = (Scene*)a_pSelf;
    int nSamples = 0;
    int nPasses  = 0;
    int nIncSamples = 0;
    double fThreshold = 0.0;
    
    // Parse our parameters
    if(!PyArg_ParseTuple(a_pArgs,"iiid",&nSamples, &nPasses, &nIncSamples, &fThreshold)){
        PYTHON_ERROR("Expected <samples> <passes> <inc. samples> <threshold>");
    }

    pSelf->SetAntialias(nSamples, nPasses, nIncSamples, fThreshold );

    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
//
//  Expected:
//      - (vec3d) position
//      - (vec3d) look at
//      - (vec3d) up
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, setCamera, "Sets perspective camera parameters")
{
    PYOBJECT pPos, pLookAt, pUp;
    Scene* pSelf = (Scene*)a_pSelf;

    // Parse arguments
    if(!PyArg_ParseTuple(a_pArgs,"OOO",&pPos,&pLookAt,&pUp)){
        PYTHON_ERROR("Expected position, look at, and up directions");
    }

    // make sure they are all vectors
    if( !Vector3D::PyTypeCheck(pPos) || !Vector3D::PyTypeCheck(pLookAt) || !Vector3D::PyTypeCheck(pUp)){
        PYTHON_ERROR("All three parameters must be vector3D objects");
    }

    // Re-assign
    pSelf->m_vCameraPosition = *(Vector3D*)pPos;
    pSelf->m_vCameraLookAt   = *(Vector3D*)pLookAt;
    pSelf->m_vCameraUp       = *(Vector3D*)pUp;

    /*
    Utils::PrintMessage("Set camera to:\n\tpos[%.2f,%.2f,%.2f]\n\tlook[%.2f,%.2f,%.2f]\n\tup[%.2f,%.2f,%.2f]",
        pSelf->m_vCameraPosition.x(),pSelf->m_vCameraPosition.y(),pSelf->m_vCameraPosition.z(),
        pSelf->m_vCameraLookAt.x(),pSelf->m_vCameraLookAt.y(),pSelf->m_vCameraLookAt.z(),
        pSelf->m_vCameraUp.x(),pSelf->m_vCameraUp.y(),pSelf->m_vCameraUp.z());
    */

    return PythonReturnValue( PythonReturn_None );        
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
//
//  Expected:
//      - (int) A light layer identifier of the type aergia.lights.Layer_*
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, setActiveLightLayer, "Sets the currently active light layer" )
{
    Scene* pSelf = (Scene*)a_pSelf;
    int newLayer;

    if( !PyArg_ParseTuple(a_pArgs,"i",&newLayer)){
        PYTHON_ERROR("Expected light layer of type aergia.lights.Layer_*");
    }

    pSelf->GetScenePtr()->setCurrentLightLayer((yafaray::scene_t::lightLayers)newLayer);
    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/20/2009
//
//  Expected:
//      - (vector3d) color to use as background
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Scene, setBackgroundColor, "Sets the background render color" )
{
    Scene* pSelf = (Scene*)a_pSelf;
    PYOBJECT pColor = NULL;

    if( !PyArg_ParseTuple(a_pArgs, "O", &pColor) || !Vector3D::PyTypeCheck(pColor)){
        PYTHON_ERROR("Expected a vector3d");
    }

    YRColorRGB color( ((Vector3D*)pColor)->GetComponentsPtr() );
    pSelf->m_backgroundColor = color;

    return PythonReturnValue( PythonReturn_None );
}
