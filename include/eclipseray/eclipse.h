///////////////////////////////////////////////////////////////////////////////
//
// eclipse.h
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
/// \file eclipse.h
/// \brief Defines base objects for interacting with the Eclipse engine
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 8:12:2008   11:31
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ECLIPSE_H
#define _ECLIPSE_H

#include <python.h>
#include "eclipseray/pymacros.h"

// -----------------------------------------------------------------------------
// General definitions and macros
// -----------------------------------------------------------------------------

/// Convenient definition for pure virtual calls
#define ECLIPSE_PURE = 0

////////////////////////////////////////////////////////////////////////////
//
/// \class EclipseObject
/// \author Dan Torres
/// \created 2008/12/08
/// \brief Defines a base interface for all objects coming from Eclipse
//
/// All objects generated from data provided by the eclipse toolset derive
/// from this common interface. Here you can conveniently add utilities and
/// functionality that all of them should inherit.
/// 
/// This class also provides functionality to expose all of its descendants
/// as Python objects, as well as reference-counted automatic destruction.
/// 
/// Children classes must, at least, implement the following:
///     PyAsString
///     DeleteObject
///         
/// Optionally, children can implement a static PyTypeCheck call, and compare
/// the provided PyObject against their own m_PythonType static member.
/// 
/// Concrete implementations MUST set the validity flag of the class to true
/// when they are sucessfully created. Use SetIsValid for this
//
////////////////////////////////////////////////////////////////////////////
class EclipseObject : public PyObject
{
    DECLARE_PYTHON_HEADER;

public:

    // -------------------------------------------------------------------------
    // Functions and utilities
    // -------------------------------------------------------------------------

    /// Destructor
    virtual ~EclipseObject();

    /// Increases this object's reference count
    inline void AddRef(){ m_nReferenceCount++; }

    /// Decreases this object's reference count
    void ReleaseRef();

	/// Retuns true if the object is in a valid state
	inline bool IsValid() const { return m_bValidState; }

    // -------------------------------------------------------------------------
    // Python interface
    //
    // All functions on this section must be re-implemented by the child class
    // -------------------------------------------------------------------------

    /*!
     *  Python text representation method
     *  This function must be implemented by all children
     *  @return A python string object with a description of ourselves 
     */
    virtual PYOBJECT PyAsString() ECLIPSE_PURE;

    // -------------------------------------------------------------------------
    // Static python interface
    //
    // Functions on this section stay here. 
    // -------------------------------------------------------------------------

    /*!
    *   Python-specific destructor
    *   Used by the Python environment to destroy EclipseObject instances from
    *   within python scripts
    */
    static void PyDestructor( PYOBJECT a_pSelf );

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates an EclipseObject
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
     *  Static interface used by actual python scripts to access this class on string form
     *  This function resolves to PyAsString on the inherited class
     */
    static PYOBJECT _PyStringRep( PYOBJECT a_pSelf );


protected:

    /// Only derived classes can instantiate objects
    EclipseObject( PyTypeObject* a_pPythonType );

    /*!
     *  Provides children with access to our reference count number
     *  @return Our current reference count
     */
    inline int GetRefCount(){ return m_nReferenceCount; }

	/*!
	 *	Sets the validity flag for an object's concrete implementation
	 *	@param a_bIsValid True if the instance is a valid object
	 */
	inline void SetIsValid( bool a_bIsValid ){ m_bValidState = a_bIsValid; }

    /*!
     *  Child message hook for reference-count based destruction.
     *  The child class is responsible for deleting any instance to which this
     *  this function is called.
     */
    virtual void DeleteObject() ECLIPSE_PURE;

private:

    int             m_nReferenceCount;  ///< Reference count for this object
	bool			m_bValidState;		///< True if the concrete object is valid

};




#endif