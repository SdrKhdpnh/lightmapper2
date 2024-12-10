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
/// \file ecscene.h
/// \brief Defines the base scene object
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 12:12:2008   14:19
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ECSCENE_H
#define _ECSCENE_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>
#include <eclipseray/ecgeometry.h>
#include <eclipseray/eccamera.h>

#include <list>

class SurfaceIntegrator;
class Light;
class Film;

////////////////////////////////////////////////////////////////////////////
//
/// \class Scene
/// \author Dan Torres
/// \created 2008/12/12
/// \brief Defines a scene wrapper around YR scene
//
/// Scenes contain all the geometry and components needed to render something.
/// They are also the creators of several geometry components such as meshes,
/// vertices, etc.
/// 
/// Instead of writing a complete wrapper for the scene_t object, simply obtain
/// a pointer to it, and call whatever function is needed. 
/// 
/// The class owns our actual YR scene.
//
////////////////////////////////////////////////////////////////////////////
class Scene : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    /// Creates a new scene
	Scene();

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *  Provide a pointer to our internal scene object
     *  @return a_pScene A pointer to a scene_t object
     */
    inline YRScene* GetScenePtr(){ return &m_scene; }

    /*!
     *	Add a surface integrator
     *  @param a concrete SurfaceIntergrator object
     */
    void AddIntegrator( SurfaceIntegrator* a_pIntegrator );

    /*!
     *	Adds a light into our scene
     *  @param a_pLight A concrete Light object
     */
    void AddLight( Light* a_pLight );

    /*!
     *	Assigns antialias parameters
     *  @param a_nSamples Number of samples
     *  @param a_nPasses Number of render passes
     *  @param a_nIncSamples Number of samples for incremental passes
     *  @param a_fThreshold Antialias threshold
     */
    void SetAntialias( int a_nSamples, int a_nPasses, int a_nIncSamples, double a_fThreshold );


    /*!
     *	Triggers the main render procedure
     *  Triggers all render operations over the registered meshes, using whatever
     *  lights, materials, and surface integrators are available. If a camera is
     *  not provided, a conventional perspective camera is used instead.
     *  @param a_pFilm The film used to render this scene
     8  @param a_pCamera An optional camera for rendering. 
     */
    void Render( Film* a_pFilm, LightmapCamera* a_pCamera = NULL ); 

    // -------------------------------------------------------------------------
    // Python interface
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates our class
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
    *  Python text representation method
    *  This function must be implemented by all children
    *  @return A python string object with a description of ourselves 
    */
    virtual PYOBJECT PyAsString();

    // Adds an appendable object into our scene
    DECLARE_PYTHON_OBJECT_METHOD( Scene, addObject );

    // Triggers the render operation
    DECLARE_PYTHON_OBJECT_METHOD( Scene, render );

    // Sets antialias values
    DECLARE_PYTHON_OBJECT_METHOD( Scene, setAntialias );

    // Sets the camera values for perspective renderings
    DECLARE_PYTHON_OBJECT_METHOD( Scene, setCamera );

    // Sets the provided light layer as the active one
    DECLARE_PYTHON_OBJECT_METHOD( Scene, setActiveLightLayer );

    // Sets the background color. Must be called BEFORE rendering
    DECLARE_PYTHON_OBJECT_METHOD( Scene, setBackgroundColor );

protected:

    /// Only our Delete function can destroy a scene
    ~Scene();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

private:

    YRScene     m_scene;            ///< Our actual scene object

    int         m_nAASamples;       ///< Number of samples (for Antialiasing)
    int         m_nAAPasses;        ///< Number of passes
    int         m_nAAIncSamples;    ///< Samples for incremental passes
    double      m_fAAThreshold;     ///< Color threshold for additional AA samples

    Vector3D    m_vCameraPosition;   ///< Camera position for perspective-based renderings
    Vector3D    m_vCameraLookAt;     ///< Look at objective for perspective renderings
    Vector3D    m_vCameraUp;         ///< Up vector for perspective renderings
    float       m_fCameraAspect;     ///< Aspect ratio
    bool        m_bFirstRender;      ///< True for the first time we render a scene
    YRColorRGB  m_backgroundColor;   ///< Background color for render operations

    /// A list of objects that must be released when we are destroyed
    std::list<EclipseObject*> m_lstOwnedReferences;

};









#endif