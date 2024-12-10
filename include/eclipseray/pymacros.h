///////////////////////////////////////////////////////////////////////////////
//
// pymacros
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
/// \file pymacros.h
/// \brief Defines macros to facilitate the integration of Python with our program
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 8:12:2008   11:56
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PYMACROS_H
#define _PYMACROS_H

#include <python.h>

// -----------------------------------------------------------------------------
// Basic python types and simple definitions
// -----------------------------------------------------------------------------

typedef PyObject* PYOBJECT;

#define PYTHON_TYPE(_myClass)                       _myClass::m_PythonType
#define PYTHON_METHODS(_myClass)                    _myClass::m_PyMethods
#define PYTHON_METHOD_CALL(_myClass, _myMethod )    PyMethodCall_##_myClass##_myMethod
#define PYTHON_METHOD_STCALL(_myClass, _myMethod )  StaticPyMethodCall_##_myClass##_myMethod

#define PYTHON_MODULE_OBJECT( _myModule )           PyModName_##_myModule
#define PYTHON_MODULE_TYPE( _myModule )             PyModType_##_myModule
#define PYTHON_MODULE_METHODS( _myModule )          PyModMethods_##_myModule
#define PYTHON_MODULE_CALL( _myModule, _myMethod )  PyModCall_##_myModule##_myMethod

#define PYTHON_MODULE_INITIALIZE(_myClass)  PyModuleInitialize##_myClass()

#define PYTHON_TYPE_BASE         Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
#define PYTHON_TYPE_FINAL        Py_TPFLAGS_DEFAULT


// -----------------------------------------------------------------------------
// Every class that implements python routines must declare this header
// -----------------------------------------------------------------------------
#define DECLARE_PYTHON_HEADER \
    public: \
        static PyTypeObject     m_PythonType;  \
        static PyMethodDef      m_PyMethods[]; \
        virtual PyTypeObject*   PyGetPythonType(){ return &m_PythonType; };


// -----------------------------------------------------------------------------
// Macros for python type declarations
// -----------------------------------------------------------------------------

/*!
 *  Declares the Python type object for a given class
 *  @param _myClass Name of the class
 *  @param _className Short class name, as a string
 *  @param _classDescription Brief description of the class, as a string
 *  @param _inheritance Either PYTHON_TYPE_BASE or PYTHON_TYPE_FINAL
 */
#define DECLARE_PYTHON_TYPE( _myClass, _className, _classDescription, _inheritance )  \
    PyTypeObject PYTHON_TYPE( _myClass ) = {                            \
        PyObject_HEAD_INIT(&PyType_Type)                                \
        0,                                                              \
        _className, sizeof(_myClass), 0,                                \
        EclipseObject::PyDestructor, 0, 0, 0, 0,                        \
        EclipseObject::_PyStringRep,                                    \
        0,					/* tp_as_number */                          \
        0,					/* tp_as_sequence */                        \
        0,					/* tp_as_mapping */                         \
        0,					/* tp_hash */                               \
        0,					/* tp_call */                               \
        0,					/* tp_str */                                \
        0,					/* tp_getattro */                           \
        0,					/* tp_setattro */                           \
        0,					/* tp_as_buffer */                          \
        _inheritance,       /* tp_flags */                              \
        _classDescription,  /* tp_doc */                                \
        0,					/* tp_traverse */                           \
        0,					/* tp_clear */                              \
        0,					/* tp_richcompare */                        \
        0,					/* tp_weaklistoffset */                     \
        0,					/* tp_iter */                               \
        0,					/* tp_iternext */                           \
        PYTHON_METHODS(_myClass), /* tp_methods */                      \
        0,					/* tp_members */                            \
        0,			/* tp_getset */                                     \
        0,		/* tp_base */                                           \
        0,					/* tp_dict */                               \
        0,					/* tp_descr_get */                          \
        0,					/* tp_descr_set */                          \
        0,					/* tp_dictoffset */                         \
        0,		/* tp_init */                                           \
        0,					/* tp_alloc */                              \
        0,					/* tp_new */                                \
}                                                                      

    
/*!
*  Declares the Python type object for a given module. 
*  This macro is automatically called for you on DECLARE_PYTHON_MODULE_INITIALIZATION
*  @param _myModule Name of the module
*  @param _moduleName Short module name, as a string
*/
#define DECLARE_PYTHON_MODULE_TYPE( _myModule, _moduleName )        \
    static PyTypeObject PYTHON_MODULE_TYPE( _myModule ) = {         \
    PyObject_HEAD_INIT(&PyType_Type)                                \
    0,                                                              \
    _moduleName, sizeof(PYTHON_MODULE_OBJECT(_myModule)), 0,        \
}                           

