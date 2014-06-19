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
#include "util.h"
#include "platform.h"
#include "physics.h"
#include "generate.h"
#include "texture_atlas.h"
#include <vector>
#include <iostream>

using namespace std;

struct MyObject
{
    shared_ptr<PhysicsObject> physicsObject;
    TransformedMesh getMesh()
    {
        TextureDescriptor td = TextureAtlas::OakWood.td();
        if(physicsObject->isStatic())
            td = TextureAtlas::BirchWood.td();
        else if(physicsObject->isSupported())
            td = TextureAtlas::JungleWood.td();
        Mesh boxMesh = Generate::unitBox(td, td, TextureAtlas::WoodEnd.td(), TextureAtlas::WoodEnd.td(), td, td);
        return transform(Matrix::scale(2).concat(Matrix::translate(-1, -1, -1)).concat(Matrix::scale(physicsObject->getExtents())).concat(Matrix::translate((VectorF)physicsObject->getPosition())), boxMesh);
    }
    MyObject(shared_ptr<PhysicsObject> physicsObject)
        : physicsObject(physicsObject)
    {
    }
};

int myMain(vector<wstring> args)
{
    shared_ptr<PhysicsWorld> physicsWorld = make_shared<PhysicsWorld>();
    vector<MyObject> objects =
    {
        MyObject(PhysicsObject::make(PositionF(0.5f, 1.5, 0, Dimension::Overworld), VectorF(0), true, false, VectorF(0.5f), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(1.1f, 2.5, 0, Dimension::Overworld), VectorF(-0.5f, 0, 0), true, false, VectorF(0.5f), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(0, -3, 0, Dimension::Overworld), VectorF(0), true, false, VectorF(0.5f), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(0, -5, 0, Dimension::Overworld), VectorF(0), false, true, VectorF(2, 0.5f, 2), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(-3, -5, 0, Dimension::Overworld), VectorF(0), false, true, VectorF(0.5f, 2, 2), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(3, -5, 0, Dimension::Overworld), VectorF(0), false, true, VectorF(0.5f, 2, 2), physicsWorld)),
        MyObject(PhysicsObject::make(PositionF(0, -5, -3, Dimension::Overworld), VectorF(0), false, true, VectorF(2, 2, 0.5f), physicsWorld)),
        //MyObject(PhysicsObject::make(PositionF(0, -5, 3, Dimension::Overworld), VectorF(0), false, true, VectorF(2, 2, 0.5f), physicsWorld)),
    };
#if 0
    physicsWorld->stepTime(0.4517);
    physicsWorld->stepTime(100);
    for(MyObject o : objects)
    {
        cout << o.physicsObject->isStatic() << " " << o.physicsObject->isSupported() << " " << (VectorF)o.physicsObject->getPosition() << " " << o.physicsObject->getVelocity() << endl;
    }
    return 0;
#else
    startGraphics();
    Renderer renderer;
    while(true)
    {
        Display::handleEvents(nullptr);
        Display::initFrame();
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Mesh theMesh = Mesh(new Mesh_t());
        for(MyObject & obj : objects)
        {
            theMesh->add(obj.getMesh());
        }
        renderer << transform(Matrix::rotateY(Display::timer() * 0).concat(Matrix::translate(0, 0, -10)), theMesh);
        Display::flip(60);
        physicsWorld->stepTime(Display::frameDeltaTime());
    }
    return 0;
#endif
}

int main(int argc, char ** argv)
{
    vector<wstring> args;
    args.resize(argc);
    for(int i = 0; i < argc; i++)
    {
        args[i] = mbsrtowcs(argv[i]);
    }
    return myMain(args);
}
