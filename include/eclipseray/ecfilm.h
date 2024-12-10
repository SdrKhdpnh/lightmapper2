///////////////////////////////////////////////////////////////////////////////
//
// ECFilm
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
/// \file ECFilm.h
/// \brief Defines parameters and output settings for render products
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 1/6/2009
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ECFILM_H
#define _ECFILM_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class Film
/// \author Dan Torres
/// \created 1/6/2009
/// \brief Defines a film object that keeps rendering settings and output data
//
/// Film objects encapsulate YR's image film, and keeps both the film and
/// output settings in one object. This is particularly handy if we render
/// multiple versions of the same scene, or attach different maps to the
/// same mesh.
/// 
/// Output formats are EXR or simple TGA. Since the second one is more useful
/// this is the one we implement. In the future, however, should be easy to
/// add support for the other, if necessary.
//
////////////////////////////////////////////////////////////////////////////
class Film : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    // -------------------------------------------------------------------------
    // Public structs and enumerations
    // -------------------------------------------------------------------------

    /*!
     *  Defines supported Antialias filter types for film
     */
    enum eFilmFilterType
    {
        FilterType_Box,
        FilterType_Mitchell,
        FilterType_Gauss
    };

    /*!
     *	Construction
     *  @param a_sOutputName Name of the image file to contain the film output, with extension.
     *  @param a_nWidth Film width
     *  @param a_nHeight Film height
     *  @param a_nFilterType Antialias filter type
     *  @param a_fFilterSize Pixel width
     *  @param a_fGamma Film gamma correction
     *  @param a_bHasDepth If true, add depth channel to the film
     *  @param a_bClampRGB If true, clamp rgb values to 0-1
     */
	Film( const char* a_sID, int a_nWidth, int a_nHeight, eFilmFilterType a_nFilterType, 
        float a_fFilterSize, float a_fGamma, bool a_bHasDepth, bool a_bClamp );

    /*!
     *	Provides access to our film object
     *  @return If the film is valid, a pointer to the internal film object 
     */
    inline YRFilm* GetYRFilm(){ return m_pFilm; }

    /*!
     *	Get the film width
     *  @return width dimensions for this film
     */
    inline int GetWidth() const { return m_nWidth; }

    /*!
     *	Get the film height
     *  @return height dimesions for this film
     */
    inline int GetHeight() const { return m_nHeight; }

    // -------------------------------------------------------------------------
    // Python stuff
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates a film object
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
    *  Python text representation method
    *  This function must be implemented by all children
    *  @return A python string object with a description of ourselves 
    */
    virtual PYOBJECT PyAsString();

    /*!
    *  Appends film filter type identifiers to the module of the provided dictionary
    *  @param a_pPyModule A valid python module
    */
    static void AppendFilmFilterTypes( PYOBJECT a_pPyModule );


protected:

    // Destruction
    ~Film();

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();


private:

    YRTga*              m_pOutput;      ///< Color output for this film object
    YRFilm*             m_pFilm;        ///< Our actual film
    char*               m_sOutputName;  ///< Name for the output file
    int                 m_nWidth;       ///< Film width
    int                 m_nHeight;      ///< Film height

};



#endif