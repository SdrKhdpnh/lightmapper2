///////////////////////////////////////////////////////////////////////////////
//
// utils
//
// Copyright (c) 2009 Bioware.
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
/// \file utils.h
/// \brief Defines a number of utilities for the whole application
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 10:12:2008   14:19
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UTILS_H
#define _UTILS_H

class EclipseObject;

// Use a namespace to protect some of the utility functions we have here just
// scattered around.
namespace Utils
{
    /*!
    *  Prints a message to the output buffer
    *  Automatically checks to see if verbose mode is enabled. If it is, the provided
    *  message is printed, otherwise, the function will just return. Use it to
    *  generically throw messages without having to test for this flag every time
    *  @param a_sMsg The message to write
    */
    void PrintMessage( const char* a_sMsg, ... );


    ////////////////////////////////////////////////////////////////////////////
    //
    /// \class ErrorManager
    /// \author Dan Torres
    /// \created 12/17/2008
    /// \brief A static utility for reporting global errors
    //
    /// This is an EXTREMELY simple error management interface. It just tracks
    /// the last n error messages, and the objects that generated them. At any 
    /// point in time you can check to see if someone raised the error flag, and 
    /// then access the offending objects.
    /// 
    //
    ////////////////////////////////////////////////////////////////////////////
    class ErrorManager
    {
    public:

        // ---------------------------------------------------------------------
        // Structures, enums, definitions
        // ---------------------------------------------------------------------

        /*!
         *	@enum eErrorType
         *  Defines known/common error types
         */
        enum eErrorType
        {
            ErrorType_Assert,   ///< Something should never had happened (program error)
            ErrorType_Params,   ///< Wrong parameters
            ErrorType_Instance, ///< Failed to instantiate something
            ErrorType_Internal, ///< Something just failed to work

            // ...
            ErrorType_Unknown   ///< We don't know. But its wrong.
        };

        /*!
         *  @struct ErrorEvent
         *	Groups together information pertinent to one error occurrence
         */
        struct ErrorEvent
        {
            EclipseObject*  s_pFailedObject;    ///< Object that triggers the error
            char*           s_sFailMessage;     ///< Whatever the object wanted to say
            eErrorType      s_nType;            ///< In a nutshell, the nature of the error
            ErrorEvent*     s_pNextError;       ///< Next available error
        };

        // ---------------------------------------------------------------------
        // Start/stop
        // ---------------------------------------------------------------------

        /*!
         *	Starts error management services
         *  You must call this function BEFORE any other error function
         *  @param a_nErrorHeapSize Size of the error heap (number of error messages to keep)
         */
        static void Start( int a_nErrorHeapSize );

        /*!
         *	Stops error management services
         *  You must call this function right before your program ends
         */
        static void Stop();

        // ---------------------------------------------------------------------
        // Utilities
        // ---------------------------------------------------------------------

        /*!
         *	Reports an error in our heap
         *  If an object is passed, its reference count will increase by 1.
         *  @param a_nType Error classification
         *  @param a_pObject The object where the error takes place. 
         *  @param a_sDesc A brief description of the error, followed by any formatting params
         */
        static void Report( eErrorType a_nType, EclipseObject* a_pObject, const char* a_sDsc, ... );

        /*!
         *	Returns the current number of active errors in the heap
         *  @return zero or more errors
         */
        static int GetErrorCount();

        /*!
         *	Clears our error heap
         *  Deletes all the errors contained in our heap and all their intermediate data
         */
        static void Clear();

        /*!
         *	Access a particular error from our heap
         *  Returns an error object from our heap, or null if the index is unknown.
         *  If the index is greater than the number of errors in the heap, the last
         *  valid error is returned.
         *  @param a_nIndex zero-based index for our heap. 
         */
        static ErrorEvent* GetError( int a_nIndex );
        
    private:

        // Deletes the children of the provided event, then deletes the event
        static void KillEvent( ErrorEvent*& a_pEvent );

        // Returns the next available error slot so we can add info to it
        static ErrorEvent* GetNextAvailableEvent();

        // Initializes an error
        static void InitializeError( ErrorEvent* a_pError, bool a_bRecycled = false );

        static int          m_nHeapSize;    ///< Total number of error messages to keep
        static ErrorEvent*  m_pFirstError;  ///< Root of our error chain
        static bool         m_bStarted;     ///< Tells if our manager has been started

    
    };

    // ...

}





#endif