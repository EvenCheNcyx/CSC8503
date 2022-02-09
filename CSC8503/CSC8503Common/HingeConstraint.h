#pragma once
#include "Constraint.h"
#include "PhysicsObject.h"
#include "../../Common/Vector3.h"

namespace NCL {
	using namespace Maths;
	namespace CSC8503 {
		class GameObject;

		class HingeConstraint : public Constraint {
		public:
			HingeConstraint(GameObject* a, GameObject* b, int axis){
				objectA = a;
				objectB = b;
				axisIndex = axis;
			};
			~HingeConstraint() {};

			void UpdateConstraint(float dt) override;
			void ApplyAngularImpulse(PhysicsObject* physA, PhysicsObject* physB, const Vector3& w1, const Vector3& u2, const Vector3& v2, float dt);

		protected:
			GameObject* objectA;
			GameObject* objectB;
			int axisIndex;
		};
	}
}