#ifndef OPENMW_MWRENDER_BULLETDEBUGDRAW_H
#define OPENMW_MWRENDER_BULLETDEBUGDRAW_H

#include <chrono>
#include <vector>

#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/ref_ptr>

#include <LinearMath/btIDebugDraw.h>

class btCollisionWorld;

namespace osg
{
    class Group;
    class Geometry;
}

namespace MWRender
{

    class DebugDrawer : public btIDebugDraw
    {
    private:
        struct CollisionView
        {
            btVector3 mOrig;
            btVector3 mEnd;
            std::chrono::time_point<std::chrono::steady_clock> mCreated;
            CollisionView(btVector3 orig, btVector3 normal)
                : mOrig(orig)
                , mEnd(orig + normal * 20)
                , mCreated(std::chrono::steady_clock::now())
            {
            }
        };
        std::vector<CollisionView> mCollisionViews;
        osg::ref_ptr<osg::Group> mShapesRoot;

    protected:
        osg::ref_ptr<osg::Group> mParentNode;
        btCollisionWorld* mWorld;
        osg::ref_ptr<osg::Geometry> mLinesGeometry;
        osg::ref_ptr<osg::Geometry> mTrisGeometry;
        osg::ref_ptr<osg::Vec3Array> mLinesVertices;
        osg::ref_ptr<osg::Vec3Array> mTrisVertices;
        osg::ref_ptr<osg::Vec4Array> mLinesColors;
        osg::ref_ptr<osg::DrawArrays> mLinesDrawArrays;
        osg::ref_ptr<osg::DrawArrays> mTrisDrawArrays;

        bool mDebugOn;

        void createGeometry();
        void destroyGeometry();

    public:
        DebugDrawer(osg::ref_ptr<osg::Group> parentNode, btCollisionWorld* world, int debugMode = 1);
        ~DebugDrawer();

        void step();

        void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;

        void drawTriangle(
            const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, btScalar) override;

        void addCollision(const btVector3& orig, const btVector3& normal);

        void showCollisions();

        void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime,
            const btVector3& color) override
        {
        }
        void drawSphere(btScalar radius, const btTransform& transform, const btVector3& color) override;

        void reportErrorWarning(const char* warningString) override;

        void draw3dText(const btVector3& location, const char* textString) override {}

        // 0 for off, anything else for on.
        void setDebugMode(int isOn) override;

        // 0 for off, anything else for on.
        int getDebugMode() const override;
    };

}

#endif
