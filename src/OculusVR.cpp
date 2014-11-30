//
//  OculusVR.cpp
//  OculusSDKTest
//
//  Created by Simon Geilfus on 29/05/13.
//
//

#include "OculusVR.h"

#include "cinder/Utilities.h"

using namespace ci;

namespace ovr {
    
    
    DeviceRef Device::create()
    {
        // Try to initialize a device
        DeviceRef newDevice( new Device() );
        if( newDevice->mHMD )
            return newDevice;
        
        // Returns a null_ptr if it failed
        else return DeviceRef();
    }
    Device::~Device()
    {
        // If thread is running wait for it to end
        mIsAutoCalibrating = false;
        if( mAutoCalibrationThread.joinable() )
            mAutoCalibrationThread.join();
        
        // Clear Hmd and Sensor
        mSensorDevice.Clear();
        mHMD.Clear();
        mManager.Clear();
        
        OVR::System::Destroy();
    }
    
    Device::Device( bool autoCalibrate )
    {
        
        // Init OVR
        OVR::System::Init( OVR::Log::ConfigureDefaultLog( OVR::LogMask_All ) );
        
        // Create Manager and Device Handle
        mManager = *OVR::DeviceManager::Create();
        mHMD     = *mManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
        
        // If everything's fine attach the device, setup the stereo and start callibration
        if( mHMD ){
            
            if (mHMD->GetDeviceInfo(&mHMDInfo))
            {
                mStereoConfig.SetHMDInfo( mHMDInfo );
            }
            
            mSensorDevice = *mHMD->GetSensor();
            
            if (mSensorDevice)
                mSensorFusion.AttachToSensor(mSensorDevice);
            
            if( autoCalibrate ){
                mIsAutoCalibrating = true;
                mMagCalibration.BeginAutoCalibration( mSensorFusion );
                mAutoCalibrationThread = std::thread( &Device::updateAutoCalibration, this );
            }
            else mIsAutoCalibrating = false;
        }
    }
    
    
    float Device::getIPD() const
    {
        return mStereoConfig.GetIPD();
    }
    float Device::getFov() 
    {
        return mStereoConfig.GetYFOVDegrees();
    }
    float Device::getEyeToScreenDistance() const
    {
        return mStereoConfig.GetEyeToScreenDistance();
    }
    float Device::getProjectionCenterOffset()
    {
        return mStereoConfig.GetProjectionCenterOffset();
    }
    float Device::getDistortionScale()
    {
        return mStereoConfig.GetDistortionScale();
    }
    vec4 Device::getDistortionParams() const
    {
        return vec4( mHMDInfo.DistortionK[0], mHMDInfo.DistortionK[1], mHMDInfo.DistortionK[2], mHMDInfo.DistortionK[3] );
    }
    
    quat Device::getOrientation()
    {
        return toCinder( mSensorFusion.GetOrientation() );
    }
    
    
    
