#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Prop.hpp"
#include "Game/objReader.hpp"
#include "Game/Portal.hpp"
#include "Game/glTFReader.hpp"

#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/Texture.hpp"
// TODO: Make gamestate set by an enum value. DO current state and next state, 
// when these are different know we're switching states, run current state frame 
// and then next frame switch. switch at top of update. enum has invalidstate as -1. 
// Default is invalid state.

Game::Game()
{
	Startup();
}

Game::~Game()
{
	delete m_randomNumberGenerator;

	m_randomNumberGenerator = nullptr;

	for (int entityIndex = 0; entityIndex < m_entities.size(); ++entityIndex)
	{
		delete m_entities[entityIndex];
		m_entities[entityIndex] = nullptr;
	}
}

void Game::Update()
{
	if (m_nextGameState != m_currentGameState)
	{
		m_currentGameState = m_nextGameState;
	}

	switch (m_currentGameState)
	{
		case GAME_STATE_ATTRACT: Update_AttractMode(); break;
		case GAME_STATE_MAIN: Update_MainMode(); break;
	}
}

void Game::Render() const
{
	switch (m_currentGameState)
	{
		case GAME_STATE_ATTRACT: Render_AttractMode(); break;
		case GAME_STATE_MAIN: Render_MainMode(); break;
	}
}

