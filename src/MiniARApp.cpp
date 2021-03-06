#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "AssetManager.h"
#include "MiniConfigImgui.h"

#include "Cinder-MeshViewer/include/CinderMeshViewer.h"
#include "Cinder-ARKit/include/CinderARKitSim.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined(CINDER_GL_ES)
namespace cinder
{
namespace gl
{
void enableWireframe() {}
void disableWireframe() {}
void setWireframeEnabled(bool enable = true)
{
    if (enable)
        enableWireframe();
    else
        disableWireframe();
}
} // namespace gl
} // namespace cinder
#endif

struct MiniARApp : public App
{
    void setup()
    {
        log::makeLogger<log::LoggerFile>();

#ifndef CINDER_COCOA_TOUCH
        mCamUi = CameraUi(&ARKit::mCam, getWindow(), -1);
#endif
        auto config = ARKit::SessionConfiguration()
                          .trackingType(ARKit::TrackingType::WorldTracking)
                          .planeDetection(ARKit::PlaneDetection::Horizontal);

        mARSession.runConfiguration(config);

        mRootGLTF = RootGLTF::create(getAssetPath("new_balance_997/scene.gltf"));

        //        createConfigUI({200, 200});
        createConfigImgui();
        gl::enableDepth();

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
#ifndef CINDER_COCOA_TOUCH
            ARKit::mCam.setAspectRatio(getWindowAspectRatio());
#endif
        });

        getWindow()->getSignalMouseUp().connect([&](MouseEvent &event) {
            mARSession.addAnchorRelativeToCamera(vec3(0.0f, 0.0f, -0.5f));
        });
        getWindow()->getSignalKeyUp().connect([&](KeyEvent &event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE)
                quit();
        });

        getSignalUpdate().connect([&] {
            if (mRootGLTF)
            {
                mRootGLTF->flipV = FLIP_V;
#ifndef CINDER_COCOA_TOUCH
                // TODO: fix light dir
                auto viewMat = mARSession.getViewMatrix();
                mRootGLTF->cameraPosition = viewMat[3];
#endif
                mRootGLTF->update();
            }
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::clear(Color(0, 0, 0));

            {
                gl::ScopedDepthWrite zWrite(false);
                gl::color(1.0f, 1.0f, 1.0f, 1.0f);
                mARSession.drawRGBCaptureTexture(getWindowBounds());
            }

            gl::ScopedMatrices matScp;
            gl::setViewMatrix(mARSession.getViewMatrix());
            gl::setProjectionMatrix(mARSession.getProjectionMatrix());

            for (const auto &a : mARSession.getAnchors())
            {
#if 0
                gl::ScopedMatrices matScp;
                gl::setModelMatrix(a.mTransform);
                gl::ScopedGlslProg glslProg(am::glslProg("color"));
                gl::ScopedColor colScp;
                gl::color(1.0f, 1.0f, 1.0f);
                gl::drawStrokedCube(vec3(0.0f), vec3(0.02f));
#else
                if (mRootGLTF)
                {
                    MESH_SCALE = 0.001f;
                    mat4 transform = a.mTransform;
                    transform *= glm::scale(vec3(MESH_SCALE, MESH_SCALE, MESH_SCALE));
                    //                    transform *= glm::toMat4(mMeshRotation);
                    mRootGLTF->currentScene->setTransform(transform);
                    mRootGLTF->draw();
                }
#endif
            }

            int idx = 0;
            for (const auto &a : mARSession.getPlaneAnchors())
            {
                gl::ScopedGlslProg glslProg(am::glslProg("color"));
                gl::ScopedMatrices matScp;

                if (idx == 0)
                {
                    if (mRootGLTF)
                    {
                        MESH_SCALE = 0.001f;
                        mat4 transform = a.mTransform;
                        transform *= glm::translate(a.mCenter);
                        //transform *= glm::toMat4(mMeshRotation);
                        //transform *= glm::toMat4((float)M_PI * 0.5f, vec3(1, 0, 0));
                        transform *= glm::scale(vec3(MESH_SCALE, MESH_SCALE, MESH_SCALE));
                        mRootGLTF->currentScene->setTransform(transform);
                        mRootGLTF->draw();
                    }
                }
                else
                {
                    gl::setModelMatrix(a.mTransform);
                    gl::translate(a.mCenter);
                    gl::rotate((float)M_PI * 0.5f, vec3(1, 0, 0)); // Make it parallel with the ground

                    const float xRad = a.mExtent.x * 0.5f;
                    const float zRad = a.mExtent.z * 0.5f;
                    gl::color(0.0f, 0.6f, 0.9f, 0.2f);
                    gl::drawSolidRect(Rectf(-xRad, -zRad, xRad, zRad));
                }

                idx++;
            }
        });
    }

    CameraUi mCamUi;
    gl::GlslProgRef mGlslProg;
    ARKit::Session mARSession;

    RootGLTFRef mRootGLTF;
    RootObjRef mRootObj;
    quat mMeshRotation;
};

CINDER_APP(MiniARApp, RendererGl, [](App::Settings *settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
