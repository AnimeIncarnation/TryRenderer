#include "Camera.h"

void Camera::SetLens(float zNear, float zFar, float aspct, float fovy)
{
	nearZ = zNear;
	farZ = zFar;
	aspect = aspct;
	fovY = fovy;
	nearHeight = tan(fovY/2) * nearZ;
	farHeight = tan(fovY/2) * farZ;
}

void Camera::LookAt(const Math::Vector3& pos, const Math::Vector3& target, const Math::Vector3& _up)
{
	position = pos;
	forward = normalize(target - position);
	right = normalize(cross(_up, forward));		//左手系所以上x前
	up = cross(forward, right);					//点积快于求单位长度
}

void Camera::UpdateViewMatrix()
{
	view = GetInverseTransformMatrix(right, up, forward, position);
}


void Camera::UpdateProjectionMatrix()
{
	if (isOrtho)
	{
		farZ = Max(farZ, nearZ + 0.1f);
		Math::Matrix4 P = Math::XMMatrixOrthographicLH(orthoSize, orthoSize, nearZ, farZ);
		proj = P;
	}
	else
	{
		nearZ = Max(nearZ, 0.01f);
		farZ = Max(farZ, nearZ + 0.1f);
		Math::Matrix4 P = Math::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
		proj = P;
	}
}

Math::Matrix4 const& Camera::GetViewMatrix() const
{
	return view;
}				
Math::Matrix4 const& Camera::GetProjectionMatrix() const
{
	return proj;
}

