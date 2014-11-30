#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Rand.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/Vbo.h"
#include "CameraStereoHMD.h"
#include "OculusVR.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class oculusBaseApp : public AppNative {
public:
	void setup();
	void mouseDown( MouseEvent event );
    void keyDown( KeyEvent event );
    void prepareSettings( Settings* settings );
	void update();
    void render();
	void draw();
    
    ovr::DeviceRef              mOculusVR;
    ovr::DistortionHelperRef    mDistortionHelper;
    CameraStereoHMD             mCamera;
    gl::FboRef                  mFbo;
    gl::TextureFontRef          mFont;
};

void oculusBaseApp::prepareSettings( Settings* settings )
{
    // If more than one Display use the second
    // one for the Oculus
    if( Display::getDisplays().size() > 1 )
    {
        settings->setDisplay( Display::getDisplays()[1] );
    }
    
    settings->disableFrameRate();
    // We need a fullscreen window for the Oculus
    settings->setFullScreen();
 }

void oculusBaseApp::setup()
{
    mOculusVR = ovr::Device::create();
    mDistortionHelper = ovr::DistortionHelper::create( false );
    
    mFont = gl::TextureFont::create(Font("Helvetica-Bold", 72));
    
    //set up Oculus screen video
    gl::Fbo::Format format;
    format.enableDepthBuffer();
    format.setSamples( 2 );
    mFbo = gl::Fbo::create( 1600, 1000, format );
    mCamera = CameraStereoHMD( 640, 800,
                               mOculusVR ? mOculusVR->getFov() : 125,
                               mOculusVR ? mOculusVR->getEyeToScreenDistance() : 0.1f,
                               10000.0f );
    mCamera.lookAt(vec3(0,0,10), vec3(0,0,0), vec3( 0, 1, 0 ));
    
    // Make the stereo a bit stronger
    mCamera.setEyeSeparation( 1.5f );    
    
}

void oculusBaseApp::keyDown(cinder::app::KeyEvent event)
{
    if( event.getChar() =='a' )
    {
        mCamera.setEyeSeparation( mCamera.getEyeSeparation() - 0.1f );
        console() << "Eye separation: " << mCamera.getEyeSeparation() << "\n";
    }
    else if( event.getChar() =='s' )
    {
        mCamera.setEyeSeparation( mCamera.getEyeSeparation() + 0.1f );
        console() << "Eye separation: " << mCamera.getEyeSeparation() << "\n";
    }
    else if( event.getChar() =='z' )
    {
        mCamera.setProjectionCenterOffset( mCamera.getProjectionCenterOffset() - 1.1f );
        console() << "Eye ProjectionCenterOffset: " << mCamera.getProjectionCenterOffset() << "\n";
    }
    else if( event.getChar() =='x' )
    {
        mCamera.setProjectionCenterOffset( mCamera.getProjectionCenterOffset() + 1.1f );
        console() << "Eye ProjectionCenterOffset: " << mCamera.getProjectionCenterOffset() << "\n";
    }
}

void oculusBaseApp::mouseDown( MouseEvent event )
{
}

void oculusBaseApp::update()
{
    //get oculus orientation and update the cameras
    if( mOculusVR )
    {
        quat orientation;
        orientation = mOculusVR->getOrientation();
        mCamera.setOrientation( orientation * quat( M_PI, vec3( 0, 1, 0 ) ) );
    }
}

//Render is the new draw function
void oculusBaseApp::render()
{
    static int i=0;
    
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    gl::setMatrices( mCamera );
    {
        gl::color(1,1,1);
        gl::ScopedMatrices matModel;
        gl::translate(vec3(0,0,-30));
        gl::rotate(toRadians((float)i++), vec3(0,1,0));
        gl::drawColorCube(vec3(0,0,0), vec3(10));
    }
    gl::disableDepthRead();
    gl::disableDepthWrite();
    
}

//in draw we will combine our eyes into one stereo image
void oculusBaseApp::draw()
{
	gl::clear( Color( 0.5f, 0.0f, 0.0f ) );

    mFbo->bindFramebuffer();
    
    gl::clear( Color( 1.f, 0.5f, 0.f ) );

    // Render Left Eye
    mCamera.enableStereoLeft();
    {
        vec2 loc = vec2( 0.0f, 0.0f );
        vec2 dim = vec2( mFbo->getWidth() / 2.0f, mFbo->getHeight() );
        gl::ScopedViewport viewport( loc, dim );
        gl::ScopedScissor scissor( loc, dim );
        gl::clear( Color( 0.5f, 0.5f, 0.f ) );
        render();
    }
    
    // Render Right Eye
    mCamera.enableStereoRight();
    {
        vec2 loc = vec2( mFbo->getWidth() / 2.0f, 0.0f );
        vec2 dim = vec2( mFbo->getWidth() / 2.0f, mFbo->getHeight() );
        gl::ScopedViewport viewport( loc, dim);
        gl::ScopedScissor scissor( loc, dim );

        gl::clear( Color( 0.f, 0.5f, 0.5f ) );
        render();
    }

    mFbo->unbindFramebuffer();
    
    // Back to 2d rendering
    gl::setMatricesWindow( getWindowSize() );//, false );
    gl::disableDepthRead();
    gl::disableDepthWrite();

    mDistortionHelper->render( mFbo->getColorTexture(), getWindowBounds());

    // Draw FPS
    gl::enableAlphaBlending();
    {
        gl::ScopedMatrices scaleFont;
        gl::scale(vec3(0.25f));
        gl::color(1,1,1);
        mFont->drawString(to_string((int)getAverageFps()) + " FPS", vec2(100,200));
    }
 
}

CINDER_APP_NATIVE( oculusBaseApp, RendererGl )
