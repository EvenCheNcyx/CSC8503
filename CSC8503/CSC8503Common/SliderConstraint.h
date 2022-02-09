#pragma once
#include "../../Common/Vector3.h"
#include "Constraint.h"

namespace NCL {
	using namespace Maths;
	namespace CSC8503 {
		class GameObject;

		class SliderConstraint : public Constraint {
		public:
			SliderConstraint(GameObject* a, GameObject* b, const Vector3& dir, float min, float max) {
				objectA = a;
				objectB = b;
				direction = dir.Normalised();
				minDist = min;
				maxDist = max;
			};

			~SliderConstraint() {}

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* objectA;
			GameObject* objectB;
			Vector3 direction;
			float minDist;
			float maxDist;
		};
	}
}