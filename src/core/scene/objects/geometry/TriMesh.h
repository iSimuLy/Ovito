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

/**
 * \file TriMesh.h
 * \brief Contains definition of Ovito::TriMesh class.
 */

#ifndef __OVITO_TRI_MESH_H
#define __OVITO_TRI_MESH_H

#include <core/Core.h>
#include "TriMeshFace.h"

namespace Ovito {

/**
 * \brief Stores a triangular mesh.
 */
class OVITO_CORE_EXPORT TriMesh
{
public:

	/// \brief Creates an empty mesh.
	TriMesh() : _hasVertexColors(false) {}

	/// \brief Resets the mesh to the empty state.
	void clear();

	/// \brief Returns the bounding box of the mesh.
	/// \return The bounding box of the mesh.
	///
	/// The bounding box is cached by the TriMesh object.
	/// Calling this method multiple times is cheap as long as the vertices of the mesh are not changed.
	const Box3& boundingBox() {
		if(_boundingBox.isEmpty())
			_boundingBox.addPoints(vertices().constData(), vertexCount());
		return _boundingBox;
	}

	/// \brief Returns the number of vertices in this mesh.
	int vertexCount() const { return _vertices.size(); }

	/// \brief Sets the number of vertices in this mesh.
	/// \param n The new number of vertices.
	///
	/// If \a n is larger than the old vertex count then new vertices are added to the mesh.
	/// These new vertices are not initialized by this method. One should use a method like setVertexPos()
	/// to assign a position to the new vertices.
	void setVertexCount(int n);

	/// \brief Allows direct access to the vertex position array of the mesh.
	/// \return A reference to the internal container that stores the vertex coordinates.
	/// \note When you change the vertex positions then you have to call invalidateVertices()
	/// to let the mesh know that it has to update its internal cache based on the new vertex coordinates.
	QVector<Point3>& vertices() { return _vertices; }

	/// \brief Allows direct read-access to the vertex position array of the mesh.
	/// \return A constant reference to the internal container that stores the vertex positions.
	const QVector<Point3>& vertices() const { return _vertices; }

	/// \brief Returns the coordinates of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose position should be returned.
	/// \return The position of the given vertex.
	const Point3& vertex(int index) const {
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		return _vertices[index];
	}

	/// \brief Returns a reference to the coordinates of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose position should be returned.
	/// \return A reference to the coordinates of the given vertex. The reference can be used to alter the vertex position.
	/// \note If you change the vertex' position then you have to call invalidateVertices() to let the mesh know that it has
	/// to update its internal caches based on the new vertex position.
	Point3& vertex(int index) {
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		return _vertices[index];
	}

	/// \brief Sets the coordinates of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose position should be set.
	/// \param p The new position of the vertex.
	/// \note After you have finished changing the vertex positions you have to call invalidateVertices()
	/// to let the mesh know that it has to update its internal caches based on the new vertex position.
	void setVertex(int index, const Point3& p) {
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		_vertices[index] = p;
	}

	/// \brief Returns whether this mesh has colors associated with its vertices.
	bool hasVertexColors() const {
		return _hasVertexColors;
	}

	/// \brief Controls whether this mesh has colors associated with its vertices.
	void setHasVertexColors(bool enableColors) {
		_hasVertexColors = enableColors;
		_vertexColors.resize(enableColors ? _vertices.size() : 0);
	}

	/// \brief Allows direct access to the vertex color array of the mesh.
	/// \return A reference to the vector that stores all vertex colors.
	/// \note After you have finished changing the vertex colors,
	/// you have to call invalidateVertices() to let the mesh know that it has to update its internal
	/// caches based on the new vertex colors.
	QVector<ColorA>& vertexColors() {
		OVITO_ASSERT(_hasVertexColors);
		OVITO_ASSERT(_vertexColors.size() == _vertices.size());
		return _vertexColors;
	}

	/// \brief Allows direct read-access to the vertex color array of the mesh.
	/// \return A constant reference to the vector that stores all vertex colors.
	const QVector<ColorA>& vertexColors() const {
		OVITO_ASSERT(_hasVertexColors);
		OVITO_ASSERT(_vertexColors.size() == _vertices.size());
		return _vertexColors;
	}

	/// \brief Returns the color of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose color should be returned.
	/// \return The color of the given vertex.
	const ColorA& vertexColor(int index) const {
		OVITO_ASSERT(_hasVertexColors);
		OVITO_ASSERT(_vertexColors.size() == _vertices.size());
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		return _vertexColors[index];
	}

