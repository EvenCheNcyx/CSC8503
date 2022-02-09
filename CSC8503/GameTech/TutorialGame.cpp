#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "StateGameObject.h"
#include "../CSC8503Common/GameObject.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/SliderConstraint.h"
#include "../CSC8503Common/HingeConstraint.h"
#include "../CSC8503Common/PositionConstraint.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace NCL;
using namespace CSC8503;

std::ifstream infile;
std::ofstream outfile;

TutorialGame::TutorialGame()	{
	machine = new PushdownMachine(new IntroScreen(this));
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 100.0f;
	useGravity		= false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}



/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}



TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex; 
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}



void TutorialGame::UpdateGame(float dt) {
	//float totalscore = PhysicsSystem::ShowTotalScore();
	//Debug::Print("Total score : " + std::to_string(totalscore), Vector2(95, 95));
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	 
	UpdateKeys();
	//PhysicsSystem::UpdateResult(dt);
	physics->UpdateResult(dt);
	//ai
	//DisplayPathfinding();
	//UpdateBot();
	//Chasing();


	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}

	if (dt == 0) {
		DrawPause();
	}

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	//锁定项目视角更新
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	world->UpdateGameObjects(dt);
	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();

	if (testStateObjectLeft) {
		testStateObjectLeft->Update(dt);
	}
	if (testStateObjectRight) {
		testStateObjectRight->Update(dt);
	}
}



void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject	= nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(800.0f);
	world->GetMainCamera()->SetPitch(-20.0f);
	world->GetMainCamera()->SetYaw(180.0f);
	world->GetMainCamera()->SetPosition(Vector3(75, 10, -45));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	InitMixedGridWorld(15, 10, 3.0f, 3.0f);
	InitGameExamples();
	InitDefaultFloor();
	BridgeConstraintTest();
	//SliderContraintTest();
	//HingeContraintTest();
}


void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(1, 1, 1);

	float invCubeMass = 5;//how heavy the middle pieces are
	int numLinks = 7;
	float maxDistance = 4;//constraint distance
	float cubeDistance = 3;//distance between links

	Vector3 startPos = Vector3(0, 15, 10);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3(0, 0, (numLinks + 2) * cubeDistance), cubeSize, 0);
	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
	    world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::SliderContraintTest()
{
	GameObject* obj_a = AddCubeToWorld(Vector3(50, 50, 50), Vector3(1, 1, 1), 0);
	GameObject* obj_b = AddCubeToWorld(Vector3(60, 50, 50), Vector3(1, 1, 1), 5);
	obj_b->GetPhysicsObject();

	SliderConstraint* constraint = new SliderConstraint(obj_a, obj_b, Vector3(1, 0, 0), 5, 15);
	world->AddConstraint(constraint);
}

