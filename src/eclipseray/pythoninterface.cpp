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
/// \file pythoninterface.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 9:12:2008   11:31
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/pythoninterface.h>
#include <eclipseray/pymacros.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/ecrenderenvironment.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecscene.h>
#include <eclipseray/ecbuffer.h>
#include <eclipseray/ecmaterial.h>
#include <eclipseray/ecmesh.h>
#include <eclipseray/ecfilm.h>
#include <eclipseray/eccamera.h>

#include <assert.h>
#include <iostream>
#include <fstream>

using namespace std;

// ----------------------------------------------------------------------------- 
// Macros and other stuff
// -----------------------------------------------------------------------------

// Main library name
#define PYDEFAULT_LIBRARYNAME   "aergia"

// -----------------------------------------------------------------------------
// Main module definition
// -----------------------------------------------------------------------------

// Method declaration
PYTHON_MODULE_METHOD_NOARGS ( aergia, ClearAll);
PYTHON_MODULE_METHOD_NOARGS ( aergia, scene         );
PYTHON_MODULE_METHOD_VARARGS( aergia, buffer        );
PYTHON_MODULE_METHOD_VARARGS( aergia, mesh          );
PYTHON_MODULE_METHOD_VARARGS( aergia, film          );
PYTHON_MODULE_METHOD_VARARGS( aergia, lightmapcam   );

// Method inclusion
START_PYTHON_MODULE_METHODS( aergia )
    ADD_MODULE_METHOD( aergia, ClearAll,    "Clear the render environment",  METH_NOARGS),
    ADD_MODULE_METHOD( aergia, scene,       "Creates a new scene object",    METH_NOARGS  ),
    ADD_MODULE_METHOD( aergia, buffer,      "Creates a new buffer object",   METH_VARARGS ),
    ADD_MODULE_METHOD( aergia, mesh,        "Creates a new mesh object",     METH_VARARGS ),
    ADD_MODULE_METHOD( aergia, film,        "Creates a new film plate",      METH_VARARGS ),
    ADD_MODULE_METHOD( aergia, lightmapcam, "Creates a lightmapping camera", METH_VARARGS ),
END_PYTHON_MODULE_METHODS();

// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( aergia, "Main raytracer module" )
{
    // Create, and return, the new module
    CREATE_PYTHON_MODULE( aergia, "Core raytracer library", pAergiaModule );
    return pAergiaModule;
}

// -----------------------------------------------------------------------------
// Local function definitions
// -----------------------------------------------------------------------------

// Initializes our exposed modules
void InitializeModules();

// -----------------------------------------------------------------------------
// Flags
// -----------------------------------------------------------------------------

#define PYFLAGS_NONE            0x00000000      // Nothing
#define PYFLAGS_INITIALIZED     0x00000001      // Python interpreter ready

// ...

// -----------------------------------------------------------------------------
// Static initialization
// -----------------------------------------------------------------------------

unsigned int    PythonInterface::m_nFlags               = PYFLAGS_NONE;
PYOBJECT        PythonInterface::m_pGlobalDictionary    = NULL;

// ...

