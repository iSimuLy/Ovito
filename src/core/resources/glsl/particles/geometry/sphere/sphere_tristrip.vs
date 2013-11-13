///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
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

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec3 cubeVerts[14];

// The particle data:
in vec3 particle_pos;
in vec3 particle_color;
in float particle_radius;

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out float particle_radius_squared_fs;
flat out vec3 particle_view_pos_fs;

void main()
{
	// Forward color to fragment shader.
	particle_color_fs = vec4(particle_color, 1);
	particle_radius_squared_fs = particle_radius * particle_radius;
	particle_view_pos_fs = vec3(modelview_matrix * vec4(particle_pos, 1));

	// Transform and project vertex.
	gl_Position = projection_matrix * modelview_matrix * vec4(particle_pos + cubeVerts[gl_VertexID % 14] * particle_radius, 1);
}