#ifndef PHYSICS_OBJECT_H_INCLUDED
#define PHYSICS_OBJECT_H_INCLUDED

#include <memory>
#include "matrix.h"
#include "position.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <functional>
#include <iostream>

using namespace std;

class PhysicsWorld;

class PhysicsObject final : public enable_shared_from_this<PhysicsObject>
{
    friend class PhysicsWorld;
private:
    PositionF position[2];
    VectorF velocity[2];
    double objectTime[2];
    bool affectedByGravity;
    bool isStatic_;
    bool supported;
    bool destroyed = false;
    VectorF extents;
    weak_ptr<PhysicsWorld> world;
    uint64_t latestUpdateTag = 0;
    size_t newStateCount = 0;
    PhysicsObject(const PhysicsObject &) = delete;
    const PhysicsObject & operator =(const PhysicsObject &) = delete;
    PhysicsObject(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world);
public:
    static shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world);
    ~PhysicsObject();
    PositionF getPosition() const;
    VectorF getVelocity() const;
    bool isAffectedByGravity() const
    {
        return affectedByGravity;
    }
    bool isSupported() const
    {
        return supported;
    }
    bool isStatic() const
    {
        return isStatic_;
    }
    bool isDestroyed() const
    {
        return destroyed;
    }
    void destroy()
    {
        destroyed = true;
    }
    VectorF getExtents() const
    {
        return extents;
    }
    shared_ptr<PhysicsWorld> getWorld() const
    {
        return world.lock();
    }
    void setNewState(PositionF newPosition, VectorF newVelocity);
    void setupNewState();
    bool collides(const PhysicsObject & rt) const;
    double getNextCollisionTime(const PhysicsObject & rt) const;
    void adjustPosition(const PhysicsObject & rt);
    bool isSupportedBy(const PhysicsObject & rt) const;
};

class PhysicsWorld final : public enable_shared_from_this<PhysicsWorld>
{
    friend class PhysicsObject;
private:
    double currentTime = 0;
    int variableSetIndex = 0;
public:
    static constexpr float distanceEPS = 10 * eps;
    static constexpr float timeEPS = eps;
    inline double getCurrentTime() const
    {
        return currentTime;
    }
    inline int getOldVariableSetIndex() const
    {
        return variableSetIndex;
    }
    inline int getNewVariableSetIndex() const
    {
        return 1 - variableSetIndex;
    }
private:
    unordered_set<shared_ptr<PhysicsObject>> objects;
    void addObject(shared_ptr<PhysicsObject> o)
    {
        objects.insert(o);
    }
    void removeObject(shared_ptr<PhysicsObject> o)
    {
        objects.erase(o);
    }
    struct CollisionEvent final
    {
        double collisionTime;
        weak_ptr<PhysicsObject> a, b;
        uint64_t aTag, bTag;
        CollisionEvent(double collisionTime, shared_ptr<PhysicsObject> a, shared_ptr<PhysicsObject> b)
            : collisionTime(collisionTime), a(a), b(b), aTag(a->latestUpdateTag), bTag(b->latestUpdateTag)
        {
        }
        bool operator ==(const CollisionEvent & rt) const
        {
            if(collisionTime != rt.collisionTime)
                return false;
            if(aTag == rt.aTag && a.lock() == rt.a.lock() && bTag == rt.bTag && b.lock() == rt.b.lock())
                return true;
            if(aTag == rt.bTag && a.lock() == rt.b.lock() && bTag == rt.aTag && b.lock() == rt.a.lock())
                return true;
            return false;
        }
        bool operator !=(const CollisionEvent & rt) const
        {
            return !operator ==(rt);
        }
    };
    struct CollisionEventHash final
    {
        size_t operator()(const CollisionEvent & ce) const
        {
            return hash<double>()(ce.collisionTime) + (size_t)ce.aTag + (size_t)ce.bTag;
        }
    };
    struct CollisionEventCompare final
    {
        bool operator()(const CollisionEvent & a, const CollisionEvent & b) const
        {
            return a.collisionTime < b.collisionTime;
        }
    };
    priority_queue<CollisionEvent, vector<CollisionEvent>, CollisionEventCompare> eventsQueue;
    unordered_set<CollisionEvent, CollisionEventHash> eventsSet;
    unordered_map<intptr_t, weak_ptr<PhysicsObject>> changedObjects;
    void swapVariableSetIndex()
    {
        variableSetIndex = (variableSetIndex != 0 ? 0 : 1);
    }
public:
    void runToTime(double stopTime);
    void stepTime(double deltaTime)
    {
        runToTime(deltaTime + getCurrentTime());
    }
};

