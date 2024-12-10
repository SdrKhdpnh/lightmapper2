///////////////////////////////////////////////////////////////////////////////
//
// ecgeometry
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
/// \file ecgeometry.h
/// \brief Defines and implements several geometric objects and numerical types
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   15:39
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ECGEOMETRY_H
#define _ECGEOMETRY_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class Vector3D
/// \author Dan Torres
/// \created 2008/12/11
/// \brief A vector composed of 3 floating point values
//
/// This object is mainly intended for in-Python script usage, since its faster
/// to receive this and directly access its members, than validate python 
/// objects from lists.
//
////////////////////////////////////////////////////////////////////////////
class Vector3D : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    // -------------------------------------------------------------------------
    // Creation
    // -------------------------------------------------------------------------

    /*!
     *	Default
     */
	Vector3D();

    /*!
     *	By component
     */
    Vector3D( float a_fX, float a_fY, float a_fZ );

    /*!
     *	By copy from a yafaray point
     */
    Vector3D( const YRPoint3D& a_point3D );

    /// Default 
    ~Vector3D(){};

    /*!
     *	Normalize
     *  Normalizes the components of this vector
     */
    void Normalize();

    // -------------------------------------------------------------------------
    // Python
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

    // Transforms this vertex by the provided matrix
    DECLARE_PYTHON_OBJECT_METHOD( Vector3D, transform );

    // normalizes this vector
    DECLARE_PYTHON_OBJECT_METHOD( Vector3D, normalize );

    // -------------------------------------------------------------------------
    // Utils
    // -------------------------------------------------------------------------

    /*!
     *	Access all of our components as an array
     */
    inline float* GetComponentsPtr(){ return &m_components[0]; }

    /*!
     *	Set all of  our values by passing a pointer to a 3-float array
     *  @param a_pValues An array of at least 3 floating point values
     */
    void SetComponents( float* a_pValues );

    /*!
     *	Return the x component
     */
    inline float& x(){ return m_components[0]; }

    /*!
     *	Return the y component
     */
    inline float& y(){ return m_components[1]; }

    /*!
     *	Return the z component
     */
    inline float& z(){ return m_components[2]; }

    /*!
     *	Our contents as YRPoint
     */
    inline const YRPoint3D& AsYRPoint3D() const { return m_components; }

protected:

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();


private:

    YRPoint3D m_components;  ///< Our 3 components

};


////////////////////////////////////////////////////////////////////////////
//
/// \class Matrix
/// \author Dan Torres
/// \created 1/7/2009
/// \brief An affine 4x4 matrix
//
////////////////////////////////////////////////////////////////////////////
class Matrix : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    // -------------------------------------------------------------------------
    // Creation
    // -------------------------------------------------------------------------

    /*!
     *	Default constructor
     *  Sets this matrix to identity
     */
    Matrix();

    /*!
     *	Floating array constructor
     *  @param a_pValues an array of 16 floats to populate this matrix with. Row major.
     */
	Matrix( float* a_pValues );

    /*!
     *	Destruction
     */
    ~Matrix(){};

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *	Sets the values of a given row
     *  @param a_nRow Zero based row index
     *  @param a_pValues An array of four floats
     */
    void SetRow( int a_nRow, float* a_pValues );

    /*!
     *	Provides access to our YRMatrix4x4 internal object
     */
    inline const YRMatrix4x4& AsYRMatrix() const { return m_matrix; }

    /*!
     *	Identity matrix
     */
    static const Matrix Identity;

    // -------------------------------------------------------------------------
    // Python
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

protected:

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();

private:

    YRMatrix4x4     m_matrix;       ///< Our 4x4 matrix

};

/*!
 *	Transforms a vector by a matrix
 *  @param a_matrix A Matrix object
 *  @param a_vector A Vector3D object
 *  @return A new vector resulting from multiplying M x V (right-side vector multiplication)
 */
inline Vector3D operator * ( const Matrix& a_matrix, const Vector3D& a_vector )
{
    return a_matrix.AsYRMatrix() * a_vector.AsYRPoint3D();
}


#endif