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
/// \file ecmesh.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 12/16/2008
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecmesh.h>
#include <eclipseray/ecbuffer.h>
#include <eclipseray/ecmaterial.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecgeometry.h> 

// -----------------------------------------------------------------------------
// Mesh implementation
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/16/2008
////////////////////////////////////////////////////////////////////////////////
Mesh::Mesh( const char* a_sID, 
           Buffer* a_pVertexBuffer, unsigned int a_nVertexOffset, unsigned int a_nVertexCount, 
           Buffer* a_pUVBuffer,     unsigned int a_nUVOffset,     unsigned int a_nUVCount, 
           Buffer* a_pIndexBuffer,  unsigned int a_nIndexOffset,  unsigned int a_nTriangleCount, 
           Scene* a_pScene, Material* a_pMaterial, float* a_pBSphere, Matrix* a_pMatrix /* = NULL */)
:EclipseObject( &m_PythonType ),
 m_pMaterial( a_pMaterial ),
 m_pScene(a_pScene),
 m_nMeshID( 0 ),
 m_vPosition( 0, 0, 0),
 m_fRadius (-1.0f)
{
	// Delegate the creation of the mesh
	if( a_sID && a_pVertexBuffer && a_pIndexBuffer && a_pUVBuffer && a_pScene && a_pMaterial )
	{
        // Select a matrix
        const Matrix* pMatrix = (a_pMatrix != NULL)? a_pMatrix : &Matrix::Identity;

        // Create the mesh
		if(CreateMesh( a_sID, a_pScene, 
            a_pVertexBuffer, a_nVertexOffset, a_nVertexCount, 
            a_pUVBuffer,     a_nUVOffset,     a_nUVCount,  
            a_pIndexBuffer,  a_nIndexOffset,  a_nTriangleCount,
            a_pBSphere, *pMatrix))
		{
			Utils::PrintMessage( "Creating mesh with id [%s] ", a_sID );
		}
		else
        {
            Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Instance, this,
                "Failed to create mesh with id [%s]", a_sID );
        }
	}	
	else
    {
        Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Instance, this,
            "Error on parameters for mesh [%s]", a_sID );
    }

    // Claim a reference over the material, and the scene
    m_pMaterial->AddRef();
    m_pScene->AddRef();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/17/2008
////////////////////////////////////////////////////////////////////////////////
Mesh::~Mesh()
{
    m_pMaterial->ReleaseRef();
    m_pScene->ReleaseRef();
    Utils::PrintMessage("Deleting mesh");
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/17/2008
////////////////////////////////////////////////////////////////////////////////
bool Mesh::CreateMesh( const char* a_sID, Scene* a_pScene, 
    Buffer* a_pVertices, unsigned int a_nVertexOffset,  unsigned int a_nVertexCount, 
    Buffer* a_pUVs,      unsigned int a_nUVOffset,      unsigned int a_nUVCount, 
    Buffer* a_pIndices,  unsigned int a_nIndexOffset,   unsigned int a_nTriangleCount,
    float* a_pBSphere, const Matrix& a_matrix)
{
    
    // Some tests
    if( a_pScene->IsValid() && a_pVertices->IsValid() && a_pIndices->IsValid() && 
        a_pUVs->IsValid() && m_pMaterial->IsValid() )
    {
        size_t nTotalVertices = a_nVertexCount + a_nVertexOffset;
        size_t nTotalUVs      = a_nUVOffset    + a_nUVCount;

        // More specific validation tests
        if( (nTotalVertices <= a_pVertices->GetElementCount()) && 
            (nTotalUVs      <= a_pUVs->GetElementCount() ) && 
            (a_pIndices->GetElementCount() / 3 >= a_nTriangleCount)  )
        {
            YRScene* pScene  = a_pScene->GetScenePtr();
            YRMaterial* pMat = m_pMaterial->GetYRMaterial();

            if( pScene->startGeometry() )
            {
                bool bHasTexcoords = a_nUVCount != 0;
                if( pScene->startTriMesh( m_nMeshID, a_nVertexCount, a_nTriangleCount, false, bHasTexcoords) )
                {
                    YRPoint3D vertex;
                    unsigned int nOffset;

                    // VERTICES
                    // ---------------------------------------------------------

                    // Use the provided matrix to move each vertex into world space
                    const YRMatrix4x4& mWorld = a_matrix.AsYRMatrix();

                    // Transform the bounding sphere
                    m_vPosition.set(a_pBSphere[0], a_pBSphere[1], a_pBSphere[2]);
                    m_vPosition = mWorld * m_vPosition;
                    m_fRadius = a_pBSphere[3];

                    float* pVertices  = (float*)a_pVertices->Lock( a_nVertexOffset );
                    for( unsigned int i = 0; i < a_nVertexCount; i++ )
                    {
                        nOffset = i * 3;
                        vertex.set( pVertices[nOffset], pVertices[nOffset+1], pVertices[nOffset+2] );
                        vertex = mWorld * vertex;

                        pScene->addVertex( vertex );
                    }
                    a_pVertices->Unlock();

                    // UV COORDS
                    // ---------------------------------------------------------
                    float* pTexCoords = (float*)a_pUVs->Lock( a_nUVOffset );
                    for( unsigned int i = 0; i < a_nUVCount; i++ )
                    {
                        nOffset = i * 2;
                        pScene->addUV( pTexCoords[nOffset], pTexCoords[nOffset+1] );
                    }
                    a_pUVs->Unlock();

                    // INDEXES
                    // ---------------------------------------------------------

                    // Now, use the newly added indexes to create our mesh
                    unsigned int j;
                    unsigned int* pIndices = (unsigned int*)a_pIndices->Lock( a_nIndexOffset );
                    for( unsigned int i = 0; i < a_nTriangleCount; i++ )
                    {
                        j = i * 3;
                        if (bHasTexcoords)
                        {
                            // For now, we are assuming that the texture coordinates share the
                            // same indices with the positions - true for Eclipse exported objects.
                            pScene->addTriangle( 
                                pIndices[j], pIndices[j+1], pIndices[j+2],
                                pIndices[j], pIndices[j+1], pIndices[j+2],
                                pMat );
                        }
                        else
                        {
                            pScene->addTriangle(pIndices[j], pIndices[j+1], pIndices[j+2], pMat );
                        }
                    }
                    a_pIndices->Unlock();
                    // Attempt to end
                    if( pScene->endTriMesh() )
                    {
                        SetIsValid( true );
                    }
                }

                if(!pScene->endGeometry())
                    SetIsValid( false );
            }
        }
    }
    return IsValid();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/17/2008
////////////////////////////////////////////////////////////////////////////////
bool Mesh::PyTypeCheck( PYOBJECT a_pObject )
{
    if( a_pObject->ob_type == &PYTHON_TYPE(Mesh) )
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/17/2008
////////////////////////////////////////////////////////////////////////////////
PYOBJECT Mesh::PyAsString()
{
    return PyString_FromFormat( "Mesh with id[%d]", m_nMeshID );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/17/2008
////////////////////////////////////////////////////////////////////////////////
void Mesh::DeleteObject()
{
    delete this;
}

// -----------------------------------------------------------------------------
// Python stuff
// -----------------------------------------------------------------------------

START_PYTHON_OBJECT_METHODS( Mesh )
// ...
END_PYTHON_OBJECT_METHODS();

DECLARE_PYTHON_TYPE( Mesh, "Mesh", "Mesh node", PYTHON_TYPE_FINAL );