void TutorialGame::HingeContraintTest()
{
	GameObject* obj_a = AddCubeToWorld_OBB(Vector3(50, 50, 50), Vector3(1, 1, 1), 10);
	GameObject* obj_b = AddCubeToWorld_OBB(Vector3(60, 50, 50), Vector3(1, 1, 1), 10);
	obj_a->GetPhysicsObject();
	obj_b->GetPhysicsObject();

	HingeConstraint* constraint = new HingeConstraint(obj_a, obj_b, 2);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, const std::string& name) {
	GameObject* floor = new GameObject(name);

	Vector3 floorSize	= Vector3(100, 0.5, 50);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position, const std::string& name) {
	GameObject* wall = new GameObject(name);

	Vector3 wallSize = Vector3(75, 25, 0.5);
	AABBVolume* volume = new AABBVolume(wallSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform()
		.SetScale(wallSize * 2)
		.SetPosition(position);

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddxWallToWorld(const Vector3& position, const std::string& name) {
	GameObject* wall = new GameObject(name);

	Vector3 wallSize = Vector3(100, 25, 0.5);
	AABBVolume* volume = new AABBVolume(wallSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform()
		.SetScale(wallSize * 2)
		.SetPosition(position);

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddzWallToWorld(const Vector3& position, const std::string& name) {
	GameObject* wall = new GameObject(name);

	Vector3 wallSize = Vector3(0.5, 25, 50);
	AABBVolume* volume = new AABBVolume(wallSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform()
		.SetScale(wallSize * 2)
		.SetPosition(position);

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddSlopeToWorld(const Vector3& position, const std::string& name) {
	GameObject* wall = new GameObject(name);

	Vector3 wallSize = Vector3(10, 1, 50);
	OBBVolume* volume = new OBBVolume(wallSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform()
		.SetScale(wallSize * 2)
		.SetPosition(position)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 0, -15));

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddBounce1(const Vector3& position) {
	GameObject* BounceTile = new GameObject("bonce1");

	Vector3 BounceTileSize = Vector3(100, 0.1, 50);

	AABBVolume* volume = new AABBVolume(BounceTileSize / 2);

	BounceTile->SetBoundingVolume((CollisionVolume*)volume);
	BounceTile->GetTransform()
		.SetScale(BounceTileSize)
		.SetPosition(position);

	BounceTile->SetRenderObject(new RenderObject(&BounceTile->GetTransform(), cubeMesh, basicTex, basicShader));
	BounceTile->SetPhysicsObject(new PhysicsObject(&BounceTile->GetTransform(), BounceTile->GetBoundingVolume()));

	BounceTile->GetPhysicsObject()->SetInverseMass(0);
	BounceTile->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(BounceTile);

	return BounceTile;
}

GameObject* TutorialGame::AddBounce2(const Vector3& position) {
	GameObject* BounceTile = new GameObject("bonce2");

	Vector3 BounceTileSize = Vector3(50, 0.1, 70);

	AABBVolume* volume = new AABBVolume(BounceTileSize / 2);

	BounceTile->SetBoundingVolume((CollisionVolume*)volume);
	BounceTile->GetTransform()
		.SetScale(BounceTileSize)
		.SetPosition(position);

	BounceTile->SetRenderObject(new RenderObject(&BounceTile->GetTransform(), cubeMesh, basicTex, basicShader));
	BounceTile->SetPhysicsObject(new PhysicsObject(&BounceTile->GetTransform(), BounceTile->GetBoundingVolume()));

	BounceTile->GetPhysicsObject()->SetInverseMass(0);
	BounceTile->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(BounceTile);

	return BounceTile;
}

GameObject* TutorialGame::AddBounce3(const Vector3& position) {
	GameObject* BounceTile = new GameObject("bonce3");

	Vector3 BounceTileSize = Vector3(50, 0.1, 30);

	AABBVolume* volume = new AABBVolume(BounceTileSize / 2);

	BounceTile->SetBoundingVolume((CollisionVolume*)volume);
	BounceTile->GetTransform()
		.SetScale(BounceTileSize)
		.SetPosition(position);

	BounceTile->SetRenderObject(new RenderObject(&BounceTile->GetTransform(), cubeMesh, basicTex, basicShader));
	BounceTile->SetPhysicsObject(new PhysicsObject(&BounceTile->GetTransform(), BounceTile->GetBoundingVolume()));

	BounceTile->GetPhysicsObject()->SetInverseMass(0);
	BounceTile->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(BounceTile);

	return BounceTile;
}

GameObject* TutorialGame::AddBounce4(const Vector3& position) {
	GameObject* BounceTile = new GameObject("bonce4");

	Vector3 BounceTileSize = Vector3(50, 0.1, 50);

	AABBVolume* volume = new AABBVolume(BounceTileSize / 2);

	BounceTile->SetBoundingVolume((CollisionVolume*)volume);
	BounceTile->GetTransform()
		.SetScale(BounceTileSize)
		.SetPosition(position);

	BounceTile->SetRenderObject(new RenderObject(&BounceTile->GetTransform(), cubeMesh, basicTex, basicShader));
	BounceTile->SetPhysicsObject(new PhysicsObject(&BounceTile->GetTransform(), BounceTile->GetBoundingVolume()));

	BounceTile->GetPhysicsObject()->SetInverseMass(0);
	BounceTile->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(BounceTile);

	return BounceTile;
}

GameObject* TutorialGame::AddTargetFloorToWorld(const Vector3& position, const std::string& name) {
	GameObject* floor = new GameObject(name);

	Vector3 floorSize = Vector3(50, 0.1, 25);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

StateGameObject* TutorialGame::AddDoorToWorld(const Vector3& position)
{
	StateGameObject* door = new StateGameObject("Left");

	Vector3 autoDoorSize = Vector3(2, 10, 8);

	AABBVolume* volume = new AABBVolume(autoDoorSize / 2);

	door->SetBoundingVolume((CollisionVolume*)volume);
	door->GetTransform()
		.SetScale(autoDoorSize)
		.SetPosition(position);

	door->SetRenderObject(new RenderObject(&door->GetTransform(), cubeMesh, basicTex, basicShader));
	door->SetPhysicsObject(new PhysicsObject(&door->GetTransform(), door->GetBoundingVolume()));

	door->GetPhysicsObject()->SetInverseMass(1.0f);
	door->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(door);

	return door;
}

//shoot constraint
namespace NCL {
	namespace CSC8503
	{
		class ShootObject : public GameObject
		{
		public:
			ShootObject(const std::string& name) {
				GameObject* shoot = new GameObject(name);
			};
			virtual ~ShootObject() {};

			void SetShootDir(const Vector3& dir) { shootDir = dir; };
			void TriggerShoot(float force) { GetPhysicsObject()->AddForce(shootDir * force); };

		protected:
			Vector3 shootDir = Vector3(0, 0, -1);
		};
	}
}

GameObject* TutorialGame::AddRightShootStart(const Vector3& position, Vector3 dimensions, float inverseMass, const std::string& name) {
	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 90, 0));

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddRightShootCube(const Vector3& position, const Vector3& dimensions, float inverseMass, const Vector3& shootDir, const std::string& name) {
	ShootObject* cube = new ShootObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2).SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 90, 0));

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetShootDir(shootDir);

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::AddRightShootBoard(const Vector3& position, const Vector3& dimensions, float inverseMass = 1.5f, const Vector3& shootDir = Vector3(-2, 0, 0), float min = 3.0f, float max = 20.0f, const std::string& name)
{
	GameObject* silder_0_A = AddRightShootStart(position + -shootDir * min, Vector3(5, 5, 1), 0.0f, name + "A");
	silder_0_A->GetPhysicsObject()->SetInverseMass(0.0f);
	GameObject* silder_0_B = AddRightShootCube(position, dimensions, inverseMass, shootDir, name + "B");
	//silder_0_B->GetPhysicsObject()->SetElasticity(1);
	SliderConstraint* silder_0_con = new SliderConstraint(silder_0_A, silder_0_B, shootDir, min, max);
	world->AddConstraint(silder_0_con);

}
//======================//

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, const std::string& name) {
	GameObject* sphere = new GameObject(name);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass, const std::string& name) {
	GameObject* capsule = new GameObject(name);

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, const std::string& name) {
	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddCubeToWorld_OBB(const Vector3& position, Vector3 dimensions, float inverseMass, const std::string& name)
{
	GameObject* cube = new GameObject(name);

	OBBVolume* volume = new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}


void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, 0, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3((x - 28) * colSpacing, 10.0f, (z - 5) * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -0.5, 0));
	AddxWallToWorld(Vector3(0, 25, 50));
	AddxWallToWorld(Vector3(0, 25, -50));
	AddzWallToWorld(Vector3(100, 25, 0));
	AddzWallToWorld(Vector3(-100, 25, 0));
	AddWallToWorld(Vector3(25, 25, 0));//,"lose"
	AddSlopeToWorld(Vector3(0, 1, 0));

	AddSphereToWorld(Vector3(70, 1, -25), 2.0f,10.0f,"ball");
	AddBounce1(Vector3(0, 0, -25));
	AddBounce2(Vector3(-75, 0, -15));
	AddBounce3(Vector3(-75, 0, 35));
	AddBounce4(Vector3(-25, 0, 25));
	AddDoorToWorld(Vector3(-12.5, 1, -10));
	AddTargetFloorToWorld(Vector3(50, 0, 25),"win");

	//other func
	AddCapsuleToWorld(Vector3(10, 100, 10),2.0f,1.0f);
	AddSphereToWorld(Vector3(10, 100,15), 2.0f, 10.0f );
	AddRightShootBoard(Vector3(20, 100, 20), Vector3(5, 10, 1));
}

void TutorialGame::InitGameExamples() {
	player = AddPlayerToWorld(Vector3(85, 1, -25));
	enemy = //AddEnemyToWorld(Vector3(-80, 1, 0));
	AddBonusToWorld(Vector3(-10, 10, 25),"bonus");
	AddBonusToWorld(Vector3(65, 3, -25), "bonus1");
	AddBonusToWorld(Vector3(-50, 3, 20), "bonus2");
	AddBonusToWorld(Vector3(-50, 3, -20), "bonus3");
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position, const std::string& name) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject(name);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 90, 0));

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position, const std::string& name) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject(name);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position, const std::string& name) {
	GameObject* apple = new GameObject(name);

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.5, 0.5, 0.5))
		.SetPosition(position)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, 90, 0));

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(0.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject)
	{
		return; //we haven't selected anything
	}

	//push the selected object
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT))
	{
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true))
		{
			/*no tortue
			if (closestCollision.node == selectionObject)
			{
				selectionObject->GetPhysicsObject()->AddForce(ray.GetDirection() * forceMagnitude);
			}
			*/

			if (closestCollision.node == selectionObject)
			{
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}

		}
	}
}


