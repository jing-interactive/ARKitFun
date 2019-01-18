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

class MiniARApp : public App
{
  public:

      void touchesBegan(TouchEvent event) override
      {
          mARSession.addAnchorRelativeToCamera(vec3(0.0f, 0.0f, -0.5f));
      }

    void setup() override
    {
        log::makeLogger<log::LoggerFile>();
 
#ifndef CINDER_COCOA_TOUCH
        mCamUi = CameraUi( &ARKit::mCam, getWindow(), -1 );
#endif
        auto config = ARKit::SessionConfiguration()
            .trackingType(ARKit::TrackingType::WorldTracking)
            .planeDetection(ARKit::PlaneDetection::Horizontal);

        mARSession.runConfiguration(config);

        mRootGLTF = RootGLTF::create(getAssetPath(MESH_NAME));

//        createConfigUI({200, 200});
        createConfigImgui();
        gl::enableDepth();

        getWindow()->getSignalResize().connect([&] {
            APP_WIDTH = getWindowWidth();
            APP_HEIGHT = getWindowHeight();
#ifndef CINDER_COCOA_TOUCH
            ARKit::mCam.setAspectRatio( getWindowAspectRatio() );
#endif
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });
        
        getSignalUpdate().connect([&] {
            if (mRootGLTF)
            {
                mRootGLTF->flipV = FLIP_V;
#if 0
                mRootGLTF->cameraPosition = ARKit::mCam.getEyePoint();
#endif
                mRootGLTF->update();
            }
        });

        getWindow()->getSignalDraw().connect([&] {


            gl::clear(Color(0, 0, 0));

            gl::color(1.0f, 1.0f, 1.0f, 1.0f);
            mARSession.drawRGBCaptureTexture(getWindowBounds());

            gl::ScopedMatrices matScp;
            gl::setViewMatrix(mARSession.getViewMatrix());
            gl::setProjectionMatrix(mARSession.getProjectionMatrix());

            gl::ScopedGlslProg glslProg(gl::getStockShader(gl::ShaderDef().color()));
            gl::ScopedColor colScp;
            gl::color(1.0f, 1.0f, 1.0f);

            for (const auto& a : mARSession.getAnchors())
            {
                gl::ScopedMatrices matScp;
                gl::setModelMatrix(a.mTransform);
#if 0
                gl::drawStrokedCube(vec3(0.0f), vec3(0.02f));
#else
                if (mRootGLTF)
                {
                    gl::setWireframeEnabled(WIRE_FRAME);
                    mRootGLTF->currentScene->setScale(MESH_SCALE);
                    mRootGLTF->currentScene->setRotation(mMeshRotation);
                    mRootGLTF->draw();
                    gl::disableWireframe();
                }
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
    ARKit::Session      mARSession;

    RootGLTFRef mRootGLTF;
    RootObjRef mRootObj;
    quat mMeshRotation;
};

CINDER_APP( MiniARApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    settings->setWindowSize(APP_WIDTH, APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
} )
