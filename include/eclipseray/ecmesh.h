///////////////////////////////////////////////////////////////////////////////
//
// ecmesh
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
/// \file ecmesh.h
/// \brief Defines classes and interfaces for the mesh object
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 16:12:2008   11:14
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESH_H
#define _MESH_H

#include <eclipseray/eclipse.h>
#include <eclipseray/yrtypes.h>
#include <eclipseray/ecscene.h>

class Buffer;
class Material; 
class Matrix;

////////////////////////////////////////////////////////////////////////////
//
/// \class Mesh
/// \author Dan Torres
/// \created 2008/12/16
/// \brief Defines the base mesh object
//
/// Mesh objects register all the appropriate buffers into our tracer, and
/// keep an instanced abstraction that results useful for reference
/// or any other specific operation that needs to remember what belongs to
/// whom (Inside our raytracer all we have are triangles, so its useful to
/// keep the object abstraction outside)
/// 
//
////////////////////////////////////////////////////////////////////////////
class Mesh : public EclipseObject
{
	DECLARE_PYTHON_HEADER;

public:

	/*!
	 *	Creates a new mesh object
     *  The vertex offset and vertex count numbers are applied to the UV buffer as well, 
     *  meaning that the vertex buffer and the uv buffer MUST have a one-to-one correspondance
	 *	@param a_sID Unique mesh ID
	 *	@param a_pVertexBuffer A buffer containing all required vertices
     *  @param a_nVertexOffset Offset (in number of vertices) for use when reading from the vertex buffer
     *  @param a_nVertexCount Number of vertices to read from the vertex buffer
     *	@param a_pUVBuffer A buffer containing a set of UV coordinates per vertex
     *  @param a_nUVOffset Offset (in number of UV pairs) for accessing our uv buffer
     *  @param a_nUVCount Number of UV components to read from the buffer
	 *	@param a_pIndexBuffer A buffer for a triangle list, containing 6 indexes per face (3 vert + 3 uv )
     *  @param a_nIndexOffset Offset (in number of components) into the index buffer
     *  @param a_nTriangleCount Number of triangles to include. This determines the number of indexes to read (6 per triangle)
	 *	@param a_pScene	A valid scene for this mesh to live in
	 *	@param a_pMaterial A valid material for this mesh to have
     *  @param a_pMatrix An optional transformation matrix to apply over the vertex buffer
	 */
	Mesh( const char* a_sID, 
        Buffer* a_pVertexBuffer, unsigned int a_nVertexOffset, unsigned int a_nVertexCount, 
        Buffer* a_pUVBuffer,     unsigned int a_nUVOffset,     unsigned int a_nUVCount,
        Buffer* a_pIndexBuffer,  unsigned int a_nIndexOffset,  unsigned int a_nTriangleCount,
		Scene* a_pScene, Material* a_pMaterial, float* a_pBSphere, Matrix* a_pMatrix = NULL);

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *	Returns the actual yafaray mesh
     */
    inline YRTriangleObject* GetYRTrimesh(){
        return m_pScene->GetScenePtr()->getMesh( m_nMeshID );
    }; 
 


	// -------------------------------------------------------------------------
	// Python stuff
	// -------------------------------------------------------------------------

	/*! 
	*   Python type check
	*   Verifies that the provided python object encapsulates a mesh
	*   @return True if the provided object type is the same as ours
	*/
	static bool PyTypeCheck( PYOBJECT a_pObject );

	/*!
	*  Python text representation method
	*  This function must be implemented by all children
	*  @return A python string object with a description of ourselves 
	*/
	virtual PYOBJECT PyAsString();

    void GetBoundingSphere(YRPoint3D& position, float& radius) { position = m_vPosition; radius = m_fRadius; }

protected:

	/// Only we can destroy ourselves
    /// (... and other methods will only make this object stronger? - JG)
	~Mesh();

	// Create a mesh with the provided arrays
	bool CreateMesh( const char* a_sID, Scene* a_pScene, 
        Buffer* a_pVertices, unsigned int a_nVertexOffset, unsigned int a_nVertexCount, 
        Buffer* a_pUVs,      unsigned int a_nUVOffset,     unsigned int a_nUVCount,
        Buffer* a_pIndices,  unsigned int a_nIndexOffset,  unsigned int a_nTriangleCount, 
        float* a_pBSphere, const Matrix& a_matrix );

	/*!
	*  Child message hook for reference-count based destruction.
	*  The child class is responsible for deleting any instance to which this
	*  this function is called.
	*/
	virtual void DeleteObject();

private:

	Material*	m_pMaterial;		///< Material used to render this mesh
    Scene*      m_pScene;           ///< Scene that contains this material
	YRObjectID	m_nMeshID;			///< Unique id for this mesh
    YRPoint3D   m_vPosition;        ///< Center of bounding sphere
    float       m_fRadius;          ///< Bounding sphere radius
};

#endif