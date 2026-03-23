#pragma once
#include "Game/GameCommon.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Renderer/Camera.hpp"

#include <vector>

class RandomNumberGenerator;
class Entity;
class Clock;
class Player;
class Prop;
class Timer;
class Shader;
class Portal;
typedef size_t SoundID;

enum GameState {
	GAME_STATE_NONE,
	GAME_STATE_ATTRACT,
	GAME_STATE_MAIN
};

class Game
{
public:
	Game();
	~Game();
	void Update();
	void Render() const;
	void Startup();

	void AddScreenShake(float screenShake);

	void AddHitStop(float hitStop);

	void RenderAllPortals() const;
	void RenderAllEntities() const;

private:
	void Startup_PopulateFromBlackboard();

	void Update_AttractMode();
	void Update_MainMode();

	void Render_AttractMode() const;
	void Render_MainMode() const;

	void Update_MainMode_Entities();
	bool Update_MainMode_KeyboardInput();
	void Update_MainMode_KeyboardInput_DebugRender();
	bool Update_MainMode_ControllerInput();
	void Update_MainMode_PlayerCCD();

	void ChangeGameState(GameState newGameState);
	void DestroyGarbageEntities();

public:
	RandomNumberGenerator*	m_randomNumberGenerator = nullptr;
	GameState				m_nextGameState = GameState::GAME_STATE_ATTRACT;
	GameState				m_currentGameState = GameState::GAME_STATE_ATTRACT;
	float					m_screenShakeFraction = 0.f;
	
	Shader* m_useTexture1Shader;
	Shader* m_drawToTexShader;

	float m_hitStopTimer = 0.f;
	bool m_isHitStop = false;

	Player* m_player;
	Vec3* m_playerTranslationThisFrame;

	std::vector<Entity*> m_entities;
	std::vector<Portal*> m_portals;
	Prop* m_cubeProp;
	Prop* m_cubeProp2;
	Prop* m_sphereProp;
	Prop* m_cylinderProp;
	Prop* m_blaineProp;
	Prop* m_venipedeProp;
	Prop* m_venipedeEyesProp;
	Prop* m_thresherProp;
	Prop* m_portal1CameraCube;
	Prop* m_portal2CameraCube;
	Prop* m_dragonProp;
	std::vector<Prop*> m_theWind;
	std::vector<Prop*> m_gridXLines;
	std::vector<Prop*> m_gridYLines;

	Rgba8 m_backgroundClearColor = Rgba8(50, 50, 50, 255);

	Timer* m_theWindTimer;
	Timer* m_theWindGoAwayTimer;
	bool m_isTheWindActive = false;

	Clock* m_gameClock;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Blackboard values
	float m_perspectiveFOV = 0.f;
	float m_rollSensitivity = 0.f;
	float m_mouseSensitivity = 0.f;
	float m_controllerSensitivity = 0.f;
	float m_moveSpeed = 0.f;
	float m_colorUndulateTime = 0.f;
};