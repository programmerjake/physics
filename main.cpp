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
    size_t objectCount = 14;
#if 1
    objects.push_back(MyObject(PhysicsObject::make(PositionF(-1, -4, 0, Dimension::Overworld), VectorF(0, 0, 0), true, false, VectorF(0.5f), PhysicsProperties(), physicsWorld)->setConstraints(vector<PhysicsConstraint>{
    [physicsWorld](PositionF & position, VectorF & velocity)
    {
        double t = physicsWorld->getCurrentTime() - 10;
        float onSpeed = 10;
        float stopTime = 0.3;
        if(t < 0 || t > stopTime)
        {
            position = PositionF(-1 + (t < 0 ? 0 : onSpeed * stopTime), -4, 0, Dimension::Overworld);
            velocity = VectorF(0);
            return;
        }
        position = PositionF(t * onSpeed - 1, -4, 0, Dimension::Overworld);
        velocity = VectorF(onSpeed, 0, 0);
    }})));
#else
    objects.push_back(MyObject(PhysicsObject::make(PositionF(-1, 0, 0, Dimension::Overworld), VectorF(0, 0, 0), true, false, VectorF(0.5f), PhysicsProperties(), physicsWorld)->setConstraints(vector<PhysicsConstraint>{
    [](PositionF & position, VectorF & velocity)
    {
        double t = Display::timer();
#if 1
        const float switchRate = 0.1;
        t *= switchRate;
        const float timeFraction = 0.5f;
        const float speed = (t - floor(t) < timeFraction ? M_PI : 0);
        t = (min<float>(timeFraction, t - floor(t)) + floor(t) * timeFraction) * M_PI;
        t /= switchRate;
#else
        const float speed = M_PI * (1 + sin(t / 1));
        t = M_PI * (t - 1 * cos(t / 1));
#endif
        const VectorF origin = VectorF(5 * sin(t), 0, 5 * cos(t));
        const float radius = 2;
        if(absSquared(position - origin) < radius * radius)
            return;
        position = PositionF(normalizeNoThrow(position - origin) * radius + origin, position.d);
        velocity -= VectorF(origin.z, 0, -origin.x) * speed;
        velocity -= (VectorF)(position - origin) * max(0.0f, dot(velocity, (VectorF)(position - origin))) / absSquared(position - origin);
        velocity += VectorF(origin.z, 0, -origin.x) * speed;
    }})));
#endif
    objects.push_back(MyObject(PhysicsObject::make(PositionF(5, 0, 0, Dimension::Overworld), VectorF(0, 0, 0), false, true, VectorF(0.5f, 5, 0.5f), PhysicsProperties(), physicsWorld)));
    for(size_t i = 0; i < objectCount; i++)
    {
        PositionF position;
        position.x = 0;
        position.y = (float)i / 4;
        position.z = 0;
        VectorF velocity(frand(-0.1, 0.1), frand(-0.1, 0.1), frand(-0.1, 0.1));
        velocity = VectorF(0);
        objects.push_back(MyObject(PhysicsObject::make(position, velocity, true, false, VectorF(0.1f), PhysicsProperties(0.9f), physicsWorld)));
    }
    MyObject floorObject(PhysicsObject::make(PositionF(0, -5.5, 0, Dimension::Overworld), VectorF(0, 0, 0), false, true, VectorF(5, 0.5f, 5), PhysicsProperties(), physicsWorld));
    float idealHeight = -5 + (2 * objectCount - 1) * 0.1f;
#if 0
    for(size_t i = 0; i < 100; i++)
    {
        physicsWorld->stepTime(1);
        for(MyObject o : objects)
        {
            cout << o.physicsObject->isStatic() << " " << o.physicsObject->isSupported() << " " << (VectorF)o.physicsObject->getPosition() << " " << o.physicsObject->getVelocity() << endl;
        }
    }
    cout << idealHeight << " : " << objects.back().physicsObject->getPosition().y << "\n" << flush;
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
        renderer << transform(Matrix::rotateY(physicsWorld->getCurrentTime() * M_PI / 10).concat(Matrix::translate(0, 0, -10)), theMesh);
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
