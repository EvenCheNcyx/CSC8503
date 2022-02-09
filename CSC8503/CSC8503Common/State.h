#pragma once
#include <functional>

namespace NCL {
	namespace CSC8503 {
		typedef std::function<void(float)> StateUpdateFunction;
		typedef void(*StateFunction)(void*);
		class State		{
		public:
			State() {}
			State(StateUpdateFunction someFunc) {
				func = someFunc;
			}

			virtual void Update(float dt1) {
				if (func != nullptr) {
					func(dt1);
				}
			}

			virtual ~State() {}

		protected:
			StateUpdateFunction func;
			float* funcData;
		};
	}
}