    Area        Device::getLeftEyeViewport()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left ).VP );
    }
    mat4   Device::getLeftEyeViewAdjust()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left ).ViewAdjust );
    }
    mat4   Device::getLeftEyeProjection()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left ).Projection );
    }
    mat4   Device::getLeftEyeOrthoProjection()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left ).OrthoProjection );
    }
    
    Area        Device::getRightEyeViewport()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Right ).VP );
    }
    mat4   Device::getRightEyeViewAdjust()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Right ).ViewAdjust );
    }
    mat4   Device::getRightEyeProjection()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Right ).Projection );
    }
    mat4   Device::getRightEyeOrthoProjection()
    {
        return toCinder( mStereoConfig.GetEyeRenderParams( OVR::Util::Render::StereoEye_Right ).OrthoProjection );
    }
    
    
    void Device::updateAutoCalibration()
    {
        while ( mIsAutoCalibrating )
        {
            mMagCalibration.UpdateAutoCalibration( mSensorFusion );
            if ( mMagCalibration.IsCalibrated() )
            {
                // if ( mSensorFusion.IsMagReady() )
                //     mSensorFusion.SetYawCorrectionEnabled(true);
                OVR::Vector3f mc = mMagCalibration.GetMagCenter();
                std::cout << "   Magnetometer Calibration Complete" << std::endl << "Center: " << mc.x << " " << mc.y << " " << mc.z << std::endl;
                
                mIsAutoCalibrating = false;
            }
            else if( !mMagCalibration.IsAutoCalibrating() ){
                mIsAutoCalibrating = false;
            }
            
            ci::sleep( 1 );
        }
    }
    
    
    
    static const char* PostProcessFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform sampler2D Texture0;\n"
    "\n"
    "vec2 HmdWarp(vec2 in01)\n"
    "{\n"
    "   vec2  theta = (in01 - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq = theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + "
    "                           HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
    "   return LensCenter + Scale * theta1;\n"
    "}\n"
    "void main()\n"
    "{\n"
    "   vec2 tc = HmdWarp(gl_TexCoord[0].st);\n"
    "   if (!all(equal(clamp(tc, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tc)))\n"
    "       gl_FragColor = vec4(0,0,0,1);\n"
    "   else\n"
    "       gl_FragColor = texture2D(Texture0, tc);\n"
    "}\n";
    
    
    // Shader with lens distortion and chromatic aberration correction.
    static const char* PostProcessFullFragShaderSrc =
    "uniform vec2 LensCenter;\n"
    "uniform vec2 ScreenCenter;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 ScaleIn;\n"
    "uniform vec4 HmdWarpParam;\n"
    "uniform vec4 ChromAbParam;\n"
    "uniform sampler2D Texture0;\n"
    "\n"
    // Scales input texture coordinates for distortion.
    // ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
    // larger due to aspect ratio.
    "void main()\n"
    "{\n"
    "   vec2  theta = (gl_TexCoord[0].st - LensCenter) * ScaleIn;\n" // Scales to [-1, 1]
    "   float rSq= theta.x * theta.x + theta.y * theta.y;\n"
    "   vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + "
    "                  HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);\n"
    "   \n"
    "   // Detect whether blue texture coordinates are out of range since these will scaled out the furthest.\n"
    "   vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);\n"
    "   vec2 tcBlue = LensCenter + Scale * thetaBlue;\n"
    "   if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tcBlue)))\n"
    "   {\n"
    "       gl_FragColor = vec4(0);\n"
    "       return;\n"
    "   }\n"
    "   \n"
    "   // Now do blue texture lookup.\n"
    "   float blue = texture2D(Texture0, tcBlue).b;\n"
    "   \n"
    "   // Do green lookup (no scaling).\n"
    "   vec2  tcGreen = LensCenter + Scale * theta1;\n"
    "   vec4  center = texture2D(Texture0, tcGreen);\n"
    "   \n"
    "   // Do red scale and lookup.\n"
    "   vec2  thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);\n"
    "   vec2  tcRed = LensCenter + Scale * thetaRed;\n"
    "   float red = texture2D(Texture0, tcRed).r;\n"
    "   \n"
    "   gl_FragColor = vec4(red, center.g, blue, 1);\n"
    "}\n";
    
    
    DistortionHelperRef DistortionHelper::create( bool chromaticAbCorrection )
    {
        return DistortionHelperRef( new DistortionHelper( chromaticAbCorrection ) );
    }
    DistortionHelper::DistortionHelper( bool chromaticAbCorrection )
    :
    mDistortionParams( 1,0.22,0.24,0 ),
    mDistortionScale( 1.71461f ),
    mChromaticAbCorrection( 0.996, -0.004, 1.014, 0 ),
    mUseChromaticAbCorrection( chromaticAbCorrection )
    {
        
        // Load and compile Distortion Shader
        try {
            mShader = gl::GlslProg::create( NULL, chromaticAbCorrection ? PostProcessFullFragShaderSrc : PostProcessFragShaderSrc );
        }
        catch( gl::GlslProgCompileExc exc ){
            std::cout << "ovr::DistortionHelper Exception: " << std::endl << exc.what() << std::endl;
        }
    }
    
    void DistortionHelper::render( const gl::TextureRef &texture, const Rectf &rect )
    {
        render( *texture, rect );
    }
    void DistortionHelper::render( const gl::Texture &texture, const Rectf &rect )
    {
        float w                         = 0.5f;
        float h                         = 1.0f;
        float x                         = 0.0f;
        float y                         = 0.0f;
        float as                        = ( (float) rect.getWidth() * 0.5f ) / (float) rect.getHeight();
        float distortionXCenterOffset   = 0.25f / mDistortionScale;
        float scaleFactor               = 1.0f / mDistortionScale;
        
        mShader->bind();
        mShader->uniform( "LensCenter", vec2( x + (w + distortionXCenterOffset * 0.5f)*0.5f, y + h*0.5f ) );
        mShader->uniform( "ScreenCenter", vec2( x + w*0.5f, y + h*0.5f ) );
        
        mShader->uniform( "Scale", vec2( (w/2) * scaleFactor, (h/2) * scaleFactor * as ) );
        mShader->uniform( "ScaleIn", vec2( (2/w),               (2/h) / as ) );
        mShader->uniform( "HmdWarpParam", mDistortionParams );

        if( mUseChromaticAbCorrection )
            mShader->uniform( "ChromAbParam", mChromaticAbCorrection );
        
        //texture.enableAndBind();
        texture.bind();
        mShader->uniform( "Texture0", 0 );
        
        glEnable( GL_SCISSOR_TEST );
        glScissor( 0, 0, rect.getWidth() * 0.5f, rect.getHeight() );
        gl::drawSolidRect( rect );
        
        
        distortionXCenterOffset = -0.25f / mDistortionScale;
        scaleFactor             = 1.0f / mDistortionScale;
        x                       = 0.5f;
        
        mShader->uniform( "LensCenter", vec2( x + (w + distortionXCenterOffset * 0.5f)*0.5f, y + h*0.5f ) );
        mShader->uniform( "ScreenCenter", vec2( x + w*0.5f, y + h*0.5f ) );
        mShader->uniform( "Scale", vec2( (w/2) * scaleFactor, (h/2) * scaleFactor * as ) );
        
        glScissor( rect.getWidth() * 0.5f, 0, rect.getWidth(), rect.getHeight() );
        gl::drawSolidRect( rect );
        glDisable( GL_SCISSOR_TEST );
        
        texture.unbind();
        //mShader->unbind();
        
    }
    
}