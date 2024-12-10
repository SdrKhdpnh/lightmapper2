///////////////////////////////////////////////////////////////////////////////
//
// pythoninterface
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
/// \file pythoninterface.h
/// \brief Contains main interfaces for embedding python into the application
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 9:12:2008   11:01
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PYTHONINTERFACE_H
#define _PYTHONINTERFACE_H

#include <python.h>
#include <eclipseray/pymacros.h>
#include "settings.h"


////////////////////////////////////////////////////////////////////////////
//
/// \class PythonInterface
/// \author Dan Torres
/// \created 2008/12/09
/// \brief Provides services for embedding Python
//
/// This static interface provides basic services for embedding Python 
/// into our application, running simple Python commands, and loading
/// and executing Python scripts.
/// 
/// To use, call Start at the beginning of your application. Use any of
/// the embedding functions, then call Stop before terminating the application.
//
////////////////////////////////////////////////////////////////////////////
class PythonInterface
{
public:

    // -------------------------------------------------------------------------
    // Maintenance
    // -------------------------------------------------------------------------

    /*!
     *  Starts up the Python services
     *  This function must be called BEFORE using any of the other Python services.
     *  @param a_appSettings Parameters used on our application
     *  @return True if all Python services are correctly initialized.
     */
    static bool Start( int argc, char *argv[] );

    /*!
     *  Stops the Python services
     *  This function must be called BEFORE finishing the application
     *  @return True if all Python services could be correctly stopped.
     */
    static bool Stop();

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *  Executes one line of Python code
     *  Interprets and executes one line of python code. This function is equivalent
     *  to Python's interactive loop. A pointer to the resulting
     *  PyObject instance is provided. This result can also be NULL or Py_None.
     *  @param a_sCode A null-terminated string with Python conde
     *  @return A Python object with the result of the requested code, NULL on error
     */
    static PYOBJECT Run( const char* a_sCode );

    /*!
     *  Executes the code contained on a file in disk
     *  Interprets and executes the code contained in a file identified by the
     *  provided filename. A pointer to the resulting PyObject instance is provided.
     *  @param a_sFilename Name of the file to execute
     *  @return A Python object with the result of the script. Null on error.
     */
    static PYOBJECT RunFile( const char* a_sFilename );


    /*!
     *	Executes the code contained on a file in disk, discarding the result.
     *  Interprets and executes the code contained in a given file, but discards
     *  any returned object. Use when you just want to run a script and you are
     *  not interested on what might come back from it.
     *  @param a_sFilename Name of the file to execute
     */
    static void RunSimpleFile( const char* a_sFilename );



private:

    // Check for error conditions, automatically print them if any, and return state
    static bool CheckErrors();


    static unsigned int m_nFlags;               ///< Various internal flags
    static PYOBJECT     m_pGlobalDictionary;    ///< Our main dictionary

};



#endif