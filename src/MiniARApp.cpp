#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "AssetManager.h"
#include "MiniConfigImgui.h"

#include "Cinder-ARKit/include/CinderARKitSim.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MiniARApp : public App
{
public:

    void touchesBegan(TouchEvent event) override
    {
        // Add a point 50cm in front of camera
        mARSession.addAnchorRelativeToCamera(vec3(0.0f, 0.0f, -0.5f));
    }

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        auto aabb = am::triMesh(MESH_NAME)->calcBoundingBox();
        ARKit::mCam.lookAt(aabb.getMax() * 2.0f, aabb.getCenter());
        mCamUi = CameraUi(&ARKit::mCam, getWindow(), -1);

        //        createConfigUI({200, 200});
        createConfigImgui();
        gl::enableDepth();

        auto config = ARKit::SessionConfiguration()
            .trackingType(ARKit::TrackingType::WorldTracking)
            .planeDetection(ARKit::PlaneDetection::Horizontal);

        mARSession.runConfiguration(config);

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
            ARKit::mCam.setAspectRatio(getWindowAspectRatio());
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        mGlslProg = am::glslProg(VS_NAME, FS_NAME);
        mGlslProg->uniform("uTex0", 0);
        mGlslProg->uniform("uTex1", 1);
        mGlslProg->uniform("uTex2", 2);
        mGlslProg->uniform("uTex3", 3);

        getWindow()->getSignalDraw().connect([&] {
            //gl::setMatrices(mCam);
            gl::clear();

            gl::color(1.0f, 1.0f, 1.0f, 1.0f);
            mARSession.drawRGBCaptureTexture(getWindowBounds());

            gl::ScopedMatrices matScp;
            gl::setViewMatrix(mARSession.getViewMatrix());
            gl::setProjectionMatrix(mARSession.getProjectionMatrix());

            for (const auto& a : mARSession.getAnchors())
            {
                gl::ScopedMatrices matScp;
                gl::setModelMatrix(a.mTransform);

#if 0
                gl::ScopedTextureBind tex0(am::texture2d(TEX0_NAME), 0);
                gl::ScopedTextureBind tex1(am::texture2d(TEX1_NAME), 1);
                gl::ScopedTextureBind tex2(am::texture2d(TEX2_NAME), 2);
                gl::ScopedTextureBind tex3(am::texture2d(TEX3_NAME), 3);
                gl::ScopedGlslProg glsl(mGlslProg);

                gl::draw(am::vboMesh(MESH_NAME));
#else
                gl::ScopedGlslProg glsl(am::glslProg("color"));
                gl::drawStrokedCube(vec3(0.0f), vec3(0.02f));
#endif
            }

            for (const auto& a : mARSession.getPlaneAnchors())
            {
                gl::ScopedMatrices matScp;
                gl::setModelMatrix(a.mTransform);
                gl::translate(a.mCenter);
                gl::rotate((float)M_PI * 0.5f, vec3(1, 0, 0)); // Make it parallel with the ground
                const float xRad = a.mExtent.x * 0.5f;
                const float zRad = a.mExtent.z * 0.5f;
                gl::color(0.0f, 0.6f, 0.9f, 0.2f);
                gl::drawSolidRect(Rectf(-xRad, -zRad, xRad, zRad));
            }
        });
    }

private:
    CameraUi            mCamUi;
    gl::GlslProgRef     mGlslProg;

    ARKit::Session mARSession;
};

CINDER_APP(MiniARApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
