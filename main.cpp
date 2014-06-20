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

float frand(float min, float max)
{
    return interpolate((float)rand() / RAND_MAX, min, max);
}

float frand(float max = 1)
{
    return frand(0, max);
}

int myMain(vector<wstring> args)
{
    shared_ptr<PhysicsWorld> physicsWorld = make_shared<PhysicsWorld>();
    vector<MyObject> objects;
    for(size_t i = 0; i < 20; i++)
    {
        PositionF position;
        position.x = 0;
        position.y = (float)i / 3 - 1;
        position.z = (float)i / 10 + i / 10;
        VectorF velocity(frand(-0.1, 0.1), frand(-0.1, 0.1), frand(-0.1, 0.1));
        //velocity = VectorF(0);
        objects.push_back(MyObject(PhysicsObject::make(position, velocity, true, false, VectorF(0.1f), PhysicsProperties(0.9f), physicsWorld)));
    }
    MyObject floorObject(PhysicsObject::make(PositionF(0, -5, 0, Dimension::Overworld), VectorF(0, 0, 0), false, true, VectorF(5, 0.5f, 5), PhysicsProperties(), physicsWorld));
#if 0
    for(size_t i = 0; i < 100; i++)
    {
        physicsWorld->stepTime(1);
        for(MyObject o : objects)
        {
            cout << o.physicsObject->isStatic() << " " << o.physicsObject->isSupported() << " " << (VectorF)o.physicsObject->getPosition() << " " << o.physicsObject->getVelocity() << endl;
        }
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
        theMesh->add(floorObject.getMesh());
        renderer << transform(Matrix::rotateY(Display::timer() * M_PI / 5).concat(Matrix::translate(0, 0, -10)), theMesh);
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
