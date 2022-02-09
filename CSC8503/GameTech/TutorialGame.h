#pragma once
#include "GameTechRenderer.h"
#include "StateGameObject.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/PushdownMachine.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();
	

			virtual void UpdateGame(float dt);

			bool UpdatePushdown(float dt) {
				if (!machine->Update(dt)) {
					return false;
				}
				return true;
			}

		protected:
			void InitialiseAssets();
			

			void InitCamera();
		

			void UpdateKeys();

			void InitWorld();
			

			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();
			void SliderContraintTest();
			void HingeContraintTest();
	
			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position, const std::string& name = "");

			//=========================
			GameObject* AddWallToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddxWallToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddzWallToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddSlopeToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddTargetFloorToWorld(const Vector3& position, const std::string& name = "");
	
			GameObject* AddBounce1(const Vector3& position);
			GameObject* AddBounce2(const Vector3& position);
			GameObject* AddBounce3(const Vector3& position);
			GameObject* AddBounce4(const Vector3& position);

			StateGameObject* AddDoorToWorld(const Vector3& position);


			PushdownMachine* machine;
			std::string winnerName;
			bool multiplayer = 0;

			void DrawMenu();
			void DrawWin();
			void DrawLose(std::string winnner);
			void DrawPause();
			int totalscore = 0;
			//AI
			StateGameObject* testStateObjectLeft;
			StateGameObject* testStateObjectRight;

			GameObject* player;
			GameObject* enemy;
			GameObject* soccer;
			GameObject* obs1;
			GameObject* obs2;
			GameObject* obs3;
			GameObject* obs4;

			Vector3 enemyPosition;
			Vector3 playerPosition;

			void TestPathfinding();
			void DisplayPathfinding();
			void Chasing();
			void Patrol();
			//void str2int(int& int_temp, const string& string_temp);


			int hitcount = 0;
			int readcount = 0;
			//AI
			
			//=========================

			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, const std::string& name = "");
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, const std::string& name = "");

			//==========================
			GameObject* AddCubeToWorld_OBB(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, const std::string& name = "");
			
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f, const std::string& name = "");

			GameObject* AddRightShootCube(const Vector3& position, const Vector3& dimensions, float inverseMass, const Vector3& shootDir, const std::string& name = "");
			GameObject* AddRightShootStart(const Vector3& position, Vector3 dimensions, float inverseMass, const std::string& name = "");
			void AddRightShootBoard(const Vector3& position, const Vector3& dimensions, float inverseMass, const Vector3& shootDir, float min, float max, const std::string& name = "");
			GameObject* AddLeftShootCube(const Vector3& position, const Vector3& dimensions, float inverseMass, const Vector3& shootDir, const std::string& name = "");
			GameObject* AddLeftShootStart(const Vector3& position, Vector3 dimensions, float inverseMass, const std::string& name = "");
			void AddLeftShootBoard(const Vector3& position, const Vector3& dimensions, float inverseMass, const Vector3& shootDir, float min, float max, const std::string& name = "");
			//==============================


			GameObject* AddPlayerToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddEnemyToWorld(const Vector3& position, const std::string& name = "");
			GameObject* AddBonusToWorld(const Vector3& position, const std::string& name = "");

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(10, 14, 0);  //change the value of offset to control the view position
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			class IntroScreen :public PushdownState {//for intro menu screen 
			protected:
				TutorialGame* game;
				bool GameMode = 0;
			public:
				IntroScreen(TutorialGame* game) {
					this->game = game;
				}
				PushdownResult OnUpdate(float dt, PushdownState** newstate) override {
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
						*newstate = new GameScreen(game, 0);
						return PushdownResult::Push;
					}
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
						*newstate = new GameScreen(game, 0);
						GameMode = 0;
						return PushdownResult::Push;
					}
					//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
					//	return PushdownResult::Pop;
					//}

					game->DrawMenu();

					return PushdownState::NoChange;
				}
				void OnAwake() override {
					std::cout << "welcome play my game, ESC to quit\n";
				}

				void OnSleep() override {
					game->InitWorld();
				}
			};

			class GameScreen :public PushdownState {
			protected:
				TutorialGame* game;
				float pausesave = 1;
				bool multiplayer;
			public:
				GameScreen(TutorialGame* game, bool multiplayer = 0) {
					game->multiplayer = multiplayer;
					this->game = game;
				}
				PushdownResult OnUpdate(float dt, PushdownState** newstate) override {
					if (pausesave < 0) {
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
							return PushdownResult::Pop;
						}
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
							*newstate = new PauseScreen(game);
							return PushdownResult::Push;
						}
					}
					else {
						pausesave -= dt;
					}

					game->UpdateGame(dt);
					return PushdownResult::NoChange;
				}
				void OnAwake() override {
					std::cout << "Resuming Game\n";
					pausesave = 0.2;
				}
			};

			class PauseScreen :public PushdownState {//for player press pause buttom screen 
			protected:
				TutorialGame* game;
				float pausesave = 1;
			public:
				PauseScreen(TutorialGame* game) {
					this->game = game;
				}
				PushdownResult OnUpdate(float dt, PushdownState** newstate) override {
					if (pausesave < 0) {
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
							return PushdownResult::Pop;
						}
					}
					else {
						pausesave -= dt;
					}
					game->UpdateGame(0);
					return PushdownResult::NoChange;
				}
				void OnAwake() override {
					std::cout << "press P to pause game" << std::endl;
					//renderer->DrawString("now game paused", Vector2(10, 10));
					pausesave = 0.2;
				}
			};
			void UpdateBot();
		};
	}
}

