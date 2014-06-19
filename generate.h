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
#ifndef GENERATE_H_INCLUDED
#define GENERATE_H_INCLUDED

#include "mesh.h"
#include <utility>

inline Mesh invert(Mesh mesh)
{
    vector<Triangle> triangles;
    triangles.reserve(mesh->size());
    for(Triangle tri : *mesh)
    {
        swap(tri.p[0], tri.p[1]);
        swap(tri.c[0], tri.c[1]);
        swap(tri.t[0], tri.t[1]);
        triangles.push_back(tri);
    }
    return Mesh(new Mesh_t(mesh->texture(), triangles));
}

inline TransformedMesh invert(TransformedMesh mesh)
{
    mesh.mesh = invert(mesh.mesh);
    return mesh;
}

namespace Generate
{
	inline Mesh quadrilateral(TextureDescriptor texture, VectorF p1, Color c1, VectorF p2, Color c2, VectorF p3, Color c3, VectorF p4, Color c4)
	{
		const TextureCoord t1 = TextureCoord(texture.minU, texture.minV);
		const TextureCoord t2 = TextureCoord(texture.maxU, texture.minV);
		const TextureCoord t3 = TextureCoord(texture.maxU, texture.maxV);
		const TextureCoord t4 = TextureCoord(texture.minU, texture.maxV);
		return Mesh(new Mesh_t(texture.image, vector<Triangle>{Triangle(p1, c1, t1, p2, c2, t2, p3, c3, t3), Triangle(p3, c3, t3, p4, c4, t4, p1, c1, t1)}));
	}

	/// make a box from <0, 0, 0> to <1, 1, 1>
	inline Mesh unitBox(TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz)
	{
		const VectorF p0 = VectorF(0, 0, 0);
		const VectorF p1 = VectorF(1, 0, 0);
		const VectorF p2 = VectorF(0, 1, 0);
		const VectorF p3 = VectorF(1, 1, 0);
		const VectorF p4 = VectorF(0, 0, 1);
		const VectorF p5 = VectorF(1, 0, 1);
		const VectorF p6 = VectorF(0, 1, 1);
		const VectorF p7 = VectorF(1, 1, 1);
		Mesh retval = Mesh(new Mesh_t());
		const Color c = Color(1);
		if(nx)
		{
			retval->add(quadrilateral(nx,
									 p0, c,
									 p4, c,
									 p6, c,
									 p2, c
									 ));
		}
		if(px)
		{
			retval->add(quadrilateral(px,
									 p5, c,
									 p1, c,
									 p3, c,
									 p7, c
									 ));
		}
		if(ny)
		{
			retval->add(quadrilateral(ny,
									 p0, c,
									 p1, c,
									 p5, c,
									 p4, c
									 ));
		}
		if(py)
		{
			retval->add(quadrilateral(py,
									 p6, c,
									 p7, c,
									 p3, c,
									 p2, c
									 ));
		}
		if(nz)
		{
			retval->add(quadrilateral(nz,
									 p1, c,
									 p0, c,
									 p2, c,
									 p3, c
									 ));
		}
		if(pz)
		{
			retval->add(quadrilateral(pz,
									 p4, c,
									 p5, c,
									 p7, c,
									 p6, c
									 ));
		}
		return retval;
	}
}

#endif // GENERATE_H_INCLUDED