void Game::Startup()
{
	DebugRenderSetVisible();

	DebugAddWorldBasis(Mat44(), -1.f);
	EulerAngles xTextEulerAngles = EulerAngles(-90.f, 0.f, 0.f);
	DebugAddWorldText("x - forward", xTextEulerAngles.GetAsMatrix_IFwd_JLeft_KUp(), 0.3f, Vec2(-0.05f, -0.2f), -1.f, Rgba8::RED);
	EulerAngles yTextEulerAngles = EulerAngles(180.f, 0.f, 0.f);
	DebugAddWorldText("y - left", yTextEulerAngles.GetAsMatrix_IFwd_JLeft_KUp(), 0.3f, Vec2(1.05f, -0.2f), -1.f, Rgba8::GREEN);
	EulerAngles zTextEulerAngles = EulerAngles(0.f, 0.f, 90.f);
	DebugAddWorldText("z - up", zTextEulerAngles.GetAsMatrix_IFwd_JLeft_KUp(), 0.3f, Vec2(-0.05f, -0.2f), -1.f, Rgba8::BLUE);

	m_randomNumberGenerator = new RandomNumberGenerator();
	m_gameClock = new Clock();

	Startup_PopulateFromBlackboard();

	m_player = new Player(this, Vec3(-5.f, 0.f, 1.f));
	m_entities.push_back(m_player);
	m_playerTranslationThisFrame = new Vec3();

	m_player->m_worldCamera = new Camera(Vec2(-1, -1), Vec2(1, 1));
	m_player->m_screenCamera = new Camera();

	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);

	//m_worldCamera->SetOrthoView(Vec2(WORLD_MIN_X, WORLD_MIN_Y), Vec2(WORLD_MAX_X, WORLD_MAX_Y));
	m_player->m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
	m_player->m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);

	m_player->m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_cubeProp = new Prop(this, Vec3(2.f, 2.f, 0.f));
	m_entities.push_back(m_cubeProp);
	m_cubeProp->m_color = Rgba8((unsigned char)200.f, (unsigned char)200.f, (unsigned char)200.f, (unsigned char)255.f);

	// Make the first prop a cube
	AABB3 cubeAABB3 = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
	Rgba8 colors[6];
	colors[0] = Rgba8::CYAN; // Front side -X
	colors[1] = Rgba8::MAGENTA; // Right side -Y
	colors[2] = Rgba8::RED; // Back side +X
	colors[3] = Rgba8::GREEN; // Left side +Y
	colors[4] = Rgba8::BLUE; // Top side +Z
	colors[5] = Rgba8::YELLOW; // Bottom side -Z
	AddVertsForAABB3D(m_cubeProp->m_vertexes, cubeAABB3, colors);

	m_sphereProp = new Prop(this, Vec3(5.f, 5.f, 0.f));
	m_entities.push_back(m_sphereProp);
	m_sphereProp->m_isIndexed = true;
	AddVertsForSphere3D(m_sphereProp->m_vertexes, m_sphereProp->m_indexes, Vec3(), 1.f, Rgba8::WHITE, AABB2(Vec2(0.f, 0.5f), Vec2(1.f, 1.f)), 32, 16);
	m_sphereProp->m_texture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/blaine.png");

	m_cylinderProp = new Prop(this, Vec3(-1.f, 5.f, 0.f));
	m_entities.push_back(m_cylinderProp);
	AddVertsForArrow3D(m_cylinderProp->m_vertexes,Vec3(0.f, 0.f, -0.5f), Vec3(0.f, 0.5f, 0.5f), 0.5f, Rgba8::WHITE);
	m_cylinderProp->m_texture = g_engine->m_render->CreateOrGetTextureFromFile("Data/Images/blaine.png");

	// Cube 2 is transparent. You will only be able to see props which render BEFORE cube 2 when looking through cube 2. Anything rendered after cube 2 will not be visible through it.
	m_cubeProp2 = new Prop(this, Vec3(5.f, -5.f, 0.f));
	m_entities.push_back(m_cubeProp2);
	AddVertsForAABB3D(m_cubeProp2->m_vertexes, cubeAABB3, colors);

	//m_blaineProp = new Prop(this, Vec3(0.f, 0.f, 2.f));
	//m_blaineProp->m_OBJModel = g_app->m_blaineModel;
	//m_entities.push_back(m_blaineProp);
	//m_blaineProp->m_isLookingAtOther = true;

	m_venipedeProp = new Prop(this, Vec3(0.f, -2.f, 0.15f));
	m_venipedeProp->m_OBJModel = g_app->m_venipedeModel;
	m_entities.push_back(m_venipedeProp);

	//std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc;
	//inputElementDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
	//0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	//inputElementDesc.push_back({ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM,
	//0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	//inputElementDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
	//0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	m_useTexture1Shader = g_engine->m_render->CreateShader("OutlineShader");
	m_drawToTexShader = g_engine->m_render->CreateShader("DrawToTex");

	m_thresherProp = new Prop(this, Vec3(20.f, -10.f, 0.f));
	m_thresherProp->m_OBJModel = g_app->m_6sharksModel;
	m_thresherProp->AddObjectToRender("obj4");
	m_thresherProp->m_orientation.m_yawDegrees = -140.f;
	m_thresherProp->m_orientation.m_rollDegrees = 90.f;
	m_entities.push_back(m_thresherProp);

	Portal* newPorta11 = new Portal(this, Vec3(-1.f, 3.f, 0.9f));
	m_portals.push_back(newPorta11);
	m_entities.push_back(newPorta11);
	Portal* newPorta12 = new Portal(this, Vec3(-1.f, -3.f, 0.9f));
	m_entities.push_back(newPorta12);
	m_portals.push_back(newPorta12);
	newPorta11->AssignPortal(newPorta12);
	newPorta12->AssignPortal(newPorta11);

	m_dragonProp = new Prop(this, Vec3(0.f, 0.f, 10.f));
	//m_dragonProp->m_GLTFModel = g_app->m_dragonModel;
	//g_app->m_dragonModel->Test_AddVertsForModel(m_dragonProp->m_vertexes, m_dragonProp->m_indexes);
	//m_dragonProp->m_texture = nullptr;
	//m_entities.push_back(m_dragonProp);

	int numOfTheWind = 100;
	for (int theWindIndex = 0; theWindIndex < numOfTheWind; ++theWindIndex)
	{
		m_theWind.push_back(new Prop(this, Vec3(0.f, 0.f, 0.f)));
		m_theWind[theWindIndex]->m_OBJModel = g_app->m_theWindModel;
		m_theWind[theWindIndex]->m_isLookingAtOther = true;
	}

	m_theWindTimer = new Timer(6.f, m_gameClock);
	//m_theWindTimer->Start();

	m_theWindGoAwayTimer = new Timer(0.7f, m_gameClock);

	int numOfGridLines = 100;
	int halfNumOfGridLines = 100 / 2;
	float gridLineSize = 0.01f;
	AABB3 gridLineAABB3 = AABB3(Vec3(-gridLineSize, (float)halfNumOfGridLines, -gridLineSize), Vec3(gridLineSize, -(float)halfNumOfGridLines, gridLineSize));
	AABB3 biggerGridLineAABB3 = AABB3(Vec3(-gridLineSize * 2.f, (float)halfNumOfGridLines, -gridLineSize * 2.f), Vec3(gridLineSize * 2.f, -(float)halfNumOfGridLines, gridLineSize * 2.f));
	AABB3 halfwayPointGridLineAABB3 = AABB3(Vec3(-gridLineSize * 4.f, (float)halfNumOfGridLines, -gridLineSize * 4.f), Vec3(gridLineSize * 4.f, -(float)halfNumOfGridLines, gridLineSize * 4.f));
	for (int gridLineIndex = 0; gridLineIndex < numOfGridLines + 1; ++gridLineIndex)
	{
		int currentLinePos = gridLineIndex - halfNumOfGridLines;
		m_gridXLines.push_back(new Prop (this, Vec3(0.f, (float)currentLinePos, 0.f)));
		m_gridYLines.push_back(new Prop(this, Vec3((float)currentLinePos, 0.f, 0.f)));

		if (gridLineIndex % halfNumOfGridLines == 0)
		{
			AddVertsForAABB3D(m_gridXLines[gridLineIndex]->m_vertexes, halfwayPointGridLineAABB3, Rgba8::RED);
			AddVertsForAABB3D(m_gridYLines[gridLineIndex]->m_vertexes, halfwayPointGridLineAABB3, Rgba8::GREEN);
		}
		else if (gridLineIndex % 5 == 0)
		{
			AddVertsForAABB3D(m_gridXLines[gridLineIndex]->m_vertexes, biggerGridLineAABB3, Rgba8(200, 0, 0, 255));
			AddVertsForAABB3D(m_gridYLines[gridLineIndex]->m_vertexes, biggerGridLineAABB3, Rgba8(0, 200, 0, 255));
		}
		else
		{
			AddVertsForAABB3D(m_gridXLines[gridLineIndex]->m_vertexes, gridLineAABB3, Rgba8(150, 150, 150, 255));
			AddVertsForAABB3D(m_gridYLines[gridLineIndex]->m_vertexes, gridLineAABB3, Rgba8(150, 150, 150, 255));
		}
		m_gridXLines[gridLineIndex]->m_orientation.m_yawDegrees += 90.f;

		m_entities.push_back(m_gridXLines[gridLineIndex]);
		m_entities.push_back(m_gridYLines[gridLineIndex]);
	}
}

