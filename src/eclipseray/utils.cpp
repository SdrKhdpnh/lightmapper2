///////////////////////////////////////////////////////////////////////////////
//
// utils
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
/// \file utils.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   14:24
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/utils.h>
#include <eclipseray/settings.h>
#include <eclipseray/eclipse.h>
#include <eclipseray/pymodules.h>
#include <eclipseray/pymacros.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define ERRORMANAGER_MAX_STR_BUFFER_SIZE    500

// -----------------------------------------------------------------------------
// Error module
// -----------------------------------------------------------------------------

// Methods
PYTHON_MODULE_METHOD_NOARGS( errors, hasErrors );
PYTHON_MODULE_METHOD_NOARGS( errors, getCount  );
PYTHON_MODULE_METHOD_NOARGS( errors, getErrors );
PYTHON_MODULE_METHOD_NOARGS( errors, clear     );

// Method dictionary
START_PYTHON_MODULE_METHODS( errors )
    ADD_MODULE_METHOD( errors, hasErrors, "Returns true if we have errors", METH_NOARGS ),   
    ADD_MODULE_METHOD( errors, getCount,  "Returns the number of errors",   METH_NOARGS ),
    ADD_MODULE_METHOD( errors, getErrors, "Returns a list of errors",       METH_NOARGS ),
    ADD_MODULE_METHOD( errors, clear,     "Clears our error list",          METH_NOARGS ),
END_PYTHON_MODULE_METHODS();

// Initialization
DECLARE_PYTHON_MODULE_INITIALIZATION( errors, "Error reporting utility" )
{
    // Create, and return, the new module
    CREATE_PYTHON_MODULE( errors, "Core error services", pErrorsModule );
    return pErrorsModule;
}

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

namespace Utils
{
	////////////////////////////////////////////////////////////////////////////
	/// \author Dan Torres
	/// \date 12/17/2008
	////////////////////////////////////////////////////////////////////////////	
    void PrintMessage( const char* a_sMsg, ... )
    {
        Settings* pSettings = Settings::GetGlobalSettings();
        if( pSettings && pSettings->Get( Settings::Setting_Output_Verbose )  )
        {
            va_list args;
            va_start( args, a_sMsg );
            vprintf( a_sMsg, args );
            va_end(args);
            printf("\n");
        }
    }

    // -----------------------------------------------------------------------------
    // Error interface
    // -----------------------------------------------------------------------------

