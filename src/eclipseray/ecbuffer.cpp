///////////////////////////////////////////////////////////////////////////////
//
// ecbuffer
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
/// \file ecbuffer.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 15:12:2008   13:34
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecbuffer.h>
#include <eclipseray/utils.h>

// -----------------------------------------------------------------------------
// Buffer implementation
// -----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
Buffer::Buffer(eBufferUsage a_nUsage, size_t a_nElements )
:EclipseObject( &m_PythonType ),
 m_pBuffer( NULL ),
 m_nBufferSize( 0 ),
 m_nElementCount( a_nElements ),
 m_nUsage( a_nUsage ),
 m_bLocked( false ),
 m_nPyBufferMarker( 0 ),
 m_nPyDataTypeSize( 0 )
{
    // Depending on the type, allocate enough space to contain the requested elements
    m_nBufferSize     = GetByteSizeFromUsage(m_nUsage) * m_nElementCount;
    m_pBuffer         = (void*)new unsigned char[ m_nBufferSize ];
    m_nType           = GetTypeFromUsage( m_nUsage );
    m_nPyDataTypeSize = GetDataTypeSize( m_nType );    
    
    if( m_pBuffer )
    {
        SetIsValid( true );
    }

    Utils::PrintMessage( "Creating buffer" );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
Buffer::~Buffer()
{
    // Free our array. We should assert if our locked flag is set at this point, but
    // for the time being, simply perform what is expected.
    if( m_pBuffer )
    {
        unsigned char* pGenericBuffer = (unsigned char*)m_pBuffer;
        delete [] pGenericBuffer;
        m_pBuffer = NULL;
    }

    Utils::PrintMessage( "Deleting buffer" );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
void* Buffer::Lock( size_t a_nContentOffset /* = 0 */, bool a_bDiscard /* = false */ )
{
    // We must be available for locking
    if( !m_bLocked && (a_nContentOffset < m_nElementCount) )
    {
        // Get a pointer to the beginning of the requested memory space
        size_t offsetInBytes   = GetByteSizeFromUsage( m_nUsage ) * a_nContentOffset;
        unsigned char* pBuffer = &((unsigned char*)m_pBuffer)[ offsetInBytes ];

        // If desired, clean our buffer
        if( a_bDiscard )
        {
            memset( pBuffer, 0, m_nBufferSize - offsetInBytes );
        }

        m_nPyBufferMarker = offsetInBytes;
        m_bLocked = true;
        return (void*)pBuffer;
    }
    else
    {
        return NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
void Buffer::Unlock()
{
    // Since we don't support nested locks (subsequent locks will fail), simply
    // unlock. Calling unlock multiple times has no effect in our object. It
    // just releases the flag.
    m_bLocked = false;
    m_nPyBufferMarker = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
size_t Buffer::GetByteSizeFromUsage(eBufferUsage a_nUsage )
{
    switch( a_nUsage )
    {
    case Usage_Position:
        return sizeof(float) * 3;
    	break;
    case Usage_Texcoord:
        return sizeof(float) * 2;
    	break;
    case Usage_Index:
        return sizeof(int) * 2;
        break;
    case Usage_Raw:
        return sizeof(unsigned char);
        break;

    default:
        return sizeof(float);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
Buffer::eDataType Buffer::GetTypeFromUsage(eBufferUsage a_nUsage )
{
    switch( a_nUsage )
    {
    case Usage_Position:
    case Usage_Texcoord:
        // ...
        return Type_Float;
    	break;

    case Usage_Index:
        // ...
        return Type_UnsignedInt;
        break;

    case Usage_Raw:
        return Type_Byte;
        break;

    default:
        return Type_Float;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/5/2009
////////////////////////////////////////////////////////////////////////////////
size_t Buffer::GetDataTypeSize( eDataType a_nDataType )
{
    // Determine block count for a single unit
    switch( a_nDataType )
    {
    case Type_Float:
        return sizeof(float);
        break;

    case Type_UnsignedInt:
        return sizeof(unsigned int);
        break;

        // ...

    default:
        return sizeof(unsigned char);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Buffer::PyTypeCheck(PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Buffer) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Buffer::PyAsString()
{
    switch( m_nUsage )
    {
    case Usage_Position:
        return PyString_FromFormat( "Buffer at %x with spatial 3D points", m_pBuffer );
        break;

    case Usage_Texcoord:
        return PyString_FromFormat( "Buffer at %x with texture coordinates", m_pBuffer );
        break;

    case Usage_Index:
        return PyString_FromFormat( "Buffer at %x with indexes", m_pBuffer );
        break;

    case Usage_Raw:
        return PyString_FromFormat( "Raw data buffer at %x", m_pBuffer );
        break;

        // ...

    default:
        return PyString_FromFormat("Buffer with unknown data at %x", m_pBuffer);
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
void Buffer::DeleteObject()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
void Buffer::AppendBufferUsageTypes(PYOBJECT a_pPyModule )
{
    if( PyModule_Check(a_pPyModule) )
    {
        PYOBJECT pDictionary = PyModule_GetDict( a_pPyModule );
        
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, Usage_Position, "BufferUsage_Position" );
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, Usage_Texcoord, "BufferUsage_Texcoord" );
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, Usage_Index,    "BufferUsage_Index"    );
        PYTHON_ADD_ENUMERATION_TO_DICTIONARY( pDictionary, Usage_Raw,      "BufferUsage_Raw"      );
    }
}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

// Method dictionary
START_PYTHON_OBJECT_METHODS( Buffer )
    ADD_OBJECT_METHOD( Buffer, lock    ),
    ADD_OBJECT_METHOD( Buffer, unlock  ),
    ADD_OBJECT_METHOD( Buffer, append  ),
    ADD_OBJECT_METHOD( Buffer, setData ),
    ADD_OBJECT_METHOD( Buffer, clear   ),
    ADD_OBJECT_METHOD( Buffer, data    ),
END_PYTHON_OBJECT_METHODS();

// Type
DECLARE_PYTHON_TYPE( Buffer, "Buffer", "Memory buffer of different kinds", PYTHON_TYPE_FINAL );

// -----------------------------------------------------------------------------
// Python functions
// -----------------------------------------------------------------------------

#define REPORT_BUFFER_ERROR( _obj, _msg )\
    Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, _obj, _msg )


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/5/2009
//
//  This function prepares our buffer to sequential information addition. It is
//  assumed that the caller knows our data type, and each addition will contain
//  a python object that directly represents that type. No extra validations are
//  done here, in order to favor speed.
//
//  Expects:
//      - (int, optional) offset
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, lock, "Prepares the buffer for appending data")
{
    size_t nOffset = 0;
    Buffer* pSelf  = (Buffer*)a_pSelf;

    // Parse an optional offset
    PyArg_ParseTuple(a_pArgs,"|i",&nOffset);

    // Try to lock this buffer
    if( pSelf->Lock(nOffset) ) 
        return PythonReturnValue( PythonReturn_True );
    else
        return PythonReturnValue( PythonReturn_False );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/5/2009
//
//  Performs a quick add to a previously locked buffer. Assumes the user knows
//  the data type and skips all data validation routines. Use it with care. The
//  expected data type is the eDataType that pertains this particular buffer
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, append, "Adds one data element to our buffer" )
{
    Buffer* pSelf  = (Buffer*)a_pSelf;
    PYOBJECT pData = NULL;

    if( !PyArg_ParseTuple(a_pArgs,"O",&pData)){
        PYTHON_ERROR("Data expected");
    }

    if( pSelf->m_nPyBufferMarker < pSelf->m_nBufferSize )
    {
        unsigned char* pByteBuffer = &((unsigned char*)pSelf->m_pBuffer)[pSelf->m_nPyBufferMarker];

        switch( pSelf->m_nType )
        {
        case Buffer::Type_Float:
            *((float*)pByteBuffer) = (float)PyFloat_AS_DOUBLE(pData);
            break;

        case Buffer::Type_UnsignedInt:
            *((unsigned int*)pByteBuffer) = (unsigned int)PyInt_AS_LONG(pData);
            break;

        case Buffer::Type_Byte:
            // Byte type not supported here
            return PythonReturnValue( PythonReturn_False );
            break;

            // ...

        default:
            return PythonReturnValue( PythonReturn_False );
            break;
        }

        pSelf->m_nPyBufferMarker += pSelf->m_nPyDataTypeSize;
        return PythonReturnValue( PythonReturn_True );
    }
    else
    {
        REPORT_BUFFER_ERROR( pSelf, "Data buffer overflow!" );  
    }

    return PythonReturnValue( PythonReturn_False );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 1/5/2009
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, unlock, "Closes the buffer after one or more append operations")
{
    Buffer* pSelf  = (Buffer*)a_pSelf;
    if( pSelf->m_bLocked )
    {
        // Just check to make sure we don't have a buffer overflow already
        if( pSelf->m_nPyBufferMarker > pSelf->m_nBufferSize ){
            REPORT_BUFFER_ERROR( pSelf, "Data buffer overflow!" );  // this raises an error in our library
            PYTHON_ERROR("Data buffer overflow!");                  // this causes an exception in Python
        }

        pSelf->Unlock();
        return PythonReturnValue( PythonReturn_True );
    }
    return PythonReturnValue( PythonReturn_False );

}


////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, setData, "Populates the buffer with data from the provided list" )
{
    PYOBJECT pNumberList = NULL;
    Buffer* pSelf = (Buffer*)a_pSelf;
    
    // Make sure we can lock
    if( pSelf->Lock() )
    {
        // We always expect a list of base-type components (i.e., floats, ints, etc)
        if(!PyArg_ParseTuple( a_pArgs, "O", &pNumberList ) || !PyList_Check(pNumberList)){
            PYTHON_ERROR( "Expected a list of numbers" );
        }

        int nElements = PyList_Size( pNumberList );

        if( nElements * pSelf->m_nPyDataTypeSize <= pSelf->m_nBufferSize )
        {
            // Note that we have one cycle per object type. This is for efficiency purposes
            if( pSelf->m_nType == Buffer::Type_Float )
            {
                float* pFBuffer = (float*)pSelf->m_pBuffer;
                for( int i = 0; i < nElements; i++ )
                {
                    pFBuffer[i] = (float)PyFloat_AS_DOUBLE(PyList_GET_ITEM(pNumberList,i));
                }
            }
            else if( pSelf->m_nType == Buffer::Type_UnsignedInt )
            {
                unsigned int* pIBuffer = (unsigned int*)pSelf->m_pBuffer;
                for( int i = 0; i < nElements; i++ )
                {
                    pIBuffer[i] = (unsigned int)PyInt_AS_LONG(PyList_GET_ITEM(pNumberList,i));
                }
            }
            // ...
            else
            {
                REPORT_BUFFER_ERROR(pSelf, "Buffer of unknown type provided");
            }

            pSelf->Unlock();
            return PythonReturnValue( PythonReturn_True );
        }
        else
        {
            REPORT_BUFFER_ERROR( pSelf, "Buffer data overflow" );
            return PythonReturnValue( PythonReturn_False );
        }
    }
    // Throw an error if we can't
    else
    {
        REPORT_BUFFER_ERROR(pSelf, "Failed to lock buffer" );
        return PythonReturnValue( PythonReturn_False );
    }
    
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 15:12:2008
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, clear, "Removes all data in our buffer" )
{
    Buffer* pSelf = (Buffer*)a_pSelf;

    // In theory, we could check to make sure that nobody has locked this buffer,
    // but for simplicity, just perform as expected.
    memset( pSelf->m_pBuffer, 0, pSelf->m_nBufferSize );
    pSelf->m_nPyBufferMarker = 0;

    return PythonReturnValue( PythonReturn_None );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_PYTHON_OBJECT_METHOD( Buffer, data, "Returns a list with the contents of the buffer" )
{
    Buffer* pSelf  = (Buffer*)a_pSelf;

    // Floating point array
    if( pSelf->m_nType == Buffer::Type_Float )
    {
        int nElements  = (int)( pSelf->m_nBufferSize / sizeof(float) );
        PYOBJECT pData = PyList_New( nElements );
        float* pFloats = (float*)pSelf->Lock();

        for( int i = 0; i < nElements; i++ )
        {
            PyList_SetItem( pData, i, PyFloat_FromDouble( pFloats[i] ) );
        }

        pSelf->Unlock();
        return pData;
    }
    // unsigned ints
    else if( pSelf->m_nType == Buffer::Type_UnsignedInt  )
    {
        int nElements  = (int)( pSelf->m_nBufferSize / sizeof(unsigned int));
        PYOBJECT pData = PyList_New( nElements );
        unsigned int* pInts = (unsigned int*)pSelf->Lock();

        for( int i = 0; i < nElements; i++ )
        {
            PyList_SetItem( pData, i, PyInt_FromLong(pInts[i]));
        }

        pSelf->Unlock();
        return pData;
    }

    // Unknown or unsupported data type. Return an empty list
    return PyList_New( 0 );    
}

    

