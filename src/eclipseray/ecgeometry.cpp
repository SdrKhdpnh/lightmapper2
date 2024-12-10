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
/// \file ecgeometry.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   15:45
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecgeometry.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/utils.h>

// -----------------------------------------------------------------------------
// Python Geometry module definition
// -----------------------------------------------------------------------------

// Methods
PYTHON_MODULE_METHOD_VARARGS( geometry, vector3d  );
PYTHON_MODULE_METHOD_VARARGS( geometry, matrix4x4 );

// Method dictionary
START_PYTHON_MODULE_METHODS( geometry )
    ADD_MODULE_METHOD( geometry, vector3d,  "Creates a new 3D vector",  METH_VARARGS ),    
    ADD_MODULE_METHOD( geometry, matrix4x4, "Creates a new 4x4 matrix", METH_VARARGS ),
END_PYTHON_MODULE_METHODS();

// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( geometry, "Geometry" )
{
    // Create, and return, the new module
    CREATE_PYTHON_MODULE( geometry, "Core geometry library", pGeometryModule );
    return pGeometryModule;
}

// -----------------------------------------------------------------------------
// Vector3D
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
Vector3D::Vector3D()
:EclipseObject( &m_PythonType )
{

    m_components.set(0.0f,0.0f,0.0f);
    SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
Vector3D::Vector3D(float a_fX, float a_fY, float a_fZ )
:EclipseObject( &m_PythonType )
{
    m_components[0] = a_fX;
    m_components[1] = a_fY;
    m_components[2] = a_fZ;
    SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
Vector3D::Vector3D( const YRPoint3D& a_point3D )
:EclipseObject( &m_PythonType )
{
    m_components = a_point3D;
    SetIsValid(true);
}


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
void Vector3D::Normalize()
{
    float fMagnitude = sqrt(m_components[0] * m_components[0] + m_components[1] * m_components[1] +
        m_components[2] * m_components[2]);

    if( fMagnitude > 0.0f )
    {
        float fNorm = 1.0f / fMagnitude;

        m_components[0] *= fNorm;
        m_components[1] *= fNorm;
        m_components[2] *= fNorm;
    }
}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Vector3D )
    ADD_OBJECT_METHOD( Vector3D, transform ),
    ADD_OBJECT_METHOD( Vector3D, normalize ),
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Vector3D, "3D vector", "3-component vector", PYTHON_TYPE_FINAL );

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Vector3D::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Vector3D) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Vector3D::PyAsString()
{
    return PyString_FromString("3d vector");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
void Vector3D::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/8/2009
////////////////////////////////////////////////////////////////////////////////
void Vector3D::SetComponents( float* a_pValues )
{
    m_components[0] = a_pValues[0];
    m_components[1] = a_pValues[1];
    m_components[2] = a_pValues[2];
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/8/2009
//
//  Expected:
//      - (matrix) transformation matrix
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Vector3D, transform, "Transforms this vector by the provided matrix" )
{
    PYOBJECT pMatrix = NULL;

    if( !PyArg_ParseTuple( a_pArgs, "O", &pMatrix) || !Matrix::PyTypeCheck(pMatrix)){
        PYTHON_ERROR("Expected matrix object");
    }

    Vector3D* pSelf = (Vector3D*)a_pSelf;

    // Perform the multiplication, be careful on how we pass the data again to ourselves.
    Vector3D vResult = (*(Matrix*)pMatrix) * (*pSelf);
    pSelf->SetComponents( vResult.GetComponentsPtr() );

    // done
    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/8/2009
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Vector3D, normalize, "Normalizes the contained vector")
{
    ((Vector3D*)a_pSelf)->Normalize();
    return PythonReturnValue( PythonReturn_None );
}


// -----------------------------------------------------------------------------
// Matrix 
// -----------------------------------------------------------------------------

const Matrix Matrix::Identity;

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
Matrix::Matrix()
:EclipseObject(&m_PythonType )
{
    m_matrix.identity();
    SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
Matrix::Matrix( float* a_pValues )
:EclipseObject(&m_PythonType )
{
    int i = 0;
    for( int r = 0; r < 4; r++)
        for( int c = 0; c < 4; c++ )
        {
            m_matrix[r][c] = a_pValues[i];
            i++;
        }

    SetIsValid(true);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
void Matrix::SetRow( int a_nRow, float* a_pValues )
{
    for( int c = 0; c < 4; c++ )
        m_matrix[a_nRow][c] = a_pValues[c];
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
bool Matrix::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Matrix) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Matrix::PyAsString()
{
    return PyString_FromString("4x4 matrix");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
////////////////////////////////////////////////////////////////////////////////
void Matrix::DeleteObject()
{
    delete this;
}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Matrix )
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Matrix, "4x4 matrix", "A 4x4 row-major matrix", PYTHON_TYPE_FINAL );


// -----------------------------------------------------------------------------
// Python module implementation 
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
// 
// Expected params:
//      - x, y, z, all floating point numbers
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( geometry, vector3d )
{
    Vector3D* pVector3D = new Vector3D;

    if( !PyArg_ParseTuple( args, "fff", &pVector3D->x(), &pVector3D->y(), &pVector3D->z() )) 
    {
        // Attempt to parse a list
        PYOBJECT pFloatList = NULL;
        if( !PyArg_ParseTuple(args,"O",&pFloatList) || !PyList_Check(pFloatList) || (PyList_Size(pFloatList)<3) ){
            PYTHON_ERROR( "Wrong syntax: Expected three floats, or list of floats" );
        }

        // Three first components
        PYOBJECT pX = PyList_GET_ITEM( pFloatList, 0 );
        PYOBJECT pY = PyList_GET_ITEM( pFloatList, 1 );
        PYOBJECT pZ = PyList_GET_ITEM( pFloatList, 2 );

        if( !PyFloat_Check(pX) || !PyFloat_Check(pY) || !PyFloat_Check(pZ) ){
            PYTHON_ERROR("List components must be floats!");
        }

        pVector3D->x() = (float)PyFloat_AS_DOUBLE( pX );
        pVector3D->y() = (float)PyFloat_AS_DOUBLE( pY );
        pVector3D->z() = (float)PyFloat_AS_DOUBLE( pZ );

    }

    // done
    return pVector3D;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/7/2009
//
//  Expected
//      A list containing 16 floats, in row-major order.
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( geometry, matrix4x4 )
{
    PYOBJECT pListOfFloats;
    float pValues[16];

    if( !PyArg_ParseTuple(args,"O",&pListOfFloats)){
        PYTHON_ERROR("Wrong syntax: Expected list with 16 floats");
    }

    if( !PyList_Check(pListOfFloats) ){
        PYTHON_ERROR("Expected type is a list with 16 components")
    }

    // Extract components
    int nElements = PyList_Size( pListOfFloats );
    if( nElements != 16 ){
        PYTHON_ERROR("Expected 16 floats");
    }

    for( int i = 0; i < 16; i++ )
    {
        pValues[i] = (float)PyFloat_AS_DOUBLE(PyList_GET_ITEM(pListOfFloats,i));
    }

    // Create the matrix, and return
    return new Matrix(pValues);
}


