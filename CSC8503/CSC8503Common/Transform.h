#pragma once
#include "../../Common/Matrix4.h"
#include "../../Common/Matrix3.h"
#include "../../Common/Vector3.h"
#include "../../Common/Quaternion.h"

#include <vector>

using std::vector;

using namespace NCL::Maths;

namespace NCL {
	namespace CSC8503 {
		class Transform
		{
		public:
			Transform();
			~Transform();

			Transform& SetPosition(const Vector3& worldPos);
			Transform& SetScale(const Vector3& worldScale);
			Transform& SetOrientation(const Quaternion& newOr);

			Vector3 GetPosition() const {
				return position;
			}

			Vector3 GetWorldPosition() const {
				return worldMatrix.GetPositionVector();
			}

			Vector3 GetScale() const {
				return scale;
			}

			Quaternion GetOrientation() const {
				return orientation;
			}

			Vector3 GetAbove() const
			{
				return Vector3(2 * (orientation.x * orientation.y - orientation.w * orientation.z), 1 - 2 * (orientation.x * orientation.x + orientation.z * orientation.z), 2 * (orientation.y * orientation.z + orientation.w * orientation.x));
			}

			Matrix4 GetMatrix() const {
				return matrix;
			}

			Matrix4 GetWorldMatrix() const {
				return worldMatrix;
			}


			void UpdateMatrix();
			void SetWorldPosition(const Vector3& worldPos);
			void SetLocalPosition(const Vector3& localPos);

		protected:
			Matrix4		matrix;
			Matrix4		worldMatrix;

			Quaternion	orientation;

			Vector3		position;
			Vector3		scale;

			Transform* parent;
		};
	}
}

