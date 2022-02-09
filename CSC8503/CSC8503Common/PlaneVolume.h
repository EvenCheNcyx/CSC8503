#pragma once
#include "CollisionVolume.h"
#include "../../Common/Vector2.h"
namespace NCL {
	using namespace Maths;
	class PlaneVolume : CollisionVolume
	{
	public:
		PlaneVolume(const Maths::Vector2& size) {
			type = VolumeType::Plane;
			Vector2 planeSizes = size;
		}
		~PlaneVolume() {}

		Maths::Vector3 GetPlaneSize() const {
			return planeSize;
		}
	protected:
		Vector2 planeSize;
	};
}