inline PhysicsObject::PhysicsObject(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world)
    : position{position, position}, velocity{velocity, velocity}, objectTime{world->getCurrentTime(), world->getCurrentTime()}, affectedByGravity(affectedByGravity), isStatic_(isStatic), extents(extents), world(world)
{
}

inline shared_ptr<PhysicsObject> PhysicsObject::make(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world)
{
    shared_ptr<PhysicsObject> retval = shared_ptr<PhysicsObject>(new PhysicsObject(position, velocity, affectedByGravity, isStatic, extents, world));
    world->objects.insert(retval);
    world->changedObjects[(intptr_t)retval.get()] = retval;
    return retval;
}

inline PhysicsObject::~PhysicsObject()
{
}

inline PositionF PhysicsObject::getPosition() const
{
    auto world = getWorld();
    int variableSetIndex = world->getOldVariableSetIndex();
    float deltaTime = world->getCurrentTime() - objectTime[variableSetIndex];
    if(affectedByGravity && !isSupported())
        return position[variableSetIndex] + deltaTime * velocity[variableSetIndex] + 0.5f * deltaTime * deltaTime * gravityVector;
    return position[variableSetIndex] + deltaTime * velocity[variableSetIndex];
}

inline VectorF PhysicsObject::getVelocity() const
{
    auto world = getWorld();
    int variableSetIndex = world->getOldVariableSetIndex();
    if(!affectedByGravity || isSupported())
        return velocity[variableSetIndex];
    float deltaTime = world->getCurrentTime() - objectTime[variableSetIndex];
    return velocity[variableSetIndex] + deltaTime * gravityVector;
}

inline void PhysicsObject::setNewState(PositionF newPosition, VectorF newVelocity)
{
    auto world = getWorld();
    int variableSetIndex = world->getNewVariableSetIndex();
    objectTime[variableSetIndex] = world->getCurrentTime();
    newPosition += position[variableSetIndex] * newStateCount;
    newVelocity += velocity[variableSetIndex] * newStateCount;
    newStateCount++;
    newPosition /= newStateCount;
    newVelocity /= newStateCount;
    cout << "new position : " << (VectorF)newPosition << " : new velocity : " << newVelocity << endl;
    position[variableSetIndex] = newPosition;
    velocity[variableSetIndex] = newVelocity;
    world->changedObjects[(intptr_t)this] = shared_from_this();
    latestUpdateTag++;
}

inline void PhysicsObject::setupNewState()
{
    auto world = getWorld();
    int oldVariableSetIndex = world->getOldVariableSetIndex();
    int newVariableSetIndex = world->getNewVariableSetIndex();
    objectTime[newVariableSetIndex] = objectTime[oldVariableSetIndex];
    position[newVariableSetIndex] = position[oldVariableSetIndex];
    velocity[newVariableSetIndex] = velocity[oldVariableSetIndex];
    newStateCount = 0;
}

inline bool PhysicsObject::collides(const PhysicsObject & rt) const
{
    auto world = getWorld();
    assert(world == rt.getWorld());
    PositionF lPosition = getPosition();
    PositionF rPosition = rt.getPosition();
    if(lPosition.d != rPosition.d)
        return false;
    VectorF lExtents = extents;
    VectorF rExtents = rt.extents;
    if(lPosition.x - lExtents.x - PhysicsWorld::distanceEPS > rPosition.x + rExtents.x)
        return false;
    if(rPosition.x - rExtents.x - PhysicsWorld::distanceEPS > lPosition.x + lExtents.x)
        return false;
    if(lPosition.y - lExtents.y - PhysicsWorld::distanceEPS > rPosition.y + rExtents.y)
        return false;
    if(rPosition.y - rExtents.y - PhysicsWorld::distanceEPS > lPosition.y + lExtents.y)
        return false;
    if(lPosition.z - lExtents.z - PhysicsWorld::distanceEPS > rPosition.z + rExtents.z)
        return false;
    if(rPosition.z - rExtents.z - PhysicsWorld::distanceEPS > lPosition.z + lExtents.z)
        return false;
    return true;
}

