#pragma once

namespace NCL {
	namespace CSC8503 {
		class PushdownState {
		public:
			enum PushdownResult {
				Push,Pop,NoChange
			};
			PushdownState() {}
			virtual ~PushdownState() {}

			virtual PushdownResult OnUpdate(float dt, PushdownState** pushFunc) ;
			virtual void OnAwake() {}
			virtual void OnSleep() {}
		};
	}
}

