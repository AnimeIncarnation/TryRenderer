#pragma once
#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "../stdafx.h"
#include "../DXMath/DXMath.h"
#include "../DXMath/MathHelper.h"
#include <iostream>
#include "DirectXCollision.h"
#include "../Component/Constants.h"

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
	void GenerateBoundingFrustum();
	void GenerateWorldBoundingFrustum(PerCameraConstant&, Camera*);
	Math::Matrix4 const& GetViewMatrix() const;
	Math::Matrix4 const& GetProjectionMatrix() const;

	void Walk();
	void Strafe();
	void UpDown();
	void Move();
	void Pitch(float d);
	void RotateY(float d);
	
	float aspect = 1.f;		//宽高比
private:
	//frustrum
	float nearZ = 1.f;
	float farZ = 1000.f;
	float fovY = MathHelper::Pi * 1/4;
	float nearHeight = tan(fovY / 2) * nearZ * 2;	//近平面的高度
	float farHeight = tan(fovY / 2) * farZ * 2;
	bool isOrtho = false;
	float orthoSize = 5;



	//camera
	Math::Matrix4 view = MathHelper::Identity4x4();
	Math::Matrix4 proj = MathHelper::Identity4x4();
public:
	Math::Vector3 position = {0, 0, 0};
	Math::Vector3 right = {1, 0, 0};
	Math::Vector3 up = {0, 1, 0};
	Math::Vector3 forward = {0, 0, 1};
	Math::Vector4 boundingFrustum[6];

	//camera移动控制
	bool isWalking = false;
	float walkStep = 0;
	bool isStrafing = false;
	float strafeStep = 0;
	bool isUpDown = false;
	float upDownStep = 0;
	void OutputPosition()
	{
		std::cout << "Position: " << position.x << "," << position.y << "," << position.z << std::endl;
	}
	void OutputLookDir()
	{
		std::cout << "LookDir: " << forward.x << "," << forward.y << "," << forward.z << "," << std::endl;
	}
};

#endif