#include "Camera.h"

void Camera::SetLens(float zNear, float zFar, float aspct, float fovy)
{
	nearZ = zNear;
	farZ = zFar;
	aspect = aspct;
	fovY = fovy;
	nearHeight = tan(fovY/2) * nearZ * 2;
	farHeight = tan(fovY/2) * farZ * 2;
}

void Camera::LookAt(const Math::Vector3& pos, const Math::Vector3& target, const Math::Vector3& _up)
{
	position = pos;
	forward = normalize(target - position);
	right = normalize(cross(_up, forward));		//左手系所以上x前
	up = cross(forward, right);					//点积快于求单位长度

	//std::cout << up.x << " " << up.y << " " << up.z << std::endl;
	//std::cout << forward.x << " " << forward.y << " " << forward.z << std::endl;
	//std::cout << (cross(up, forward)).x << " " << (cross(up, forward)).y << " " << (cross(up, forward)).z << std::endl;
}

void Camera::UpdateViewMatrix()
{
	//view = GetInverseTransformMatrix(right, up, forward, position);

	Math::Matrix4 mat1, mat2;
	mat1.SetX({ 1,0,0,0});
	mat1.SetY({ 0,1,0,0});
	mat1.SetZ({ 0,0,1,0});
	mat1.SetW({ -position.x,-position.y,-position.z,1 });
	
	mat2.SetX({(cross(up,forward)).x, up.x, forward.x, 0});
	mat2.SetY({(cross(up,forward)).y, up.y, forward.y, 0});
	mat2.SetZ({(cross(up,forward)).z, up.z, forward.z, 0});
	mat2.SetW({0,0,0,1});
	view = mat2 * mat1;
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

void Camera::GenerateBoundingFrustum()
{
	//注意，该函数构建出的是观察空间的视锥体，我们需要的是世界空间中的视锥体
	//BoundingFrustum::CreateFromMatrix(boundingFrustum, proj);

	float nearWidth = nearHeight * aspect;
	Math::Vector3 nearLeftBottom = { -nearWidth / 2, -nearHeight / 2, nearZ };
	Math::Vector3 nearLeftTop= { -nearWidth / 2, nearHeight / 2, nearZ };
	Math::Vector3 nearRightBottom = { nearWidth / 2, -nearHeight / 2, nearZ };
	Math::Vector3 nearRightTop = { nearWidth / 2, nearHeight / 2, nearZ };
	
	//满足左手定则
	Math::Vector3 normalLeft = XMVector3Cross(nearLeftBottom, nearLeftTop-nearLeftBottom);
	Math::Vector3 normalRight = XMVector3Cross(nearRightTop, nearRightBottom-nearRightTop);
	Math::Vector3 normalUp = XMVector3Cross(nearLeftTop, nearRightTop-nearLeftTop);
	Math::Vector3 normalDown = XMVector3Cross(nearRightBottom, nearLeftBottom-nearRightBottom);

	boundingFrustum[0] = { normalUp,0 };
	boundingFrustum[1] = { normalDown,0 };
	boundingFrustum[2] = { normalLeft,0 };
	boundingFrustum[3] = { normalRight,0 };
	boundingFrustum[4] = {0,0,-1,nearZ};
	boundingFrustum[5] = {0,0,1,-farZ};
}

//把视锥体转换到世界空间
void Camera::GenerateWorldBoundingFrustum(PerCameraConstant& perCameraConstant, Camera* mainCamera)
{
	Math::Matrix4 viewInverse = XMMatrixInverse(nullptr, perCameraConstant.viewMatrix);
	for (UINT i = 0;i < 6;i++)
	{
		//拿到相机的点法向。向量的空间变换第四位是零，点已经是世界空间坐标了
		Math::Vector4 norm = { mainCamera->boundingFrustum[i].x, mainCamera->boundingFrustum[i].y, mainCamera->boundingFrustum[i].z ,0 };
		Math::Vector3 pos = mainCamera->position;

		//变为世界法向
		norm = viewInverse * norm;
		//法向归一化
		norm = Math::Vector3(DirectX::XMVector3Normalize(norm));
		perCameraConstant.frustum[i].x = norm.GetX();
		perCameraConstant.frustum[i].y = norm.GetY();
		perCameraConstant.frustum[i].z = norm.GetZ();
		Math::Vector3 nearPoint = pos + mainCamera->forward * mainCamera->NearZ();
		Math::Vector3 farPoint = pos + mainCamera->forward * mainCamera->FarZ();
		if (i < 4)
			perCameraConstant.frustum[i].w = -(norm.GetX() * pos.GetX() + norm.GetY() * pos.GetY() + norm.GetZ() * pos.GetZ());  //根据点和法向算出D
		else if (i == 4)
			perCameraConstant.frustum[i].w = -(norm.GetX() * nearPoint.GetX() + norm.GetY() * nearPoint.GetY() + norm.GetZ() * nearPoint.GetZ());
		else
			perCameraConstant.frustum[i].w = -(norm.GetX() * farPoint.GetX() + norm.GetY() * farPoint.GetY() + norm.GetZ() * farPoint.GetZ());
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

void Camera::Walk()
{
	if(isWalking)
		position += walkStep * forward;
}

void Camera::Strafe()
{
	if(isStrafing)
		position += strafeStep * right;
}

void Camera::UpDown()
{
	if (isUpDown)
		position += upDownStep * up;
}

void Camera::Move()
{
	Walk();
	Strafe();
	UpDown();
}

void Camera::Pitch(float angle)
{
	XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3((XMFLOAT3*)&right), angle);
	XMStoreFloat3((XMFLOAT3*)&up, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&up), rotate)));
	XMStoreFloat3((XMFLOAT3*)&forward, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&forward), rotate)));
}

void Camera::RotateY(float angle)
{
	//XMMATRIX rotate = XMMatrixRotationY(angle);
	XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3((XMFLOAT3*)&up), angle);
	XMStoreFloat3((XMFLOAT3*)&up, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&up), rotate)));
	XMStoreFloat3((XMFLOAT3*)&right, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&right), rotate)));
	XMStoreFloat3((XMFLOAT3*)&forward, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3((XMFLOAT3*)&forward), rotate)));
}