inline double PhysicsObject::getNextCollisionTime(const PhysicsObject & rt) const
{
    auto world = getWorld();
    assert(world == rt.getWorld());
    PositionF lPosition = getPosition();
    PositionF rPosition = rt.getPosition();
    if(lPosition.d != rPosition.d)
        return -1;
    VectorF lExtents = extents;
    VectorF rExtents = rt.extents;
    if(collides(rt))
        return world->getCurrentTime();
    VectorF lVelocity = getVelocity();
    VectorF rVelocity = rt.getVelocity();
    VectorF relativeAcceleration = VectorF(0);
    if(isAffectedByGravity() && !isSupported())
        relativeAcceleration += gravityVector;
    if(rt.isAffectedByGravity() && !rt.isSupported())
        relativeAcceleration -= gravityVector;
    array<float, 12> collisions;
    size_t usedCount = 0;
    VectorF quadratic = 0.5f * relativeAcceleration;
    VectorF linear = lVelocity - rVelocity;
    VectorF constant1 = lPosition - rPosition - (lExtents + rExtents);
    VectorF constant2 = lPosition - rPosition + (lExtents + rExtents);
    usedCount += solveQuadratic(constant1.x, linear.x, quadratic.x, &collisions[usedCount]);
    usedCount += solveQuadratic(constant1.y, linear.y, quadratic.y, &collisions[usedCount]);
    usedCount += solveQuadratic(constant1.z, linear.z, quadratic.z, &collisions[usedCount]);
    usedCount += solveQuadratic(constant2.x, linear.x, quadratic.x, &collisions[usedCount]);
    usedCount += solveQuadratic(constant2.y, linear.y, quadratic.y, &collisions[usedCount]);
    usedCount += solveQuadratic(constant2.z, linear.z, quadratic.z, &collisions[usedCount]);
    sort(collisions.begin(), collisions.begin() + usedCount);
    for(auto t : collisions)
    {
        if(t < PhysicsWorld::timeEPS)
            continue;
        VectorF v1 = linear * t + t * t * quadratic + constant1;
        VectorF v2 = linear * t + t * t * quadratic + constant2;
        if(v1.x < PhysicsWorld::distanceEPS && v1.y < PhysicsWorld::distanceEPS && v1.z < PhysicsWorld::distanceEPS && v2.x > -PhysicsWorld::distanceEPS && v2.y > -PhysicsWorld::distanceEPS && v2.z > -PhysicsWorld::distanceEPS)
            return t + world->getCurrentTime();
    }
    return -1;
}

inline void PhysicsWorld::runToTime(double stopTime)
{
    vector<CollisionEvent> deletedEvents;
    for(;;)
    {
        vector<shared_ptr<PhysicsObject>> objectsVector(objects.begin(), objects.end());
        sort(objectsVector.begin(), objectsVector.end(), [](shared_ptr<PhysicsObject> a, shared_ptr<PhysicsObject> b)
        {
            return a->getPosition().y - a->getExtents().y < b->getPosition().y - b->getExtents().y;
        });
        for(auto i = objectsVector.begin(); i != objectsVector.end(); i++)
        {
            shared_ptr<PhysicsObject> objectA = *i;
            if(!objectA || objectA->isDestroyed())
                continue;
            objectA->position[getOldVariableSetIndex()] = objectA->getPosition();
            objectA->velocity[getOldVariableSetIndex()] = objectA->getVelocity();
            objectA->objectTime[getOldVariableSetIndex()] = currentTime;
            objectA->supported = false;
            if(objectA->isStatic())
            {
                objectA->supported = true;
                continue;
            }
            VectorF & velocity = objectA->velocity[getOldVariableSetIndex()];
            for(auto j = objectsVector.begin(); j != i; j++)
            {
                shared_ptr<PhysicsObject> objectB = *j;
                if(!objectB || objectB->isDestroyed())
                    continue;
                bool supported = objectA->isSupportedBy(*objectB);
                if(supported)
                {
                    objectA->supported = true;
                    //velocity.y = max(velocity.y, objectB->getVelocity().y);
                }
            }
            cout << (objectA->supported ? "true " : "false ") << velocity << endl;
        }
        while(!changedObjects.empty())
        {
            shared_ptr<PhysicsObject> objectA = shared_ptr<PhysicsObject>(get<1>(*changedObjects.begin()));
            changedObjects.erase(changedObjects.begin());
            if(!objectA || objectA->isDestroyed())
                continue;
            for(auto i = objects.begin(); i != objects.end();)
            {
                shared_ptr<PhysicsObject> objectB = *i;
                if(objectA == objectB)
                {
                    i++;
                    continue;
                }
                if(!objectB || objectB->isDestroyed())
                {
                    i = objects.erase(i);
                    continue;
                }
                if(changedObjects.find((intptr_t)objectB.get()) != changedObjects.end())
                {
                    i++;
                    continue;
                }
                double t = objectA->getNextCollisionTime(*objectB);
                if(t > 0)
                {
                    //cout << "New Collision Event : " << t << endl;
                    CollisionEvent event(t, objectA, objectB);
                    if(get<1>(eventsSet.insert(event)))
                        eventsQueue.push(event);
                }
                i++;
            }
        }
        for(auto i = objects.begin(); i != objects.end(); i++)
        {
            shared_ptr<PhysicsObject> o = *i;
            o->setupNewState();
        }

        if(eventsQueue.empty())
            break;
        CollisionEvent event = eventsQueue.top();
        if(event.collisionTime > stopTime)
            break;
        do
        {
            event = eventsQueue.top();
            eventsQueue.pop();
            deletedEvents.push_back(event);
            shared_ptr<PhysicsObject> objectA(event.a), objectB(event.b);
            if(objectA && objectB && !objectA->isDestroyed() && !objectB->isDestroyed())
            {
                if(event.aTag == objectA->latestUpdateTag && event.bTag == objectB->latestUpdateTag)
                {
                    currentTime = event.collisionTime;
                    cout << currentTime << endl;
                    objectA->adjustPosition(*objectB);
                    objectB->adjustPosition(*objectA);
                }
            }
        }
        while(!eventsQueue.empty() && eventsQueue.top().collisionTime < event.collisionTime + timeEPS);
        swapVariableSetIndex();
    }
    currentTime = stopTime;
    for(auto e : deletedEvents)
    {
        eventsSet.erase(e);
    }
    cout << currentTime << endl;
}