// -------------------------------------------------------------------
// Macros for method definition
// -------------------------------------------------------------------

/*!
 *  Method declaration on the object's header file
 *  @param _className Name of the class owning the method
 *  @param _methodName Name reserved for this method
 */
#define DECLARE_PYTHON_OBJECT_METHOD( _className, _methodName )      \
    PYOBJECT PYTHON_METHOD_CALL(_className,_methodName)(PYOBJECT a_pSelf, PYOBJECT a_pArgs, PYOBJECT a_pKwds ); \
    static PYOBJECT PYTHON_METHOD_STCALL(_className,_methodName)(PYOBJECT a_pSelf, PYOBJECT a_pArgs, PYOBJECT a_pKwds){ \
    return ((_className*)a_pSelf)->PYTHON_METHOD_CALL(_className,_methodName)(a_pSelf,a_pArgs,a_pKwds); }; \
    static char _methodName##_doc[];\


/*!
 *  Method block start
 *  Begin with this, then add as many python methods as necessary, then end with
 *  END_PYTHON_OBJECT_METHODS
 */                                
#define START_PYTHON_OBJECT_METHODS( _className ) \
    PyMethodDef _className::m_PyMethods [] = {

/*! 
*  Declares one object method entry for the object dictionary
*  @param _className name of the class
*  @param _methodName name of the function as described on the DECLARE_PYTHON_OBJECT_METHOD macro
*/
#define ADD_OBJECT_METHOD(_className,_methodName )\
    {#_methodName, (PyCFunction)PYTHON_METHOD_STCALL(_className,_methodName), METH_VARARGS, _className::_methodName##_doc }

/*!
 *  Method block end
 */                                
#define END_PYTHON_OBJECT_METHODS() \
    { NULL, NULL }}

/*!
 *  Declares the actual implementation of a python instance method
 *  @param _className Name of the owning class
 *  @param _methodName Name of this particular method as described on the DECLARE_PYTHON_OBJECT_METHOD macro
 *  @param _desc A brief string describing what the function does
 */                                
#define IMPLEMENT_PYTHON_OBJECT_METHOD( _className, _methodName, _desc )    \
    char _className::_methodName##_doc[] = _desc;                           \
    PYOBJECT _className::PYTHON_METHOD_CALL(_className,_methodName)(PYOBJECT a_pSelf, PYOBJECT a_pArgs, PYOBJECT a_pKwds )

// -----------------------------------------------------------------------------
// Macros for module definition
// -----------------------------------------------------------------------------

/*!
 *  Declares necessary functions for the initialization of a Python module.
 *  This macro defines an 'empty' object that can be used to contain modular functions.
 *  If you want a module that contains members, the typedef'ed structure should
 *  define them explicitly.
 *  @param _moduleName Unique name identifier for our module
 */                                
#define DECLARE_PYTHON_MODULE( _moduleName )                        \
    extern PyTypeObject PYTHON_MODULE_TYPE( _moduleName );          \
    extern PYOBJECT     PYTHON_MODULE_INITIALIZE( _moduleName );    \
    typedef struct{ PyObject_HEAD; } PYTHON_MODULE_OBJECT(_moduleName);

/*! 
 *  Declares a python method that receives no arguments. 
 *  Its actual implementation can be defined at a different moment
 *  @param _moduleName Name of the module that contains this method
 *  @param _methodName Name of the function for this method
 */
#define PYTHON_MODULE_METHOD_NOARGS(_moduleName,_methodName)\
    static PyObject* PYTHON_MODULE_CALL(_moduleName,_methodName)(PYTHON_MODULE_OBJECT(_moduleName)* self)

/*! 
*  Declares a python method that receives arguments. 
*  Its actual implementation can be defined at a different moment
*  @param _moduleName Name of the module that contains this method
*  @param _methodName Name of the function for this method
*/
#define PYTHON_MODULE_METHOD_VARARGS(_moduleName,_methodName)\
    static PyObject* PYTHON_MODULE_CALL(_moduleName,_methodName)(PYTHON_MODULE_OBJECT(_moduleName)* self, PyObject* args, PyObject* kwds)

/*!
 *  Starts an inclussion block for adding module methods to the module's dictionary
 */
#define START_PYTHON_MODULE_METHODS(_moduleName)\
    static PyMethodDef PYTHON_MODULE_METHODS(_moduleName) [] = {

/*!
 *  Ends an inclussion block for module methods
 */
#define END_PYTHON_MODULE_METHODS()    \
    {NULL}}

/*! 
 *  Declares one module method entry for the module dictionary
 *  @param _moduleName name of the module
 *  @param _methodName name of the function as described on the PYTHON_MODULE_METHOD_* macro
 *  @param _description A string description of the method
 *  @param _methodType Either METH_VARARGS or METH_NOARGS
 */
#define ADD_MODULE_METHOD(_moduleName,_methodName,_description, _methodType )\
    {#_methodName, (PyCFunction)PYTHON_MODULE_CALL(_moduleName,_methodName), _methodType, _description }

/*!
 *  Resolves into the function call that initializes a python module. 
 *  The body of this function still has to be written by you, in which you
 *  will probably want to call CREATE_PYTHON_MODULE. The function must either
 *  return a new module object, or NULL.
 */
#define DECLARE_PYTHON_MODULE_INITIALIZATION( _moduleName, _moduleNameStr ) \
    DECLARE_PYTHON_MODULE_TYPE( _moduleName, _moduleNameStr );              \
    PYOBJECT PYTHON_MODULE_INITIALIZE( _moduleName )

/*!
 *  Creates a module and assigns it to a given object name
 *  @param _moduleName The name of the module to create
 *  @param _moduleDesc Brief string description for the module
 *  @param _newModuleObject (out) Name to use for the creation of the new module
 */
#define CREATE_PYTHON_MODULE( _moduleName, _moduleDesc, _newModuleObject )      \
    PYTHON_MODULE_TYPE( _moduleName ).ob_type = &PyType_Type;                   \
    if( PyType_Ready(&PYTHON_MODULE_TYPE( _moduleName )) < 0 ){ return NULL; }  \
    PYOBJECT _newModuleObject = Py_InitModule3( #_moduleName, PYTHON_MODULE_METHODS(_moduleName), _moduleDesc ); \
    if( !_newModuleObject ){ return NULL; }



// -----------------------------------------------------------------------------
// Other macros
// -----------------------------------------------------------------------------

/// Returns an object containing the provided string, and triggers an error event
#define PYTHON_ERROR( _string ) \
    { PyErr_SetString( PyExc_TypeError, _string ); return NULL; }

// -----------------------------------------------------------------------------
// Other utilities
// -----------------------------------------------------------------------------

/*!
 *  Adds an enumeration into the provided dictionary 
 *  @param _pyDictionary The dictionary to add the new enumeration to
 *  @param _enumValue The enumeration value
 *  @param _enumName The name to use in Python for this enumeration
 */
#define PYTHON_ADD_ENUMERATION_TO_DICTIONARY( _pyDictionary, _enumValue, _enumName )    \
    { PYOBJECT pAsInt = PyInt_FromLong( _enumValue );                                   \
      PyDict_SetItemString( _pyDictionary, _enumName, pAsInt );                         \
      Py_DECREF(pAsInt);                                                                \
    }

/*!
 *  @enum ePythonReturnValue
 *  Describes a set of predefined python return values
 */
enum ePythonReturnValue
{
    PythonReturn_True,
    PythonReturn_False,
    PythonReturn_None
};

/*!
 *  Returns a special predefined python value
 *  @param a_returnValue A member of the ePythonReturnValue enumeration
 */
inline PYOBJECT PythonReturnValue( ePythonReturnValue a_returnValue )
{
    switch( a_returnValue )
    {
    case PythonReturn_True:
        Py_INCREF( Py_True );
        return Py_True;
        break;

    case PythonReturn_False:
        Py_INCREF( Py_False );
        return Py_False;
        break;

        // ...

    default:
        Py_INCREF( Py_None );
        return Py_None;
        break;
    }
}


#endif