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
uniform mat4 modelview_projection_matrix;

// The vertex data
in vec3 vertex_pos;

// The cylinder data:
in vec3 cylinder_base;				// The position of the cylinder in model coordinates.
in vec3 cylinder_axis;				// The axis of the cylinder in model coordinates.
in float cylinder_radius;			// The radius of the cylinder in model coordinates.
in vec4 cylinder_color;

// Outputs to fragment shader
flat out vec4 cylinder_color_in;		// The base color of the cylinder.
flat out vec3 cylinder_view_base;		// Transformed cylinder position in view coordinates
flat out vec3 cylinder_view_axis;		// Transformed cylinder axis in view coordinates
flat out float cylinder_radius_in;		// The radius of the cylinder
flat out float cylinder_length_squared;	// The squared length of the cylinder

void main()
{
	// Pass color to fragment shader.
	cylinder_color_in = cylinder_color;
	// Pass radius to fragment shader.
	cylinder_radius_in = cylinder_radius;
	// Pass length to fragment shader.
	cylinder_length_squared = cylinder_axis.x * cylinder_axis.x + cylinder_axis.y * cylinder_axis.y + cylinder_axis.z * cylinder_axis.z;

	// Transform cylinder to eye coordinates.
	cylinder_view_base = vec3(modelview_matrix * vec4(cylinder_base, 1));
	cylinder_view_axis = vec3(modelview_matrix * vec4(cylinder_axis, 0));

	// Transform and project vertex position.
	gl_Position = modelview_projection_matrix * vec4(vertex_pos, 1.0);
}