void Game::AddScreenShake(float screenShake)
{
	m_screenShakeFraction += screenShake;
}

void Game::AddHitStop(float hitStop)
{
	m_hitStopTimer += hitStop;
	m_isHitStop = true;
}

void Game::RenderAllPortals() const
{
	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		m_portals[portalIndex]->RenderPortal();
	}
}

void Game::Startup_PopulateFromBlackboard()
{
	m_perspectiveFOV = g_gameConfigBlackboard.GetValue("perspectiveFOV", 0.f);
	m_rollSensitivity = g_gameConfigBlackboard.GetValue("rollSensitivity", 0.f);
	m_mouseSensitivity = g_gameConfigBlackboard.GetValue("mouseSensitivity", 0.f);
	m_controllerSensitivity = g_gameConfigBlackboard.GetValue("controllerSensitivity", 0.f);
	m_moveSpeed = g_gameConfigBlackboard.GetValue("moveSpeed", 0.f);
	m_colorUndulateTime = g_gameConfigBlackboard.GetValue("colorUndulateTime", 0.f);
}

void Game::Update_AttractMode()
{
	// Keyboard Inputs
	if (g_engine->m_input->WasKeyJustPressed(' ') or g_engine->m_input->WasKeyJustPressed('N'))
	{
		ChangeGameState(GameState::GAME_STATE_MAIN);
	}

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->SetIsQuitting();
	}

	// Xbox Controller Inputs
	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::GAMEPAD_B) or g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::GAMEPAD_A))
	{
		ChangeGameState(GameState::GAME_STATE_MAIN);
	}

	if (g_engine->m_input->m_controllers[0].WasButtonJustPressed(XboxButtonID::BACK))
	{
		g_app->SetIsQuitting();
	}
}

