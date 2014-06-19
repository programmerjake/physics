/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "mesh.h"
#include "platform.h"
#include <iostream>

Renderer & Renderer::operator <<(const Mesh_t & m)
{
    m.texture().bind();
    glVertexPointer(3, GL_FLOAT, 0, (const void *)m.points.data());
    glTexCoordPointer(2, GL_FLOAT, 0, (const void *)m.textureCoords.data());
    glColorPointer(4, GL_FLOAT, 0, (const void *)m.colors.data());
    glDrawArrays(GL_TRIANGLES, 0, (GLint)m.size() * 3);
    return *this;
}

