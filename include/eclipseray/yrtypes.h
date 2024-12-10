    ///////////////////////////////////////////////////////////////////////////////
//
// yrtypes
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
/// \file yrtypes.h
/// \brief Contains definitions for all types used from Yafaray
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 10:12:2008   18:14
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _YRTYPES_H
#define _YRTYPES_H 

#include <core_api/light.h>
#include <core_api/vector3d.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/material.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>

#include <yafraycore/tga_io.h>
#include <yafraycore/meshtypes.h>

#include <core_api/matrix4.h>

// It might not be necessary to include this whole thing. But given the size of
// the project, distributing this definitions under their respective wrappers 
// doesn't buy us much anyways.

typedef yafaray::light_t             YRLight;                ///< Top light interface
typedef yafaray::color_t             YRColorRGB;             ///< RGB color
typedef yafaray::point3d_t           YRPoint3D;              ///< 3D point (not a vector)
typedef yafaray::vector3d_t          YRVector3D;             ///< 3D vector 
typedef yafaray::renderEnvironment_t YRRenderEnvironment;    ///< Render environment
typedef yafaray::parameter_t         YRParameter;            ///< Map parameter
typedef yafaray::paraMap_t           YRParameterMap;         ///< A map of parameters
typedef yafaray::scene_t             YRScene;                ///< Scene object
typedef yafaray::material_t          YRMaterial;             ///< Material object
typedef yafaray::surfaceIntegrator_t YRSurfaceIntegrator;    ///< Surface integrator object
typedef yafaray::objID_t			 YRObjectID;			 ///< Unique object id
typedef yafaray::imageFilm_t         YRFilm;                 ///< Image film
typedef yafaray::outTga_t            YRTga;                  ///< Tga texture
typedef yafaray::matrix4x4_t         YRMatrix4x4;            ///< A 4x4 matrix 
typedef yafaray::camera_t            YRCamera;               ///< A base camera type
typedef yafaray::ray_t               YRRay;                  ///< A simple ray object 
typedef yafaray::PFLOAT              YRPFloat;               ///< Float
typedef yafaray::triangleObject_t    YRTriangleObject;       ///< A trimesh
typedef yafaray::uv_t                YRuv;                   ///< UV coordinate pair



#endif