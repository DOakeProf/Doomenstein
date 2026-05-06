#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"

#include "Game/Controller.hpp"

class Map;
struct Camera;
struct Vec3;
struct Mat44;
struct EulerAngles;
struct Rgba8;

enum class PlayerState
{
	NONE,
	FIRSTPERSON,
	FREEFLY,
	COUNT
};

enum class ControlState
{
	NONE,
	KEYBOARD,
	CONTROLLER,
	COUNT
};

class Player : public Controller
{
public:
	Player(Map* owner, Vec3 const& startingPosition);
	virtual ~Player() = default;

	void Update() override;
	void Render();

	void Render_HUD();
	void Render_Death();

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Inputs
	void HandleInputs();
	void HandleInputs_FreeFly();
	void HandleInputs_FreeFly_Keyboard();
	void HandleInputs_FreeFly_Controller();
	void HandleInputs_FirstPerson();
	void HandleInputs_FirstPerson_Keyboard();
	void HandleInputs_FirstPerson_Controller();
	void HandleInputs_Debug();

	virtual Mat44 GetModelToWorldTransform() const;
	virtual Mat44 GetWorldToModelTransform() const;

	void SetPlayerState(PlayerState const& state);
	void SetControllerState(ControlState const& state);
	void SetViewport(bool isMultiplayer, int playerIndex);

	bool IsPlayer() const override;

	int m_playerIndex = -1;
	int m_controllerIndex = -1;
	int m_playerKills;
	int m_playerDeaths;
	Camera* m_worldCamera = nullptr;
	Camera* m_screenCamera = nullptr;
	AABB2	m_viewport;

	Vec3 m_position; // Not initialized like the others because by simply just defining a new object of these classes, they are already initialized in their constructors.
	Vec3 m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_orientationRecoil;
	EulerAngles m_angularVelocity;
	EulerAngles m_recoil;
	Rgba8 m_color = Rgba8::WHITE;
	PlayerState m_playerState = PlayerState::FREEFLY;
	PlayerState m_desiredPlayerState = PlayerState::FREEFLY;
	ControlState m_controlState = ControlState::KEYBOARD;
	ControlState m_desiredControlState = ControlState::KEYBOARD;

	float m_jumpBuffer = 1.f;
	bool m_sprintToggle = false;
};