	/// \brief Returns a reference to the color of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose color should be returned.
	/// \return A reference to the color of the given vertex. The reference can be used to alter the vertex color.
	/// \note After you have finished changing the vertex colors,
	/// you have to call invalidateVertices() to let the mesh know that it has
	/// to update its internal caches based on the new vertex colors.
	ColorA& vertexColor(int index) {
		OVITO_ASSERT(_hasVertexColors);
		OVITO_ASSERT(_vertexColors.size() == _vertices.size());
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		return _vertexColors[index];
	}

	/// \brief Sets the color of the vertex with the given index.
	/// \param index The index starting at 0 of the vertex whose color should be set.
	/// \param p The new color of the vertex.
	/// \note After you have finished changing the vertex colors,
	/// you have to call invalidateVertices() to let the mesh know that it has
	/// to update its internal caches based on the new vertex colors.
	void setVertexColor(int index, const ColorA& c) {
		OVITO_ASSERT(_hasVertexColors);
		OVITO_ASSERT(_vertexColors.size() == _vertices.size());
		OVITO_ASSERT(index >= 0 && index < vertexCount());
		_vertexColors[index] = c;
	}

	/// \brief Invalidates the parts of the internal mesh cache that depend on the vertex array.
	///
	/// This method must be called each time the vertices of the mesh have been modified.
	void invalidateVertices() {
		_boundingBox.setEmpty();
	}

	/// \brief Returns the number of faces (triangles) in this mesh.
	int faceCount() const { return _faces.size(); }

	/// \brief Sets the number of faces in this mesh.
	/// \param n The new number of faces.
	///
	/// If \a n is larger than the old face count then new faces are added to the mesh.
	/// These new faces are not initialized by this method. One has to use methods of the TriMeshFace class
	/// to assign vertices to the new faces.
	void setFaceCount(int n);

	/// \brief Allows direct access to the face array of the mesh.
	/// \return A reference to the vector that stores all mesh faces.
	/// \note If you change the faces in the returned array then
	/// you have to call invalidateFaces() to let the mesh know that it has
	/// to update its internal caches based on the new face definitions.
	QVector<TriMeshFace>& faces() { return _faces; }

	/// \brief Allows direct read-access to the face array of the mesh.
	/// \return A const reference to the vector that stores all mesh faces.
	const QVector<TriMeshFace>& faces() const { return _faces; }

	/// \brief Returns the face with the given index.
	/// \param index The index starting at 0 of the face who should be returned.
	const TriMeshFace& face(int index) const {
		OVITO_ASSERT(index >= 0 && index < faceCount());
		return _faces[index];
	}

	/// \brief Returns a reference to the face with the given index.
	/// \param index The index starting at 0 of the face who should be returned.
	/// \return A reference to the requested face. This reference can be used to change the
	/// the face.
	/// \note If you change the returned face then
	/// you have to call invalidateFaces() to let the mesh know that it has
	/// to update its internal caches based on the new face definition.
	TriMeshFace& face(int index) {
		OVITO_ASSERT(index >= 0 && index < faceCount());
		return _faces[index];
	}

	/// \brief Adds a new triangle face and returns a reference to it.
	/// \return A reference to the new face. The new face has to be initialized
	/// after it has been created.
	///
	/// Increases the number of faces by one.
	TriMeshFace& addFace();

	/// \brief Invalidates the parts of the internal mesh cache that depend on the face array.
	///
	/// This must be called each time the faces of the mesh have been modified.
	void invalidateFaces() {}

	/************************************* Ray intersection *************************************/

	/// \brief Performs a ray intersection calculation.
	/// \param ray The ray to test for intersection with the object.
	/// \param t If an intersection has been found, the method stores the distance of the intersection in this output parameter.
	/// \param normal If an intersection has been found, the method stores the surface normal at the point of intersection in this output parameter.
	/// \param faceIndex If an intersection has been found, the method stores the index of the face intersected by the ray in this output parameter.
	/// \param backfaceCull Controls whether backfacing faces are tested too.
	/// \return True if the closest intersection has been found. False if no intersection has been found.
	bool intersectRay(const Ray3& ray, FloatType& t, Vector3& normal, int& faceIndex, bool backfaceCull) const;

	/*********************************** Persistence ***********************************/

	/// \brief Saves the mesh to the given stream.
    /// \param stream The destination stream.
	void saveToStream(SaveStream& stream);

	/// \brief Loads the mesh from the given stream.
    /// \param stream The source stream.
	void loadFromStream(LoadStream& stream);

private:

	/// The cached bounding box of the mesh computed from the vertices.
	Box3 _boundingBox;

	/// Array of vertex coordinates.
	QVector<Point3> _vertices;

	/// Indicates whether per-vertex colors are stored in this mesh.
	bool _hasVertexColors;

	/// Array of vertex colors.
	QVector<ColorA> _vertexColors;

	/// Array of mesh faces.
	QVector<TriMeshFace> _faces;
};

};

#endif // __OVITO_TRI_MESH_H
