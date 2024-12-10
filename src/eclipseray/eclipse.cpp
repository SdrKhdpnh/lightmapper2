///////////////////////////////////////////////////////////////////////////////
//
// EclipseObject
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
/// \file EclipseObject
/// \brief 
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 8:12:2008   11:49
//
///////////////////////////////////////////////////////////////////////////////

#include "eclipseray/eclipse.h"

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

PyTypeObject EclipseObject::m_PythonType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "EclipseObject",
    sizeof(EclipseObject),
    0,
    PyDestructor,
    0,
    0,
    0,
    0,
    _PyStringRep,
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    0,					/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    0,					/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,					/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    EclipseObject::m_PyMethods,			/* tp_methods */
    0,					/* tp_members */
    0,			/* tp_getset */
    0,		/* tp_base */
    0,					/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    0,		/* tp_init */
    0,					/* tp_alloc */
    0,					/* tp_new */
};

// Methods for this object
PyMethodDef EclipseObject::m_PyMethods [] = {
    {NULL,NULL}
};

// String representation of our object
PYOBJECT EclipseObject::_PyStringRep(PYOBJECT a_pSelf )
{
    return ((EclipseObject*)a_pSelf)->PyAsString();
}

// Destructor for python-specific code
void EclipseObject::PyDestructor(PYOBJECT a_pSelf )
{
    ((EclipseObject*)a_pSelf)->ReleaseRef();
}

// Checks the type of the provided python object
bool EclipseObject::PyTypeCheck(PYOBJECT a_pObj )
{
    if( a_pObj->ob_type == &EclipseObject::m_PythonType )
        return true;
    else
        return false;
}

// -----------------------------------------------------------------------------
// EclipseObject implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 8:12:2008
////////////////////////////////////////////////////////////////////////////////
EclipseObject::EclipseObject(PyTypeObject* a_pPythonType )
:m_nReferenceCount(1),
 m_bValidState(false)
{
    // Python instance initialization
    ob_type = a_pPythonType;
    _Py_NewReference(this);
    PyType_Ready( a_pPythonType );

    // ...
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 8:12:2008
////////////////////////////////////////////////////////////////////////////////
EclipseObject::~EclipseObject()
{
    _Py_ForgetReference(this);
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 8:12:2008
////////////////////////////////////////////////////////////////////////////////
void EclipseObject::ReleaseRef()
{
    --m_nReferenceCount;

    // Trigger the destruction mechanism of the child
    if( m_nReferenceCount <= 0 )
    {
        DeleteObject();
    }
}

