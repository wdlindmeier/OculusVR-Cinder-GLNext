//
//  CameraStereoHMD.h
//  OculusSDKTest
//
//  Created by Simon Geilfus on 26/05/13.
//
//

#pragma once

#include "cinder/Camera.h"


class CameraStereoHMD : public ci::CameraStereo {
public:
	CameraStereoHMD();
	CameraStereoHMD( int pixelWidth, int pixelHeight, float fov );
	CameraStereoHMD( int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane );
    
    //! Returns value used to offset the projections matrices
    float   getProjectionCenterOffset() const { return mProjectionCenterOffset; }
    //! Set the value used to offset the projections matrices
    void    setProjectionCenterOffset( float offset ) { mProjectionCenterOffset = offset; }
	
    //! Returns Left Eye Projection Matrix
	virtual const ci::mat4&	getProjectionMatrixLeft() const;
    //! Returns Left Eye ModelView Matrix
	virtual const ci::mat4&	getModelViewMatrixLeft() const;
    //! Returns Left Eye Inverse-ModelView Matrix
	virtual const ci::mat4&	getInverseModelViewMatrixLeft() const;
    
    //! Returns Right Eye Projection Matrix
	virtual const ci::mat4&	getProjectionMatrixRight() const;
    //! Returns Right Eye ModelView Matrix
	virtual const ci::mat4&	getModelViewMatrixRight() const;
    //! Returns Right Eye Inverse-ModelView Matrix
	virtual const ci::mat4&	getInverseModelViewMatrixRight() const;
    
protected:
    
	virtual void	calcViewMatrix() const;
	virtual void	calcProjection() const;
    
private:
    
    float           mProjectionCenterOffset;
};