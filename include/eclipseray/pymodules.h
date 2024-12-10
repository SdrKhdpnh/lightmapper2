///////////////////////////////////////////////////////////////////////////////
//
// pymodules
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
/// \file pymodules.h
/// \brief Defines python modules used to extend our internal python interpreter
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   11:38
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PYMODULES_H
#define _PYMODULES_H

#include <eclipseray/pymacros.h>

// -----------------------------------------------------------------------------
// Python modules
//
// Each one of these macros defines a set of python modules that are available
// to extend our internal interpreter. The implementation of each module will
// reside in a different file, most frequently the one that defines the related
// object. See the documentation on each module for more information.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Main application module
//
// Handles construction of root objects, and groups together all sub-libraries.
// All libraries declared afterwards are added to this one.
// Defined inside pythoninterface.cpp 
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( aergia )

// -----------------------------------------------------------------------------
// Lights
//
// Handles construction of lights
// Defined in eclight.cpp
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( lights )

// -----------------------------------------------------------------------------
// Materials
//
// Handles construction of different material types
// Defined in ecmaterial.cpp
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( materials )

// -----------------------------------------------------------------------------
// Geometry
//
// Handles construction of geometry types such as vectors, matrices, and
// quaternions. We could use lists for all these, but any function receiving them
// would have to perform type checks on all of their methods.
// Defined in ecgeometry.cpp
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( geometry )

// -----------------------------------------------------------------------------
// Surface integrators
//
// Construction of integrators of different kinds.
// Defined in ecintegrator.cpp
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( integrators )

// -----------------------------------------------------------------------------
// Errors
//
// Handles information about the current error state of the program. Allows to
// query errors and get their properties.
// Defined in utils.cpp
// -----------------------------------------------------------------------------
DECLARE_PYTHON_MODULE( errors )





#endif