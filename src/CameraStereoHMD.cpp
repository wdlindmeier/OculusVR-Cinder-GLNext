//
//  CameraStereoHMD.cpp
//  OculusSDKTest
//
//  Created by Simon Geilfus on 26/05/13.
//
//

#include "CameraStereoHMD.h"

using namespace ci;

CameraStereoHMD::CameraStereoHMD()
: ci::CameraStereo()
, mProjectionCenterOffset( 0.151976f )
{
    setEyeSeparation( 0.64f );
    //setEyeSeparation( 0.00119808f );
    setConvergence(0);
    setFov( 125.871f );
}

CameraStereoHMD::CameraStereoHMD( int pixelWidth, int pixelHeight, float fov )
: CameraStereo( pixelWidth, pixelHeight, fov )
, mProjectionCenterOffset( 0.151976f )
{
    setEyeSeparation( 0.64f );
    //setEyeSeparation( 0.00119808f );
    setConvergence(0);
}

CameraStereoHMD::CameraStereoHMD( int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane )
: CameraStereo( pixelWidth, pixelHeight, fov, nearPlane, farPlane )
, mProjectionCenterOffset( 0.151976f )
{
    setEyeSeparation( 0.64f );
    //setEyeSeparation( 0.00119808f );
    setConvergence(0);
}

const mat4& CameraStereoHMD::getProjectionMatrixLeft() const
{
	if( ! mProjectionCached )
		calcProjection();
    
    return mProjectionMatrixLeft;
}

const mat4& CameraStereoHMD::getModelViewMatrixLeft() const
{
	if( ! mModelViewCached )
		calcViewMatrix();
    
    return mViewMatrixLeft;
}

const mat4& CameraStereoHMD::getInverseModelViewMatrixLeft() const
{
	if( ! mInverseModelViewCached )
		calcInverseView();
    
    return mInverseModelViewMatrixLeft;
}

const mat4& CameraStereoHMD::getProjectionMatrixRight() const
{
	if( ! mProjectionCached )
		calcProjection();
    
    return mProjectionMatrixRight;
}

const mat4& CameraStereoHMD::getModelViewMatrixRight() const
{
	if( ! mModelViewCached )
		calcViewMatrix();
    
    return mViewMatrixRight;
}

const mat4& CameraStereoHMD::getInverseModelViewMatrixRight() const
{
	if( ! mInverseModelViewCached )
		calcInverseView();
    
    return mInverseModelViewMatrixRight;
}

void CameraStereoHMD::calcViewMatrix() const
{
	// calculate default matrix first
	CameraPersp::calcViewMatrix();
    
    mViewMatrixLeft = glm::translate( mat4(1), vec3( getEyeSeparation(), 0, 0 ) ) * mViewMatrix;
    mViewMatrixRight = glm::translate( mat4(1), vec3( -getEyeSeparation(), 0, 0 ) ) * mViewMatrix;
}

void CameraStereoHMD::calcProjection() const
{
	// calculate default matrices first
	CameraPersp::calcProjection();
	
    mProjectionMatrixLeft = glm::translate( mat4(1), vec3( mProjectionCenterOffset, 0, 0 ) ) * mProjectionMatrix;
    mInverseProjectionMatrixLeft = glm::affineInverse( mProjectionMatrixLeft );

    mProjectionMatrixRight = glm::translate( mat4(1), vec3( -mProjectionCenterOffset, 0, 0 ) ) * mProjectionMatrix;
    mInverseProjectionMatrixRight = glm::affineInverse( mProjectionMatrixRight );
}