void Game::Render_AttractMode() const
{
	g_engine->m_render->ClearScreen(Rgba8(0, 0, 0, 255));

	g_engine->m_render->BeginCamera(m_player->m_screenCamera);

	Vertex vertices[] = {
		Vertex(Vec3(600.f, 300.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
		Vertex(Vec3(1000.f, 300.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
		Vertex(Vec3(800.f, 600.f, 0.0f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f))
	};

	g_engine->m_render->BindTexture(NULL);
	g_engine->m_render->DrawVertexArray(3, vertices);

	g_engine->m_render->EndCamera(m_player->m_screenCamera);
}

void Game::Update_MainMode()
{
	float deltaSeconds = (float)m_gameClock->GetDeltaSeconds();

	bool didGameReset = Update_MainMode_KeyboardInput();
	if (didGameReset)
	{
		return;
	}

	didGameReset = Update_MainMode_ControllerInput();
	if (didGameReset)
	{
		return;
	}

	AABB2 screenBox = AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	std::string timeInformationText = Stringf("Time: %.2f FPS: %.1f Scale: %.2f", m_gameClock->GetTotalSeconds(), m_gameClock->GetFrameRate(), m_gameClock->GetTimeScale());
	DebugAddScreenText(timeInformationText, screenBox, SCREEN_SIZE_Y * 0.02f, Vec2(0.99f, 0.99f), 0.f);
	std::string playerPositionText = Stringf("Player position: %.2f, %.2f, %.2f", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddMessage(playerPositionText, 0.f);

	Update_MainMode_Entities();

	Update_MainMode_PlayerCCD();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Cube 1
	float cubeRotationStrength = 30.f;
	m_cubeProp->m_orientation.m_yawDegrees = m_cubeProp->m_orientation.m_yawDegrees + (cubeRotationStrength * deltaSeconds);
	m_cubeProp->m_orientation.m_pitchDegrees = m_cubeProp->m_orientation.m_pitchDegrees + (cubeRotationStrength * deltaSeconds);

	//m_cubeProp->m_position = m_portals[0]->m_portalCamera->GetPosition();
	//m_cubeProp->m_orientation = m_portals[0]->m_portalCamera->GetOrientation();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Cube 2
	m_colorUndulateTime += deltaSeconds;
	float colorFraction = CosDegrees(m_colorUndulateTime * 100.f);
	float newColor = RangeMap(colorFraction, -1.f, 1.f, 10.f, 255.f);
	m_cubeProp2->m_color = Rgba8((unsigned char)newColor, (unsigned char)newColor, (unsigned char)newColor, (unsigned char)newColor);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Sphere 1
	float sphereRotationStrength = 45.f;
	m_sphereProp->m_orientation.m_yawDegrees = m_sphereProp->m_orientation.m_yawDegrees + (sphereRotationStrength * deltaSeconds);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Venipede
	float venipedeWalkSpeed = 1.f;
	float venipedeStopDistanceFromPlayer = 1.f;
	float venipedeTurnStrength = 90.f;

	Vec3 venipedeToPlayer = m_player->m_position - m_venipedeProp->m_position;
	Vec2 clampedVenipedeToPlayer = Vec2(venipedeToPlayer.x, venipedeToPlayer.y);
	float desiredDegrees = clampedVenipedeToPlayer.GetOrientationDegrees() + 180.f;
	float shortestDegreesToDesiredYaw = GetShortestAngularDispDegrees(m_venipedeProp->m_orientation.m_yawDegrees, desiredDegrees);
	float maximumTurnThisFrame = venipedeTurnStrength * deltaSeconds;
	float turnDegrees = GetClamped(shortestDegreesToDesiredYaw, -maximumTurnThisFrame, maximumTurnThisFrame);
	m_venipedeProp->m_orientation.m_yawDegrees += turnDegrees;

	if (clampedVenipedeToPlayer.GetLength() >= venipedeStopDistanceFromPlayer)
	{
		Mat44 lookAtMatrix = m_venipedeProp->GetLookAtMatrix(m_player->m_position, m_player->GetModelToWorldTransform().GetKBasis3D());
		Vec3 lookAtForwardVector = lookAtMatrix.GetIBasis3D();
		Vec3 forwardVectorClampedXY = Vec3(lookAtForwardVector.x, lookAtForwardVector.y, 0.f).GetNormalized();
		Vec3 walkDistance = forwardVectorClampedXY * -(venipedeWalkSpeed * deltaSeconds);
		m_venipedeProp->m_position += walkDistance;
	}

	// Camera updates
	if (m_player->m_worldCamera != nullptr)
	{
		m_player->m_worldCamera->SetPerspectiveView(g_engine->m_window->m_config.m_clientAspect, m_perspectiveFOV, 0.1f, 100.f);
	}
	if (m_player->m_screenCamera != nullptr)
	{
		m_player->m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	}

	if (m_screenShakeFraction != 0.f)
	{
		float screenShakeX = (m_randomNumberGenerator->RollRandomFloatInRange(-m_screenShakeFraction, m_screenShakeFraction) * 1);
		float screenShakeY = (m_randomNumberGenerator->RollRandomFloatInRange(-m_screenShakeFraction, m_screenShakeFraction) * 1);
		screenShakeX *= screenShakeX;
		screenShakeY *= screenShakeY;
		m_player->m_worldCamera->Translate2D(Vec2(screenShakeX, screenShakeY));
		m_screenShakeFraction -= (float)m_gameClock->GetDeltaSeconds();
		if (m_screenShakeFraction < 0.f)
		{
			m_screenShakeFraction = 0.f;
		}
	}
}

void Game::Render_MainMode() const
{
	g_engine->m_render->ClearScreen(m_backgroundClearColor);

	g_engine->m_render->BeginCamera(m_player->m_worldCamera);

	// Render Entities
	RenderAllEntities();
	RenderAllPortals();

	g_engine->m_render->EndCamera(m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_player->m_screenCamera);

	DebugRenderScreen(*m_player->m_screenCamera);

	g_engine->m_render->EndCamera(m_player->m_screenCamera);
}

void Game::Update_MainMode_Entities()
{
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	for (int entityIndex = 0; entityIndex < m_entities.size(); ++entityIndex)
	{
		Entity* entity = m_entities[entityIndex];
		if (entity != nullptr)
		{
			entity->Update();
		}
	}

	if (m_theWindTimer->DecrementPeriodIfElapsed())
	{
		m_theWindTimer->Stop();
		m_isTheWindActive = true;
		m_theWindGoAwayTimer->Start();

		float minValue = 1.f;
		float maxValue = 8.f;
		for (int theWindIndex = 0; theWindIndex < m_theWind.size(); ++theWindIndex)
		{
			Prop* currentWind = m_theWind[theWindIndex];
			if (currentWind != nullptr)
			{
				float newX = m_randomNumberGenerator->RollRandomFloatInRange(minValue, maxValue);
				float newY = m_randomNumberGenerator->RollRandomFloatInRange(minValue, maxValue);
				float newZ = m_randomNumberGenerator->RollRandomFloatInRange(minValue, maxValue);
				int randomNegate = m_randomNumberGenerator->RollRandomIntInRange(0, 1);
				if (randomNegate == 0)
				{
					newX = -newX;
				}
				randomNegate = m_randomNumberGenerator->RollRandomIntInRange(0, 1);
				if (randomNegate == 0)
				{
					newY = -newY;
				}
				randomNegate = m_randomNumberGenerator->RollRandomIntInRange(0, 1);
				if (randomNegate == 0)
				{
					newZ = -newZ;
				}
				Vec3 newPosition = m_player->m_position + Vec3(newX, newY, newZ);
				currentWind->m_position = newPosition;
			}
		}
	}

	if (m_theWindGoAwayTimer->DecrementPeriodIfElapsed())
	{
		m_isTheWindActive = false;
		m_theWindGoAwayTimer->Stop();
		m_theWindTimer->Start();
		m_theWindTimer->m_period = m_randomNumberGenerator->RollRandomFloatInRange(2.f, 8.5f);
	}
}

bool Game::Update_MainMode_KeyboardInput()
{
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->GameReset();
		return true;
	}

	if (g_engine->m_input->IsKeyDown('T'))
	{
		m_gameClock->SetTimeScale(0.1f);
	}
	else
	{
		m_gameClock->SetTimeScale(1.f);
	}

	float currentMoveSpeed = m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 10.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
	}
	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_player->m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_player->m_position;
	Vec3 newAddedPosition;
	if (g_engine->m_input->IsKeyDown('A'))
	{
		newAddedPosition.y += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('D'))
	{
		newAddedPosition.y -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('W'))
	{
		newAddedPosition.x += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('S'))
	{
		newAddedPosition.x -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	Mat44 orientationMatrix = newOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	orientationMatrix.AppendTranslation3D(newAddedPosition);
	Vec3 newTranslation = orientationMatrix.GetTranslation3D();

	if (g_engine->m_input->IsKeyDown('Z'))
	{
		newTranslation.z -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (g_engine->m_input->IsKeyDown('C'))
	{
		newTranslation.z += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}

	m_playerTranslationThisFrame->x = newTranslation.x;
	m_playerTranslationThisFrame->y = newTranslation.y;
	m_playerTranslationThisFrame->z = newTranslation.z;

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_player->m_position = Vec3();
		m_player->m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}

	Update_MainMode_KeyboardInput_DebugRender();

	if (g_engine->m_input->WasKeyJustPressed('8'))
	{
		m_portals[0]->m_position = m_player->m_position + (m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f);
		m_portals[0]->m_orientation.m_yawDegrees = m_player->m_orientation.m_yawDegrees;
	}
	if (g_engine->m_input->WasKeyJustPressed('9'))
	{
		m_portals[1]->m_position = m_player->m_position + (m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f);
		m_portals[1]->m_orientation.m_yawDegrees = m_player->m_orientation.m_yawDegrees;
	}

	return false;
}

void Game::Update_MainMode_KeyboardInput_DebugRender()
{
	if (g_engine->m_input->WasKeyJustPressed('1'))
	{
		Vec3 end = (m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 20.f);
		DebugAddWorldCylinder(Vec3(0.f,0.f,0.f), end, m_player->m_position, 0.0625f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	}
	if (g_engine->m_input->IsKeyDown('2'))
	{
		Vec3 center = Vec3(m_player->m_position.x, m_player->m_position.y, 0.f);
		DebugAddWorldSphere(center, 0.2f, 60.f, Rgba8(150, 75, 0), Rgba8(150, 75, 0));
	}
	if (g_engine->m_input->WasKeyJustPressed('3'))
	{
		Vec3 center = m_player->m_position + (m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 2.f);
		DebugAddWorldWireSphere(center, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED);
	}
	if (g_engine->m_input->WasKeyJustPressed('4'))
	{
		DebugAddBasis(m_player->GetModelToWorldTransform(), 20.f, 1.f, 0.2f);
	}
	if (g_engine->m_input->WasKeyJustPressed('5'))
	{
		std::string positionText = Stringf("Player Position: %.2f, %.2f, %.2f", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
		std::string orientationText = Stringf("\nPlayer Orientation: %.2f, %.2f, %.2f", m_player->m_orientation.m_yawDegrees, m_player->m_orientation.m_pitchDegrees, m_player->m_orientation.m_rollDegrees);
		DebugAddWorldBillboardText(positionText + orientationText, m_player->m_position, 0.125f, Vec2(0.5f, 0.5f), 10.f, Rgba8::WHITE, Rgba8::WHITE);
	}
	if (g_engine->m_input->WasKeyJustPressed('6'))
	{
		DebugAddWorldWireCylinder(Vec3(0.f, 0.f, -0.5f), Vec3(0.f, 0.f, 0.5f), m_player->m_position, 0.5f, 10.f, Rgba8::WHITE, Rgba8::RED);
	}
	if (g_engine->m_input->WasKeyJustPressed('7'))
	{
		DebugAddMessage("Yaw: " + std::to_string(m_player->m_orientation.m_yawDegrees) +
			" Pitch: " + std::to_string(m_player->m_orientation.m_pitchDegrees) +
			" Roll: " + std::to_string(m_player->m_orientation.m_rollDegrees), 5.f);
	}
}

bool Game::Update_MainMode_ControllerInput()
{
	XboxController* controller = &g_engine->m_input->m_controllers[0];
	if (controller->WasButtonJustPressed(XboxButtonID::BACK))
	{
		g_app->GameReset();
		return true;
	}

	float currentMoveSpeed = m_moveSpeed;
	if (controller->IsButtonDown(XboxButtonID::GAMEPAD_A))
	{
		currentMoveSpeed *= 10.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_player->m_orientation;
	newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * m_controllerSensitivity;
	newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * m_controllerSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	
	newOrientation.m_rollDegrees += controller->GetRightTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;
	newOrientation.m_rollDegrees -= controller->GetLeftTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_rollSensitivity;

	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_player->m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_player->m_position;
	Vec3 newAddedPosition;
	newAddedPosition.y -= controller->GetLeftStick().GetPosition().x * (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	newAddedPosition.x += controller->GetLeftStick().GetPosition().y * (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;

	Mat44 orientationMatrix = newOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	orientationMatrix.AppendTranslation3D(newAddedPosition);
	Vec3 newTranslation = orientationMatrix.GetTranslation3D();

	if (controller->IsButtonDown(XboxButtonID::LEFT_SHOULDER))
	{
		newTranslation.z -= (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}
	if (controller->IsButtonDown(XboxButtonID::RIGHT_SHOULDER))
	{
		newTranslation.z += (float)s_systemClock->GetDeltaSeconds() * currentMoveSpeed;
	}

	if (newTranslation.x != 0.f)
	{
		m_playerTranslationThisFrame->x = newTranslation.x;
	}
	if (newTranslation.y != 0.f)
	{
		m_playerTranslationThisFrame->y = newTranslation.y;
	}
	if (newTranslation.z != 0.f)
	{
		m_playerTranslationThisFrame->z = newTranslation.z;
	}

	if (controller->IsButtonDown(XboxButtonID::START))
	{
		m_player->m_position = Vec3();
		m_player->m_orientation = EulerAngles();
	}

	return false;
}

void Game::Update_MainMode_PlayerCCD()
{
	Vec3 rayStart = m_player->m_position;
	Vec3 rayFwdNormal = m_playerTranslationThisFrame->GetNormalized();
	float rayLength = m_playerTranslationThisFrame->GetLength();

	for (int portalIndex = 0; portalIndex < m_portals.size(); ++portalIndex)
	{
		Portal* portal = m_portals[portalIndex];
		Vec3 portalToPlayer = m_player->m_position - portal->m_position;
		Vec3 portalFwdVector = portal->m_orientation.GetForwardDir_IFwd_JLeft_KUp();
		float PtPdotPFwd = DotProduct3D(portalToPlayer, portalFwdVector);
		if (PtPdotPFwd > 0.f)
		{
			portal->m_isPlayerOnFrontSide = true;
		}
		else
		{
			portal->m_isPlayerOnFrontSide = false;
		}
		Mat44 portalTransform = portal->m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		Mat44 otherPortalTransform = portal->GetOtherPortal()->m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		Vec3 bottomLeft = portalTransform.TransformPosition3D(portal->bl);
		Vec3 bottomRight = portalTransform.TransformPosition3D(portal->br);
		Vec3 topRight = portalTransform.TransformPosition3D(portal->tr);
		Vec3 topLeft = portalTransform.TransformPosition3D(portal->tl);
		RaycastResult3D raycastResult = RaycastVSQuad3D(rayStart, rayFwdNormal, rayLength, 
			bottomLeft + portal->m_position,
			bottomRight + portal->m_position,
			topRight + portal->m_position,
			topLeft + portal->m_position
		);
		if (raycastResult.m_didImpact)
		{
			portalToPlayer = m_player->m_position - portal->m_position;
			portalTransform.Transpose();
			//portalToPlayer = portalTransform.TransformPosition3D(portalToPlayer);
			portalToPlayer = otherPortalTransform.TransformPosition3D(portalToPlayer);
			m_player->m_position = portal->GetOtherPortal()->m_position + portalToPlayer;
			m_player->m_orientation -= portal->m_orientation;
			m_player->m_orientation += portal->GetOtherPortal()->m_orientation;
		}
	}

	m_player->m_position = *m_playerTranslationThisFrame + m_player->m_position;
}

void Game::RenderAllEntities() const
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	 //Render the initial visible meshes of everything.
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	for (int entityIndex = 0; entityIndex < m_entities.size(); ++entityIndex)
	{
		Entity* entity = m_entities[entityIndex];
		if (entity != nullptr)
		{
			entity->Render();
		}
	}

	if (m_isTheWindActive)
	{
		for (int theWindIndex = 0; theWindIndex < m_theWind.size(); ++theWindIndex)
		{
			Prop* currentWind = m_theWind[theWindIndex];
			if (currentWind != nullptr)
			{
				currentWind->Render();
			}
		}
	}
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// Render Debug
	DebugRenderWorld(*m_player->m_worldCamera);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Render the interior wireframe of everything
	//g_engine->m_render->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	//for (int entityIndex = 0; entityIndex < m_entities.size(); ++entityIndex)
	//{
	//	Entity* entity = m_entities[entityIndex];
	//	if (entity != nullptr)
	//	{
	//		entity->Render();
	//	}
	//}

	//if (m_isTheWindActive)
	//{
	//	for (int theWindIndex = 0; theWindIndex < m_theWind.size(); ++theWindIndex)
	//	{
	//		Prop* currentWind = m_theWind[theWindIndex];
	//		if (currentWind != nullptr)
	//		{
	//			currentWind->Render();
	//		}
	//	}
	//}
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
}

void Game::ChangeGameState(GameState newGameState)
{
	m_nextGameState = newGameState;
}