    int                         ErrorManager::m_nHeapSize   = 1;
    ErrorManager::ErrorEvent*   ErrorManager::m_pFirstError = NULL;
    bool                        ErrorManager::m_bStarted    = false;    

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::Start( int a_nErrorHeapSize )
    {
        if( !m_bStarted )
        {
            m_nHeapSize  = a_nErrorHeapSize > 0? a_nErrorHeapSize : 1;
            m_bStarted   = true;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::Stop()
    {
        if( m_bStarted )
        {
            Clear();
            m_bStarted = false;          
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::Report( eErrorType a_nType, EclipseObject* a_pObject, 
        const char* a_sDsc, ... )
    {
        ErrorEvent* pError = GetNextAvailableEvent();
        pError->s_pFailedObject = a_pObject;
        pError->s_nType         = a_nType;

        // Keep a reference for this object
        if( pError->s_pFailedObject )
            pError->s_pFailedObject->AddRef();

        static char _tmpStrBuff[ERRORMANAGER_MAX_STR_BUFFER_SIZE];

        va_list args;
        va_start( args, a_sDsc );
        vsprintf( _tmpStrBuff, a_sDsc, args );
        va_end(args);

        pError->s_sFailMessage = strdup( _tmpStrBuff );
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    int ErrorManager::GetErrorCount()
    {
        int nCount = 0;
        ErrorEvent* pError = m_pFirstError;

        while( pError )
        {
            ++nCount;
            pError = pError->s_pNextError;
        }

        return nCount;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::Clear()
    {
        KillEvent( m_pFirstError );
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    ErrorManager::ErrorEvent* ErrorManager::GetError( int a_nIndex )
    {
        ErrorEvent* pEvent = NULL;
        if( (a_nIndex < m_nHeapSize) && (a_nIndex >= 0))
        {
            pEvent = m_pFirstError;
            while( pEvent && a_nIndex )
            {
                --a_nIndex;
                pEvent = pEvent->s_pNextError;
            }
        }

        return pEvent;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::KillEvent( ErrorEvent*& a_pEvent )
    {
        if( a_pEvent )
        {
            // First, kill its children
            KillEvent( a_pEvent->s_pNextError );

            // free our message
            if( a_pEvent->s_sFailMessage )
                free( a_pEvent->s_sFailMessage );

            // free our object reference
            if( a_pEvent->s_pFailedObject )
                a_pEvent->s_pFailedObject->ReleaseRef();

            // Kill ourselves
            delete a_pEvent;
            a_pEvent = NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/17/2008
    ////////////////////////////////////////////////////////////////////////////////
    ErrorManager::ErrorEvent* ErrorManager::GetNextAvailableEvent()
    {
        // If we can just append one more message, go ahead
        if( GetErrorCount() < m_nHeapSize )
        {
            if( m_pFirstError )
            {
                ErrorEvent* pThis = m_pFirstError;
                while( pThis->s_pNextError )
                {
                    pThis = pThis->s_pNextError;
                }

                pThis->s_pNextError = new ErrorEvent;
                InitializeError( pThis->s_pNextError );
                return pThis->s_pNextError;
            }
            else
            {
                m_pFirstError = new ErrorEvent;
                InitializeError( m_pFirstError );
                return m_pFirstError;
            }


        }

        // Otherwise, recycle. We are guaranteed to have at least one node
        else
        {
            // Deal with heaps of just one node
            if( !m_pFirstError->s_pNextError )
            {
                InitializeError( m_pFirstError, true );
                return m_pFirstError;
            }

            // The new root will be the second node, the last node must point
            // to the new one, and the new one should face NULL
            ErrorEvent* pNewNode = m_pFirstError;
            m_pFirstError        = m_pFirstError->s_pNextError;
            ErrorEvent* pLast    = m_pFirstError;
            
            while( pLast->s_pNextError )
                pLast = pLast->s_pNextError;

            pLast->s_pNextError    = pNewNode;
            pNewNode->s_pNextError = NULL;
            InitializeError( pNewNode, true );
            return pNewNode;

        }

    }

    ////////////////////////////////////////////////////////////////////////////////
    /// \author Dan Torres
    /// \date 12/18/2008
    ////////////////////////////////////////////////////////////////////////////////
    void ErrorManager::InitializeError( ErrorEvent* a_pError, bool a_bRecycled /* = false */ )
    {
        a_pError->s_nType = ErrorType_Unknown;
        a_pError->s_pNextError = NULL;

        if( a_bRecycled )
        {
            if( a_pError->s_sFailMessage )
                free ( a_pError->s_sFailMessage );

            if( a_pError->s_pFailedObject )
                a_pError->s_pFailedObject->ReleaseRef();
        }

        a_pError->s_sFailMessage  = NULL;
        a_pError->s_pFailedObject = NULL;

    }



}

// -----------------------------------------------------------------------------
// Error module implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_NOARGS ( errors, hasErrors )
{
    if( Utils::ErrorManager::GetErrorCount() > 0 )
        return PythonReturnValue( PythonReturn_True );
    else
        return PythonReturnValue( PythonReturn_False );
};

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_NOARGS ( errors, getCount  )
{
    return PyInt_FromLong( Utils::ErrorManager::GetErrorCount() );
};

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
//
//  Creates a list with all the current error strings
//
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_NOARGS( errors, getErrors  )
{
    int nErrors = Utils::ErrorManager::GetErrorCount();
    PYOBJECT pErrorList = PyList_New( nErrors );

    for( int i = 0; i < nErrors; i++ )
    {
        Utils::ErrorManager::ErrorEvent* pEvent = Utils::ErrorManager::GetError( i );
        if( pEvent )
        {
            PyList_SetItem( pErrorList, i, PyString_FromString(pEvent->s_sFailMessage));
        }
        else
        {
            PyList_SetItem( pErrorList, i, PyString_FromString("Unknown error"));
        }
    }

    return pErrorList;
};

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
PYTHON_MODULE_METHOD_NOARGS( errors, clear     )
{
    Utils::ErrorManager::Clear();
    return PythonReturnValue( PythonReturn_None );
}