inline void PhysicsObject::adjustPosition(const PhysicsObject & rt)
{
    if(isStatic())
        return;
    PositionF aPosition = getPosition();
    PositionF bPosition = rt.getPosition();
    VectorF aVelocity = getVelocity();
    VectorF bVelocity = rt.getVelocity();
    VectorF deltaPosition = aPosition - bPosition;
    VectorF AbsDeltaPosition = VectorF(abs(deltaPosition.x), abs(deltaPosition.y), abs(deltaPosition.z));
    VectorF extentsSum = getExtents() + rt.getExtents();
    VectorF surfaceOffset = extentsSum - AbsDeltaPosition + VectorF(PhysicsWorld::distanceEPS * 2);
    VectorF deltaVelocity = aVelocity - bVelocity;
    float interpolationT = 0.5f;
    if(rt.isStatic())
        interpolationT = 1.0f;
    float interpolationTY = interpolationT;
    if(rt.isSupported())
        interpolationTY = 1.0f;
    VectorF normal(0);
    if(deltaPosition.x == 0)
        deltaPosition.x = PhysicsWorld::distanceEPS;
    if(deltaPosition.y == 0)
        deltaPosition.y = PhysicsWorld::distanceEPS;
    if(deltaPosition.z == 0)
        deltaPosition.z = PhysicsWorld::distanceEPS;
    if(surfaceOffset.x < surfaceOffset.y && surfaceOffset.x < surfaceOffset.z)
    {
        normal.x = sgn(deltaPosition.x);
        aPosition.x += interpolationT * normal.x * surfaceOffset.x;
    }
    else if(surfaceOffset.y < surfaceOffset.z)
    {
        normal.y = sgn(deltaPosition.y);
        aPosition.y += interpolationTY * normal.y * surfaceOffset.y;
    }
    else
    {
        normal.z = sgn(deltaPosition.z);
        aPosition.z += interpolationT * normal.z * surfaceOffset.z;
    }
    if(dot(deltaVelocity, normal) < 0)
        aVelocity -= 1.5f * dot(deltaVelocity, normal) * normal * interpolationT;
    setNewState(aPosition, aVelocity);
}

bool PhysicsObject::isSupportedBy(const PhysicsObject & rt) const
{
    if(isStatic())
        return true;
    if(!rt.isSupported() && !rt.isStatic())
        return false;
    PositionF aPosition = getPosition();
    PositionF bPosition = rt.getPosition();
    if(aPosition.d != bPosition.d)
        return false;
    VectorF aVelocity = getVelocity();
    VectorF bVelocity = rt.getVelocity();
    VectorF extentsSum = extents + rt.extents;
    VectorF deltaPosition = aPosition - bPosition;
    if(deltaPosition.x + PhysicsWorld::distanceEPS > -extentsSum.x && deltaPosition.x - PhysicsWorld::distanceEPS < extentsSum.x &&
       deltaPosition.z + PhysicsWorld::distanceEPS > -extentsSum.z && deltaPosition.z - PhysicsWorld::distanceEPS < extentsSum.z)
    {
        if(deltaPosition.y > 0)
        {
            if(deltaPosition.y < PhysicsWorld::distanceEPS * 4 + extentsSum.y)
            {
                if(aVelocity.y - PhysicsWorld::distanceEPS < bVelocity.y)
                    return true;
            }
        }
    }
    return false;
}

#endif // PHYSICS_OBJECT_H_INCLUDED
