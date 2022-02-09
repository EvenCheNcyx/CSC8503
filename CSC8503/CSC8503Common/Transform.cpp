#include "Transform.h"

using namespace NCL::CSC8503;

Transform::Transform()
{
	scale	= Vector3(1, 1, 1);
}

Transform::~Transform()
{

}

void Transform::UpdateMatrix() {
	matrix =
		Matrix4::Translation(position) *
		Matrix4(orientation) *
		Matrix4::Scale(scale);
}

Transform& Transform::SetPosition(const Vector3& worldPos) {
	position = worldPos;
	UpdateMatrix();
	return *this;
}

Transform& Transform::SetScale(const Vector3& worldScale) {
	scale = worldScale;
	UpdateMatrix();
	return *this;
}

Transform& Transform::SetOrientation(const Quaternion& worldOrientation) {
	orientation = worldOrientation;
	UpdateMatrix();
	return *this;
}

//========================//
void Transform::SetWorldPosition(const Vector3& worldPos) {
	if (parent) {
		Vector3 parentPos = parent->GetWorldMatrix().GetPositionVector();
		Vector3 posDiff = parentPos - worldPos;

		position = posDiff;
		matrix.SetPositionVector(posDiff);
	}
	else {
		position = worldPos;
		worldMatrix.SetPositionVector(worldPos);
	}
}