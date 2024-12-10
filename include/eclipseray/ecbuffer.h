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
/// \file ecbuffer.h
/// \brief Defines utilities for managing static memory buffers
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 15:12:2008   11:50
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _ECBUFFER_H
#define _ECBUFFER_H

#include <eclipseray/eclipse.h>

////////////////////////////////////////////////////////////////////////////
//
/// \class Buffer
/// \author Dan Torres
/// \created 2008/12/15
/// \brief A generic memory buffer
//
/// Memory buffers can be defined to create static heaps used for intermediate
/// mesh creation. More specifically, you will want to create a vertex buffer
/// and an index buffer to keep intermediate data while you define new
/// meshes. This prevents memory fragmentation while still allowing you to have
/// as many static buffers as needed, for example, if more than one thread is
/// creating them.
/// 
/// Note that we don't define concrete objects as vertex, uvcoords, etc. Instead
/// we keep an internal type manager based on the intended usage. It would have
/// been simpler to use templates, but that would have required a new set
/// of Python macros for its script declarations. We don't have that much time.
/// 
/// IMPORTANT: The format for index buffers is <vertex> <uv>, ...
/// 
/// About Python lists Vs EclipseRay buffers:
/// 
/// It would seem a better idea to directly use lists to create meshes, instead
/// of having to create intermediate buffers. For one, using buffer objects as
/// a static heap is a better way to manage memory internally; Also, meshes
/// can do object-oriented validation of the data they receive; Finally, it
/// may be that we implement ways of reading binary data (i.e., data that 
/// can be more easily processed from a binary source than parsed from text and
/// placed into a Python list), and in those cases, having a buffer object to
/// encapsulate the resulting data and being able to throw it around in Python will
/// be very handy.
//
////////////////////////////////////////////////////////////////////////////
class Buffer : public EclipseObject
{
    DECLARE_PYTHON_HEADER;

public:

    /*!
    *  @enum eBufferUsage
    *  Provides an identifier that declares the usage of data contained in a buffer
    */
    enum eBufferUsage
    {
        Usage_Position,    ///< Contains sets of 3 floating point numbers representing position
        Usage_Texcoord,    ///< Contains sets of 2 floating point numbers representing UV coordinates
        Usage_Index,       ///< Contains sets of 2 integers representing indexes into vertex and uv buffers
        Usage_Raw          ///< Contains unknown information. This is a raw data buffer
    };


    // -------------------------------------------------------------------------
    // Creation / destruction
    // -------------------------------------------------------------------------

    /*!
     *  Explicit constructor.
     *  Specify the type and number of elements to allocate, NOT the size in bytes
     *  (unless the usage is Usage_Raw, then a_nElements will naturally be the buffer
     *  size in bytes)
     *  @param a_nUsage Buffer usage
     *  @param a_nElements Number of elements of type eBufferUsage to allocate (NOT number of bytes)
     */
	Buffer( eBufferUsage a_nUsage, size_t a_nElements );

    // -------------------------------------------------------------------------
    // Access
    // -------------------------------------------------------------------------

    /*!
     *  Provides access to our buffer.
     *  If the discard flag is enabled, the whole heap is set to zero. Call Unlock
     *  after you are done accessing this buffer. The content offset must be on usage
     *  units, not in bytes. For example, if this is a position array, an offset of 1
     *  will skip the first 12 bytes.
     *  @param a_nContentOffset Offset, in usage units, NOT bytes, from the start of the buffer.
     *  @param a_bDiscard If true, the buffer is cleaned before passing it to the caller
     *  @return A pointer to the first element in our buffer. NULL if the buffer could not
     *          be locked. Reasons for this will include out of memory conditions, or another
     *          object currently accessing this buffer.
     */
    void* Lock( size_t a_nContentOffset = 0,  bool a_bDiscard = false );

    /*!
     *  Releases access to our buffer.
     *  You must call this function after modifying the contents of this buffer,
     *  otherwise, further calls to Lock will fail.
     */
    void Unlock();

    /// Returns our assigned buffer usage
    inline eBufferUsage GetUsage() const {return m_nUsage; }
   
    /// Returns our total allocated size, in bytes
    inline size_t GetAllocatedSize() const { return m_nBufferSize; }

	/// Returns our total number of elements
	inline size_t GetElementCount() const {return m_nElementCount; }

    // -------------------------------------------------------------------------
    // Python interface
    // -------------------------------------------------------------------------

    /*! 
    *   Python type check
    *   Verifies that the provided python object encapsulates a buffer
    *   @return True if the provided object type is the same as ours
    */
    static bool PyTypeCheck( PYOBJECT a_pObject );

    /*!
    *  Python text representation method
    *  @return A python string object with a description of ourselves 
    */
    virtual PYOBJECT PyAsString();  

    /*!
     *  Appends buffer usage identifiers to the module of the provided dictionary
     *  @param a_pPyModule A valid python module
     */
    static void AppendBufferUsageTypes( PYOBJECT a_pPyModule );

    // -------------------------------------------------------------------------
    // Python-specific methods
    //
    // Two ways of adding data: 
    //  1) Element by element. Call Lock, then call append as many times as 
    //      needed, then call Unlock. 
    //  2) In one whole batch. Call setData with a list of components. 
    //
    // For both cases, the data provided must be of the eDataType contained by the buffer.
    // -------------------------------------------------------------------------

    // Prepares the buffer for quick by-component addition, lots of assumptions
    // are done here in favor of speed, so read function documentation.
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, lock );

    // Quickly appends 1 eDataType component. It is assumed that the provided object
    // is the correct eDataType for this buffer.
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, append );

    // Closes the buffer after one or more addData operations
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, unlock );


    // Appends data into our buffer. Expects a list of components of the required
    // type, in order to speed-up buffer configuration. It is expected that you
    // use very few calls to this function (ideally one) to fill-up each buffer
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, setData );

    // Cleans our whole array and sets the buffer marker to zero
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, clear );

    // Returns a list with our information. Usually you wouldn't call this unless
    // you want to debug something in python
    DECLARE_PYTHON_OBJECT_METHOD( Buffer, data );
    
protected:

    // Destroys our internal buffer, and frees its requested heap
    ~Buffer();

    // Internal data type
    enum eDataType{ Type_UnsignedInt, Type_Float, Type_Byte };

    // Size in bytes from usage descriptor
    size_t GetByteSizeFromUsage( eBufferUsage a_nUsage );

    // Type from usage descriptor
    eDataType GetTypeFromUsage( eBufferUsage a_nUsage );

    // datatype size
    size_t GetDataTypeSize( eDataType a_nDataType );

    /*!
    *  Child message hook for reference-count based destruction.
    *  The child class is responsible for deleting any instance to which this
    *  this function is called.
    */
    virtual void DeleteObject();


private:

    void*			m_pBuffer;          ///< Our buffer in main memory
    size_t			m_nBufferSize;      ///< Total allocated size in bytes
	size_t			m_nElementCount;	///< Number of contained elements
    eBufferUsage	m_nUsage;           ///< Our buffer usage
    eDataType		m_nType;            ///< Internal data type
    bool			m_bLocked;          ///< Lock control

    unsigned int	m_nPyBufferMarker;  ///< Marker (in bytes) for incremental data addition
    size_t          m_nPyDataTypeSize;  ///< Number of bytes per data type block


};


#endif