// -----------------------------------------------------------------------------
// Interface implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
bool PythonInterface::Start( int argc, char *argv[] )
{
    if( !(m_nFlags & PYFLAGS_INITIALIZED ) )
    {
        Settings* pSettings = Settings::GetGlobalSettings();

        // Extend our main library before initializing the interpreter
        PyImport_AppendInittab( PYDEFAULT_LIBRARYNAME, InitializeModules );
        
        // Start the Python interpreter
        Py_Initialize();
        PySys_SetArgv(argc, argv);

        // Create a base dictionary
        m_pGlobalDictionary = PyDict_New();
        if( m_pGlobalDictionary )
        {
            PyDict_SetItemString( m_pGlobalDictionary, "__builtins__", PyEval_GetBuiltins() );
            PyDict_SetItemString( m_pGlobalDictionary, "__name__", PyString_FromString("__main__"));
        }
        else
            return false;

        // If required, configure output streams.
        if( pSettings && pSettings->Get( Settings::Setting_Python_OutputToFile ) )
        {
            // Need the sys module
            PYOBJECT pResult = Run( "import sys" );
            if( pResult != NULL )
            {
                Py_DECREF( pResult );
                pResult = Run( "sys.stderr=open('errors.log','w')" );
                Py_DECREF( pResult );
                pResult = Run( "sys.stdout=open('stdout.log','w')" );
                Py_DECREF( pResult );
            }
        }

        // So far so good. Check for errors
        if( CheckErrors() )
        {
            return false;
        }

        // We're done. Mark as initialized
        m_nFlags |= PYFLAGS_INITIALIZED;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
bool PythonInterface::Stop()
{
    if( m_nFlags & PYFLAGS_INITIALIZED )
    {
        // Start by fetching error states
        if( CheckErrors() )
            return false;

        // Clear our dictionary
        Py_DECREF( m_pGlobalDictionary );

        // Finalize our python interface
        Py_Finalize();

        // Done
        m_nFlags &= ~(PYFLAGS_INITIALIZED);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
bool PythonInterface::CheckErrors()
{
    if( PyErr_Occurred() )
    {
        PyErr_Print();
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT PythonInterface::Run(const char* a_sCode )
{
    // We could create a python node object, pre-compile the piece of code, and
    // run the precompiled version, but since this is likely a one time only
    // event, use a high level interface to run it
    if( a_sCode && (m_nFlags & PYFLAGS_INITIALIZED) )
    {
        PYOBJECT pResult = PyRun_String( a_sCode, Py_single_input, m_pGlobalDictionary, 
            m_pGlobalDictionary );

        if( CheckErrors() )
        {
            Py_XDECREF(pResult);
            return NULL;
        }
        else
        {
            return pResult;
        }

    }
    else
        return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT PythonInterface::RunFile(const char* a_sFilename )
{
    if( a_sFilename && (m_nFlags & PYFLAGS_INITIALIZED))
    {
        // Again, we could load and pre-compile this script, which is cool but only
        // worth doing if we were running this multiple times. For a one-time-only
        // run, its the same to just use the high-level Python interface. Start
        // by loading the whole file in one go:
        ifstream inputFile;
        inputFile.open( a_sFilename, ifstream::in );
        if( inputFile.is_open() )
        {
            // File length
            inputFile.seekg( 0, ios::end );
            int nLength = inputFile.tellg();
            inputFile.seekg( 0, ios::beg );

            // Reserve enough memory to hold this file
            char* pFileBuffer = new char[ nLength ];
            memset( pFileBuffer, 0, nLength );

            // Read the file
            inputFile.read( pFileBuffer, nLength );

            // Run as a script
            PYOBJECT pResult = PyRun_String( pFileBuffer, Py_file_input, m_pGlobalDictionary,
                m_pGlobalDictionary );

            // cleanup
            inputFile.close();
            delete [] pFileBuffer;

            // Final check, and return
            if( CheckErrors() )
            {
                Py_XDECREF(pResult);
                return NULL;
            }
            else
            {
                return pResult;
            }
        }
        else
        {
            std::cout << "Failed to open script file \"" << a_sFilename << "\"\n";
        }
    }

    // Something went wrong
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/30/2008
////////////////////////////////////////////////////////////////////////////////
void PythonInterface::RunSimpleFile( const char* a_sFilename )
{
    PYOBJECT pResult = PythonInterface::RunFile( a_sFilename );
    Py_XDECREF( pResult );
}

// -----------------------------------------------------------------------------
// Module implementation
// -----------------------------------------------------------------------------

PYTHON_MODULE_METHOD_NOARGS( aergia, ClearAll )
{
    RenderEnvironment::ClearAll();
    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_NOARGS( aergia, scene )
{
    return new Scene;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
//
//  Expected params:
//      - Buffer usage (aergia.BufferUsage_*)
//      - Number of components (int)
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( aergia, buffer )
{
    // Expect two integers
    int nIntUsage, nComponentCount;
    if( !PyArg_ParseTuple( args, "ii", &nIntUsage, &nComponentCount )){
        PYTHON_ERROR( "Expected <component usage>, <component count>" );
    }

    // Cast, create, return
    return new Buffer( (Buffer::eBufferUsage)nIntUsage, (size_t)nComponentCount );
}


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
// 
//  Expected params:
//      - (string) id, unique
//      - (Buffer) Vertex buffer
//      - (int) Vertex buffer offset (zero-based, number of vertices to skip)
//      - (int) Vertex count
//      - (Buffer) UV buffer
//      - (int) uv offset (zero-based, number of uv's to skip)
//      - (int) uv count
//      - (Buffer) Index buffer
//      - (int) Index offset (zero-based, number of index duples to skip)
//      - (int) Triangle count
//      - (matrix) world matrix for the mesh
//      - (Scene) scene
//      - (Material) Material for the mesh
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( aergia, mesh )
{
    char* sID = NULL;
    PYOBJECT pVBuffer = NULL;
    PYOBJECT pIBuffer = NULL;
    PYOBJECT pTBuffer = NULL;
    PYOBJECT pScene   = NULL;
    PYOBJECT pMat     = NULL;
    PYOBJECT pMatrix  = NULL;
    PYOBJECT pBSphere = NULL;
    int nVertexOffset, nVertexCount,nUVOffset,nUVCount,nIndexOffset,nTriangleCount;

    // Obtain all necessary parameters
    if( !PyArg_ParseTuple( args, "sOiiOiiOiiOOOO", &sID, 
        &pVBuffer, &nVertexOffset,&nVertexCount, 
        &pTBuffer, &nUVOffset,    &nUVCount,
        &pIBuffer, &nIndexOffset, &nTriangleCount, &pMatrix, &pBSphere, &pScene, &pMat )) {
            PYTHON_ERROR( "Wrong parameters. Check function documentation" );
    }

    float sp[4];
    if (!PyArg_ParseTuple( pBSphere, "ffff", &sp[0], &sp[1], &sp[2], &sp[3]))
    {
        PYTHON_ERROR("Wrong parameters: could not parse bounding sphere.");
    }

    // Type checks
    if( !Buffer::PyTypeCheck(pVBuffer) || !Buffer::PyTypeCheck(pIBuffer) || !Buffer::PyTypeCheck(pTBuffer) ||
        !Scene::PyTypeCheck(pScene) || !Material::PyTypeCheck(pMat) || !Matrix::PyTypeCheck(pMatrix))
    {
        PYTHON_ERROR( "One or more parameters are of the wrong type!" );
    }

    // All is well. Create new mesh
    Mesh* pNewMesh = new Mesh( sID, 
        (Buffer*)pVBuffer, nVertexOffset, nVertexCount, 
        (Buffer*)pTBuffer, nUVOffset,     nUVCount,
        (Buffer*)pIBuffer, nIndexOffset,  nTriangleCount,
        (Scene*)pScene, (Material*)pMat, sp, (Matrix*)pMatrix);

    return pNewMesh;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
//
//  Expects:
//      - (string) id
//      - (int) width
//      - (int) height
//      - (eFilmType, via aergia.FilmFilterType_*) type
//      - (float) filter size
//      - (float) gamma
//      - (1/0) has depth
//      - (1/0) clamp color ranges
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( aergia, film )
{
    char* sID = NULL;
    int nWidth, nHeight, nType,nDepth,nClamp;
    float fFilterSize, fGamma;

    // Parameters
    if( !PyArg_ParseTuple( args, "siiiffii", &sID, &nWidth, &nHeight, 
        &nType, &fFilterSize, &fGamma, &nDepth, &nClamp) ){
            PYTHON_ERROR("Wrong number or type of parameters on film creation call. Check documentation");
    }

    // Create the new piece of film
    Film* pNewFilm = new Film( sID, nWidth, nHeight, (Film::eFilmFilterType)nType,
        fFilterSize, fGamma, (nDepth == 1)?true:false, (nClamp == 1)?true: false);

    return pNewFilm;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/13/2009
//
//  Expects:
//      - (film) desired film
//      - (mesh) mesh to lightmap
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_VARARGS( aergia, lightmapcam )
{
    PYOBJECT pFilm, pMesh;
    if( !PyArg_ParseTuple( args, "OO", &pFilm, &pMesh) || !Mesh::PyTypeCheck(pMesh) || !Film::PyTypeCheck( pFilm ) ){
        PYTHON_ERROR("Expected: <film object> <mesh object>");
    }

    return new LightmapCamera( (Film*)pFilm, (Mesh*)pMesh );

}

// -----------------------------------------------------------------------------
// Module initialization
// -----------------------------------------------------------------------------


// This handy macro links the initialization function of a module into its
// own name in the provided dictionary, effectively chaining the first to the second
#define APPEND_PYTHON_MODULE( _moduleName, _dictionary )  \
    PyDict_SetItemString( _dictionary, #_moduleName, PYTHON_MODULE_INITIALIZE(_moduleName) )

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
///
/// Initializes all the modules exposed by the system into the Python environment
///
////////////////////////////////////////////////////////////////////////////////
void InitializeModules()
{
    // Start by creating our root application module
    PYOBJECT pMainModule = PYTHON_MODULE_INITIALIZE( aergia );

    if( pMainModule )
    {
        // Grab the dictionary from this module
        PYOBJECT pMainDictionary = PyModule_GetDict( pMainModule );

        // Start all other modules, and append them to our main one
        APPEND_PYTHON_MODULE( lights,      pMainDictionary );
        APPEND_PYTHON_MODULE( materials,   pMainDictionary );
        APPEND_PYTHON_MODULE( geometry,    pMainDictionary );
        APPEND_PYTHON_MODULE( integrators, pMainDictionary );
        APPEND_PYTHON_MODULE( errors,      pMainDictionary );

        // ...

        // Append several constant types to our main dictionary
        Buffer::AppendBufferUsageTypes( pMainModule );
        Film::AppendFilmFilterTypes( pMainModule );

        Utils::PrintMessage("Initialized python modules");
    }
    else
    {
        Utils::PrintMessage( "Failed to initialize modules" );
    }    
}