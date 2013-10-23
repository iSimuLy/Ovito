///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include "TriMesh.h"

namespace Ovito {

/******************************************************************************
* Clears all vertices and faces.
******************************************************************************/
void TriMesh::clear()
{
	_vertices.clear();
	_faces.clear();
	_vertexColors.clear();
	_boundingBox.setEmpty();
	_hasVertexColors = false;
}

/******************************************************************************
* Sets the number of vertices in this mesh.
******************************************************************************/
void TriMesh::setVertexCount(int n)
{
	_vertices.resize(n);
	if(_hasVertexColors)
		_vertexColors.resize(n);
	invalidateVertices();
}

/******************************************************************************
* Sets the number of faces in this mesh.
******************************************************************************/
void TriMesh::setFaceCount(int n)
{
	_faces.resize(n);
	invalidateFaces();
}

/******************************************************************************
* Adds a new triangle face and returns a reference to it.
* The new face is NOT initialized by this method.
******************************************************************************/
TriMeshFace& TriMesh::addFace()
{
	setFaceCount(faceCount()+1);
	return _faces.back();
}

/******************************************************************************
* Saves the mesh to the given stream.
******************************************************************************/
void TriMesh::saveToStream(SaveStream& stream)
{
	stream.beginChunk(0x01);

	// Save vertices.
	stream << _vertices;

	// Save vertex colors.
	stream << _hasVertexColors;
	stream << _vertexColors;

	// Save faces.
	stream << (int)faceCount();
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {
		stream.writeEnum(face->_flags);
		stream << face->_vertices[0];
		stream << face->_vertices[1];
		stream << face->_vertices[2];
		stream << face->_smoothingGroups;
		stream << face->_materialIndex;
	}

	stream.endChunk();
}

/******************************************************************************
* Loads the mesh from the given stream.
******************************************************************************/
void TriMesh::loadFromStream(LoadStream& stream)
{
	stream.expectChunk(0x01);

	// Reset mesh.
	clear();

	// Load vertices.
	stream >> _vertices;

	// Load vertex colors.
	stream >> _hasVertexColors;
	stream >> _vertexColors;
	OVITO_ASSERT(_vertexColors.size() == _vertices.size() || !_hasVertexColors);

	// Load faces.
	int nFaces;
	stream >> nFaces;
	_faces.resize(nFaces);
	for(auto face = faces().begin(); face != faces().end(); ++face) {
		stream.readEnum(face->_flags);
		stream >> face->_vertices[0];
		stream >> face->_vertices[1];
		stream >> face->_vertices[2];
		stream >> face->_smoothingGroups;
		stream >> face->_materialIndex;
	}

	stream.closeChunk();
}

/******************************************************************************
* Performs a ray intersection calculation.
******************************************************************************/
bool TriMesh::intersectRay(const Ray3& ray, FloatType& t, Vector3& normal, int& faceIndex, bool backfaceCull) const
{
	FloatType bestT = FLOATTYPE_MAX;
	int index = 0;
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {

		Point3 v0 = vertex(face->vertex(0));
		Vector3 e1 = vertex(face->vertex(1)) - v0;
		Vector3 e2 = vertex(face->vertex(2)) - v0;

		Vector3 h = ray.dir.cross(e2);
		FloatType a = e1.dot(h);

		if(std::fabs(a) < FLOATTYPE_EPSILON)
			continue;

		FloatType f = 1 / a;
		Vector3 s = ray.base - v0;
		FloatType u = f * s.dot(h);

		if(u < 0.0f || u > 1.0f)
			continue;

		Vector3 q = s.cross(e1);
		FloatType v = f * ray.dir.dot(q);

		if(v < 0.0f || u + v > 1.0f)
			continue;

		FloatType tt = f * e2.dot(q);

		if(tt < FLOATTYPE_EPSILON)
			continue;

		if(tt >= bestT)
			continue;

		// Compute face normal.
		Vector3 faceNormal = e1.cross(e2);
		if(faceNormal.isZero(FLOATTYPE_EPSILON)) continue;

		// Do backface culling.
		if(backfaceCull && faceNormal.dot(ray.dir) >= 0)
			continue;

		bestT = tt;
		normal = faceNormal;
		faceIndex = (face - faces().constBegin());
	}

	if(bestT != FLOATTYPE_MAX) {
		t = bestT;
		return true;
	}

	return false;
}

};	// End of namespace