//====MENU=========//
void TutorialGame::DrawMenu() {
	glClearColor(0, 0, 0, 1);
	Debug::FlushRenderables(0);
	renderer->DrawString("Welcome to game", Vector2(10, 10));
	renderer->DrawString("Press '1' for game", Vector2(10, 20));
	renderer->Render();
	world->ClearAndErase();
	physics->Clear();
}
//void TutorialGame::DrawWin() {
//	renderer->DrawString("you win", Vector2(40, 50));
//}
//void TutorialGame::DrawLose(std::string winnner) {
//	renderer->DrawString(winnner, Vector2(40, 50));
//}
void TutorialGame::DrawPause() {
	renderer->DrawString("now game paused", Vector2(40, 50));
}
//====MENU=========//


//======AI========//
vector<Vector3> testNodes;

void TutorialGame::DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));

	}
}

void TutorialGame::UpdateBot()
{
	if (!testNodes.empty()) {
		Vector3 forceDir = testNodes[0] - obs4->GetTransform().GetWorldPosition();
		forceDir.Normalise();
		Debug::DrawLine(obs4->GetTransform().GetWorldPosition(), testNodes[0], Vector4(1, 1, 1, 1));
		float length = (testNodes[0] - obs4->GetTransform().GetWorldPosition()).Length();
		if (length < 100.f) {
			testNodes.erase(testNodes.begin());
		}
		else {
			obs4->GetPhysicsObject()->ApplyLinearImpulse(forceDir * 50.f);
		}
	}
}

