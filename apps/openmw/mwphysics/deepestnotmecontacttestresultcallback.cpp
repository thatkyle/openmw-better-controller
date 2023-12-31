#include "deepestnotmecontacttestresultcallback.hpp"

#include <algorithm>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "collisiontype.hpp"

namespace MWPhysics
{

    DeepestNotMeContactTestResultCallback::DeepestNotMeContactTestResultCallback(
        const btCollisionObject* me, const std::vector<const btCollisionObject*>& targets, const btVector3& origin)
        : mMe(me)
        , mTargets(targets)
        , mOrigin(origin)
        , mLeastDistSqr(std::numeric_limits<float>::max())
    {
    }

    btScalar DeepestNotMeContactTestResultCallback::addSingleResult(btManifoldPoint& cp,
        const btCollisionObjectWrapper* col0Wrap, int partId0, int index0, const btCollisionObjectWrapper* col1Wrap,
        int partId1, int index1)
    {
        const btCollisionObject* collisionObject = col1Wrap->m_collisionObject;
        if (collisionObject != mMe)
        {
            if (collisionObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor
                && !mTargets.empty())
            {
                if ((std::find(mTargets.begin(), mTargets.end(), collisionObject) == mTargets.end()))
                    return 0.f;
            }

            btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
            if (!mObject || distsqr < mLeastDistSqr)
            {
                mObject = collisionObject;
                mLeastDistSqr = distsqr;
                mContactPoint = cp.getPositionWorldOnA();
                mContactNormal = cp.m_normalWorldOnB;
            }
        }

        return 0.f;
    }
}
