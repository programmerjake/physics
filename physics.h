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
#include <cmath>

using namespace std;

class PhysicsWorld;

struct PhysicsProperties final
{
    float bounceFactor, slideFactor;
    PhysicsProperties(float bounceFactor = sqrt(0.5f), float slideFactor = 1 - sqrt(0.5f))
        : bounceFactor(limit(bounceFactor, 0.0f, 1.0f)), slideFactor(limit(slideFactor, 0.0f, 1.0f))
    {
    }
};

typedef function<void(PositionF & position, VectorF & velocity)> PhysicsConstraint;

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
    PhysicsProperties properties;
    shared_ptr<const vector<PhysicsConstraint>> constraints;
    PhysicsObject(const PhysicsObject &) = delete;
    const PhysicsObject & operator =(const PhysicsObject &) = delete;
    PhysicsObject(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world, PhysicsProperties properties);
public:
    static shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, PhysicsProperties properties, shared_ptr<PhysicsWorld> world);
    ~PhysicsObject();
    PositionF getPosition() const;
    VectorF getVelocity() const;
    shared_ptr<PhysicsObject> setConstraints(shared_ptr<const vector<PhysicsConstraint>> constraints = nullptr)
    {
        this->constraints = constraints;
        return shared_from_this();
    }
    shared_ptr<PhysicsObject> setConstraints(const vector<PhysicsConstraint> &constraints)
    {
        this->constraints = make_shared<vector<PhysicsConstraint>>(constraints);
        return shared_from_this();
    }
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
    const PhysicsProperties & getProperties() const
    {
        return properties;
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
    static constexpr float distanceEPS = 20 * eps;
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

inline PhysicsObject::PhysicsObject(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, shared_ptr<PhysicsWorld> world, PhysicsProperties properties)
    : position{position, position}, velocity{velocity, velocity}, objectTime{world->getCurrentTime(), world->getCurrentTime()}, affectedByGravity(affectedByGravity), isStatic_(isStatic), extents(extents), world(world)
{
}

inline shared_ptr<PhysicsObject> PhysicsObject::make(PositionF position, VectorF velocity, bool affectedByGravity, bool isStatic, VectorF extents, PhysicsProperties properties, shared_ptr<PhysicsWorld> world)
{
    shared_ptr<PhysicsObject> retval = shared_ptr<PhysicsObject>(new PhysicsObject(position, velocity, affectedByGravity, isStatic, extents, world, properties));
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
    //cout << "new position : " << (VectorF)newPosition << " : new velocity : " << newVelocity << endl;
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
    float stepDuration = 1 / 30.0f;
    size_t stepCount = (size_t)ceil((stopTime - currentTime) / stepDuration - timeEPS);
    for(size_t i = 1; i <= stepCount; i++)
    {
        if(i >= stepCount)
            currentTime = stopTime;
        else
            currentTime += stepDuration;
        bool anyCollisions = true;
        for(size_t i = 0; i < 10 && anyCollisions; i++)
        {
            anyCollisions = false;
            vector<shared_ptr<PhysicsObject>> objectsVector(objects.begin(), objects.end());
            vector<pair<float, shared_ptr<PhysicsObject>>> temporaryObjectsVector;
            temporaryObjectsVector.resize(objectsVector.size());
            for(size_t i = 0; i < temporaryObjectsVector.size(); i++)
                temporaryObjectsVector[i] = make_pair(objectsVector[i]->getPosition().y - objectsVector[i]->getExtents().y, objectsVector[i]);
            sort(temporaryObjectsVector.begin(), temporaryObjectsVector.end(), [](pair<float, shared_ptr<PhysicsObject>> a, pair<float, shared_ptr<PhysicsObject>> b)
            {
                return get<0>(a) < get<0>(b);
            });
            for(size_t i = 0; i < temporaryObjectsVector.size(); i++)
                objectsVector[i] = get<1>(temporaryObjectsVector[i]);
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
                for(auto j = objectsVector.begin(); j != i; j++)
                {
                    shared_ptr<PhysicsObject> objectB = *j;
                    if(!objectB || objectB->isDestroyed())
                        continue;
                    bool supported = objectA->isSupportedBy(*objectB);
                    if(supported)
                    {
                        objectA->supported = true;
                    }
                }
            }
            constexpr size_t xScaleFactor = 5, zScaleFactor = 5;
            constexpr size_t bigHashPrime = 14713, smallHashPrime = 91;
            struct HashNode final
            {
                HashNode * hashNext;
                int x, z;
                shared_ptr<PhysicsObject> object;
            };
            array<HashNode *, bigHashPrime> overallHashTable;
            overallHashTable.fill(nullptr);
            static thread_local HashNode * freeListHead = nullptr;
            vector<shared_ptr<PhysicsObject>> collideObjectsList;
            collideObjectsList.reserve(objects.size());
            for(auto i = objects.begin(); i != objects.end();)
            {
                shared_ptr<PhysicsObject> o = *i;
                if(!o || o->isDestroyed())
                {
                    i = objects.erase(i);
                    continue;
                }
                o->setupNewState();
                PositionF position = o->getPosition();
                VectorF extents = o->getExtents();
                float fMinX = position.x - extents.x;
                float fMaxX = position.x + extents.x;
                int minX = ifloor(fMinX * xScaleFactor);
                int maxX = iceil(fMaxX * xScaleFactor);
                float fMinZ = position.z - extents.z;
                float fMaxZ = position.z + extents.z;
                int minZ = ifloor(fMinZ * zScaleFactor);
                int maxZ = iceil(fMaxZ * zScaleFactor);
                if((size_t)(maxZ - minZ) * (size_t)(maxX * minX) > (size_t)(xScaleFactor + 1) * (size_t)(zScaleFactor + 1))
                {
                    collideObjectsList.push_back(o);
                }
                else
                {
                    for(int xPosition = minX; xPosition <= maxX; xPosition++)
                    {
                        for(int zPosition = minZ; zPosition <= maxZ; zPosition++)
                        {
                            HashNode * node = freeListHead;
                            if(node != nullptr)
                                freeListHead = freeListHead->hashNext;
                            else
                                node = new HashNode;
                            size_t hash = (size_t)(xPosition * 8191 + zPosition) % bigHashPrime;
                            node->hashNext = overallHashTable.at(hash);
                            node->x = xPosition;
                            node->z = zPosition;
                            node->object = o;
                            overallHashTable.at(hash) = node;
                        }
                    }
                }
                i++;
            }
            size_t startCollideObjectsListSize = collideObjectsList.size();
            for(shared_ptr<PhysicsObject> objectA : objects)
            {
                if(objectA->isStatic())
                    continue;
                collideObjectsList.resize(startCollideObjectsListSize);
                PositionF position = objectA->getPosition();
                VectorF extents = objectA->getExtents();
                float fMinX = position.x - extents.x;
                float fMaxX = position.x + extents.x;
                int minX = ifloor(fMinX * xScaleFactor);
                int maxX = iceil(fMaxX * xScaleFactor);
                float fMinZ = position.z - extents.z;
                float fMaxZ = position.z + extents.z;
                int minZ = ifloor(fMinZ * zScaleFactor);
                int maxZ = iceil(fMaxZ * zScaleFactor);
                array<HashNode *, smallHashPrime> perObjectHashTable;
                perObjectHashTable.fill(nullptr);
                for(int xPosition = minX; xPosition <= maxX; xPosition++)
                {
                    for(int zPosition = minZ; zPosition <= maxZ; zPosition++)
                    {
                        size_t hash = (size_t)(xPosition * 8191 + zPosition);
                        hash %= bigHashPrime;
                        HashNode * node = overallHashTable.at(hash);
                        while(node != nullptr)
                        {
                            if(node->x == xPosition && node->z == zPosition) // found one
                            {
                                size_t perObjectHash = std::hash<shared_ptr<PhysicsObject>>()(node->object) % smallHashPrime;
                                HashNode ** pnode = &perObjectHashTable.at(perObjectHash);
                                HashNode * node2 = *pnode;
                                bool found = false;
                                while(node2 != nullptr)
                                {
                                    if(node2->object == node->object)
                                    {
                                        found = true;
                                        break;
                                    }
                                    pnode = &node2->hashNext;
                                    node2 = *pnode;
                                }
                                if(!found)
                                {
                                    node2 = freeListHead;
                                    if(node2 == nullptr)
                                        node2 = new HashNode;
                                    else
                                        freeListHead = node2->hashNext;
                                    node2->hashNext = perObjectHashTable.at(perObjectHash);
                                    node2->object = node->object;
                                    node2->x = node2->z = 0;
                                    perObjectHashTable.at(perObjectHash) = node2;
                                    collideObjectsList.push_back(node->object);
                                }
                            }
                            node = node->hashNext;
                        }
                    }
                }
                for(HashNode * node : perObjectHashTable)
                {
                    while(node != nullptr)
                    {
                        HashNode * nextNode = node->hashNext;
                        node->hashNext = freeListHead;
                        freeListHead = node;
                        node = nextNode;
                    }
                }
                for(auto objectB : collideObjectsList)
                {
                    if(objectA != objectB && objectA->collides(*objectB))
                    {
                        anyCollisions = true;
                        objectA->adjustPosition(*objectB);
                        //cout << "collision" << endl;
                    }
                }
                if(objectA->constraints)
                {
                    for(PhysicsConstraint constraint : *objectA->constraints)
                    {
                        if(constraint)
                            constraint(objectA->position[getNewVariableSetIndex()], objectA->velocity[getNewVariableSetIndex()]);
                    }
                }
            }
            for(HashNode * node : overallHashTable)
            {
                while(node != nullptr)
                {
                    HashNode * nextNode = node->hashNext;
                    node->hashNext = freeListHead;
                    freeListHead = node;
                    node = nextNode;
                }
            }
            swapVariableSetIndex();
        }
    }
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
        aVelocity -= ((1 + properties.bounceFactor * rt.properties.bounceFactor) * dot(deltaVelocity, normal) * normal + (1 - properties.slideFactor) * (1 - rt.properties.slideFactor) * (deltaVelocity - normal * dot(deltaVelocity, normal))) * interpolationT;
    else
        aVelocity = interpolate(0.5f, aVelocity, bVelocity);
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
    VectorF extentsSum = extents + rt.extents;
    VectorF deltaPosition = aPosition - bPosition;
    if(deltaPosition.x + PhysicsWorld::distanceEPS > -extentsSum.x && deltaPosition.x - PhysicsWorld::distanceEPS < extentsSum.x &&
       deltaPosition.z + PhysicsWorld::distanceEPS > -extentsSum.z && deltaPosition.z - PhysicsWorld::distanceEPS < extentsSum.z)
    {
        if(deltaPosition.y > 0)
        {
            if(deltaPosition.y < PhysicsWorld::distanceEPS * 4 + extentsSum.y)
            {
                return true;
            }
        }
    }
    return false;
}

#endif // PHYSICS_OBJECT_H_INCLUDED