void TutorialGame::Chasing() {
	enemyPosition = enemy->GetTransform().GetPosition();
	playerPosition = player->GetTransform().GetPosition();
	if (enemyPosition.x - playerPosition.x < 100 && enemyPosition.x - playerPosition.x >-100 && enemyPosition.z - playerPosition.z<50 && enemyPosition.z - playerPosition.z>-50) {

		enemy->GetPhysicsObject()->AddForce(Vector3(playerPosition.x - enemyPosition.x, 0, playerPosition.z - enemyPosition.z));
		if (enemyPosition.x - playerPosition.x < 20 && enemyPosition.x - playerPosition.x >-20 && enemyPosition.z - playerPosition.z<20 && enemyPosition.z - playerPosition.z>-20) {
			if (forceMagnitude > 100.0f || forceMagnitude < -100.0f) {
				std::cout << "You destroyed the enemy!" << std::endl;
				enemy->GetTransform().SetWorldPosition(enemyPosition);
			}
			else {
				std::cout << "Restart!" << std::endl;
				InitWorld();
				InitCamera();
				selectionObject = nullptr;
			}
		}
	}
	else {
		Patrol();
	}

}

void TutorialGame::Patrol() {
	enemy->GetPhysicsObject()->AddForce(Vector3(rand() % (500 - (-500) + 1) - 500, 0, rand() % (500 - (-500) + 1) - 500));
}
//======AI========//