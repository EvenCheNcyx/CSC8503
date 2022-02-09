#include "HingeConstraint.h"
#include "GameObject.h"
#include "../../Common/Maths.h"

using namespace NCL;
using namespace CSC8503;
using namespace Maths;

void HingeConstraint::UpdateConstraint(float dt){
	if (axisIndex < 0 || axisIndex>2) {
		return;
	}

	Quaternion orientationA = objectA->GetTransform().GetOrientation();
	Quaternion orientationB = objectB->GetTransform().GetOrientation();

	Matrix3 rotMatA = Matrix3(orientationA);
	Matrix3 rotMatB = Matrix3(orientationB);
	Vector3 w1 = rotMatA.GetColumn(axisIndex);
	std::vector<Vector3> uv;
	for (int i = 0; i < 3; i++){
		if (i == axisIndex) {
			continue;
		}
		uv.push_back(rotMatB.GetColumn(i));
	}

	PhysicsObject* physA = objectA->GetPhysicsObject();
	PhysicsObject* physB = objectB->GetPhysicsObject();
	ApplyAngularImpulse(physA, physB, w1, uv[0], uv[1], dt);
}

void HingeConstraint::ApplyAngularImpulse(PhysicsObject* physA, PhysicsObject* physB, const Vector3& w1, const Vector3& u2, const Vector3& v2, float dt){
	float C1 = Maths::Clamp(Vector3::Dot(u2, w1), -1.0f, 1.0f);
	float C2 = Maths::Clamp(Vector3::Dot(v2, w1), -1.0f, 1.0f);

	if (abs(C1) < 0.0001f && abs(C2) < 0.0001f) {
		return;
	}

	Vector3 J1 = Vector3::Cross(u2, w1);
	Vector3 J2 = Vector3::Cross(v2, w1);

	Vector3 rAngularVelocity = physB->GetAngularVelocity() - physA->GetAngularVelocity();
	float dC1 = Vector3::Dot(J1, rAngularVelocity);
	float dC2 = Vector3::Dot(J2, rAngularVelocity);

	physA->UpdateInertiaTensor();
	physB->UpdateInertiaTensor();

	Vector3 invInertia = physA->GetInertia() + physB->GetInertia();
	
	float biasFactor = 0.001f;

	float K1 = Vector3::Dot(J1, invInertia * J1);
	float K2 = Vector3::Dot(J2, invInertia * J2);

	float lambda1 = -(dC1 + biasFactor * C1 / dt) / K1;
	float lambda2 = -(dC2 + biasFactor * C2 / dt) / K2;

	Vector3 impulse = J1 * lambda1 + J2 * lambda2;

	physA->ApplyAngularImpulse(-impulse);
	physB->ApplyAngularImpulse(impulse);
}

