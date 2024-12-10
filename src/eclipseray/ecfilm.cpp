///////////////////////////////////////////////////////////////////////////////
//
// ecfilm
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
/// \file ecfilm.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 1/6/2009
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecfilm.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecrenderenvironment.h>

// -----------------------------------------------------------------------------
// Film implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
Film::Film( const char* a_sID, int a_nWidth, int a_nHeight, eFilmFilterType a_nFilterType, 
           float a_fFilterSize, float a_fGamma, bool a_bHasDepth, bool a_bClamp )
:EclipseObject( &m_PythonType ),
 m_pOutput( NULL ),
 m_pFilm( NULL ),
 m_sOutputName( NULL ),
 m_nWidth( a_nWidth ),
 m_nHeight(a_nHeight)
{
    // Create the output object
    m_sOutputName = strdup( a_sID );
    m_pOutput     = new yafaray::outTga_t( a_nWidth, a_nHeight, m_sOutputName );

    // Create the film itself. Use the render environment's function to keep compatibility
    YRParameterMap params;
    params.clear();
    params[ "gamma"         ] = YRParameter( a_fGamma );
    params[ "clamp_rpg"     ] = YRParameter( a_bClamp );
    params[ "AA_pixelwidth" ] = YRParameter( a_fFilterSize );
    params[ "width"         ] = YRParameter( a_nWidth );
    params[ "height"        ] = YRParameter( a_nHeight );
    
    switch( a_nFilterType )
    {
    case FilterType_Gauss:
        params["filter_type"] = YRParameter(std::string("gauss"));
        break;

    case FilterType_Mitchell:
        params["filter_type"] = YRParameter(std::string("mitchell"));
        break;

        // ...

    default:
        params["filter_type"] = YRParameter(std::string("box"));
        break;
    };


    m_pFilm = RenderEnvironment::GetREObject()->createImageFilm( params, *m_pOutput );

    if( m_pFilm )
    {
        if( a_bHasDepth )
            m_pFilm->addChannel( "Depth" );

        SetIsValid( true );

        Utils::PrintMessage( "Created film with id [%s]", a_sID );
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
Film::~Film()
{
    if( IsValid() )
    {
        // Delete our film, and our output object
        delete m_pFilm;
        delete m_pOutput;
        free( m_sOutputName );
        SetIsValid(false);

        Utils::PrintMessage("Deleting film");
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
void Film::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
bool Film::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Film) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Film::PyAsString()
{
    return PyString_FromFormat("Film plate [%s]", m_sOutputName );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/6/2009
////////////////////////////////////////////////////////////////////////////////
void Film::AppendFilmFilterTypes( PYOBJECT a_pPyModule )
{
    if( PyModule_Check(a_pPyModule) )
    {
        PYOBJECT pDictionary = PyModule_GetDict( a_pPyModule );

        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, FilterType_Box,     "FilmFilterType_Box"   );
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, FilterType_Gauss,   "FilmFilterType_Gauss" );
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, FilterType_Mitchell,"FilmFilterType_Mitchell" );
    }
}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Film )
    // ...
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Film, "Film", "Film object", PYTHON_TYPE_FINAL );