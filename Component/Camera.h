#pragma once
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "../stdafx.h"
#include "../DXMath/DXMath.h"
#include "../DXMath/MathHelper.h"

class Camera
{
public:
	Camera() = default;
	Camera(bool isOrtho, int orthoSize = 5):isOrtho(isOrtho),orthoSize(orthoSize){}

	~Camera() = default;
	//frustrum getter
	float NearZ() const { return nearZ; }
	float FarZ() const { return farZ; }
	float Aspect() const { return aspect; }
	float FovY() const { return fovY; }
	float FovX() const {
		float halfWidth = 0.5 * NearWidth();
		return 2.0 * atan(halfWidth / nearZ);
	}
	float NearHeight()const { return nearHeight; }
	float NearWidth()const { return nearHeight * aspect; }
	float FarHeight()const { return farHeight; }
	float FarWidth()const { return farHeight * aspect; }

	//frustrum setter
	void SetLens(float zNear, float zFar, float aspect, float fovY);
	void IsOrtho() { isOrtho = true; };
	
	//camera setter
	void LookAt(const Math::Vector3& pos, const Math::Vector3& target, const Math::Vector3& up);

	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	Math::Matrix4 const& GetViewMatrix() const;
	Math::Matrix4 const& GetProjectionMatrix() const;
	
	float aspect = 1.f;		//宽高比
private:
	//frustrum
	float nearZ = 1.f;
	float farZ = 1000.f;
	float fovY = MathHelper::Pi * 1/4;
	float nearHeight = tan(fovY / 2) * nearZ;	//近平面的高度
	float farHeight = tan(fovY / 2) * farZ;
	bool isOrtho = false;
	float orthoSize = 5;

	//camera
	Math::Vector3 position = {0, 0, 0};
	Math::Vector3 right = {1, 0, 0};
	Math::Vector3 up = {0, 1, 0};
	Math::Vector3 forward = {0, 0, 1};
	Math::Matrix4 view = MathHelper::Identity4x4();
	Math::Matrix4 proj = MathHelper::Identity4x4();
};

#endif