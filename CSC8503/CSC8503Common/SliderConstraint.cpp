#include "SliderConstraint.h"
#include "GameObject.h"
#include <algorithm>

using namespace NCL;
using namespace CSC8503;
using namespace Maths;

void SliderConstraint::UpdateConstraint(float dt)
{
	Vector3 relativePos = objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition();
	Vector3 offsetPos;

	if (Vector3::Dot((-relativePos).Normalised(), direction) == 1.0f){
		if (relativePos.Length() >= minDist && relativePos.Length() <= maxDist){
			return;
		}
	}

	float distOnLine = Vector3::Dot(-relativePos, direction);

	distOnLine = std::max(minDist, distOnLine);
	distOnLine = std::min(maxDist, distOnLine);
	offsetPos = relativePos + direction * distOnLine;

	float offset = offsetPos.Length();

	if (abs(offset) > 0.0f){
		Vector3 offsetDir = offsetPos.Normalised();

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();
		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

		if (constraintMass > 0.0f){
			float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);
			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * -offset;
			float lambda = -(velocityDot + bias) / constraintMass;
			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;
			physA->ApplyLinearImpulse(aImpulse);
			physB->ApplyLinearImpulse(bImpulse);
		}
	}
}