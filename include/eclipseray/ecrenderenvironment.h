///////////////////////////////////////////////////////////////////////////////
//
// renderenvironment
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
/// \file renderenvironment.h
/// \brief Defines the render environment object, and related utilities
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   11:04
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RENDERENVIRONMENT_H
#define _RENDERENVIRONMENT_H

#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class RenderEnvironment
/// \author Dan Torres
/// \created 2008/12/11
/// \brief A wrapper around YR's render environment object
//
/// This class provides a convenient wrapper around Yafray's render environment
/// and presents it as a singleton. In Eclipseray, we only need one environment
/// to create all needed materials, lights, integrators, etc.
/// 
/// Although the RenderEnvironment class will provide tools to simplify 
/// some tasks, we are not reimplementing YR's interface, so for any special
/// operations, act over our internal render environment object by calling
/// GetREObject.
/// 
/// Render environments own the following objects: Lights, materials, textures,
/// cameras, integrators, backgrounds, and volume tables. 
/// 
/// Our RenderEnvironment object is the first to be created, and the last to be
/// destroyed.
/// 
//  NOTES:
//      - RenderEnvironment objects used to be python objects (EclipseObject-derived)
//        as well, but it was simpler to keep one single environment as a singleton
//        since they are really only used to create other object types.
//
////////////////////////////////////////////////////////////////////////////
class RenderEnvironment
{    

public:

    // -------------------------------------------------------------------------
    // Initialization and shutdown
    // -------------------------------------------------------------------------

    /*!
     *	Start the environment object. 
     *  Call first thing before creating any other objects
     */
    static void Start();

    /*!
     *	Stops the environment object.
     *  Call right before exiting the program
     */
    static void Stop();

    static void ClearAll();

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *  Provides access to our internal environment
     *  Instead of adding functions to delegate into our internal raytracer
     *  environment, use this function to call them yourself. 
     *  @return A pointer to our internal render environment
     */
    static YRRenderEnvironment* GetREObject(){ return  m_pEnvironment; }

private:

    static YRRenderEnvironment* m_pEnvironment; ///< Our yafaray environment
    static bool                 m_bInit;        ///< True when we are initialized
};






#endif