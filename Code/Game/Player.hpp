#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"

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

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Inputs
	void HandleInputs();
	void HandleInputs_FreeFly();
	void HandleInputs_FreeFly_Keyboard();
	void HandleInputs_FreeFly_Controller();
	void HandleInputs_FirstPerson();
	void HandleInputs_FirstPerson_Keyboard();
	void HandleInputs_FirstPerson_Controller();

	virtual Mat44 GetModelToWorldTransform() const;
	virtual Mat44 GetWorldToModelTransform() const;

	void SetPlayerState(PlayerState const& state);
	void SetControllerState(ControlState const& state);

	bool IsPlayer() const override;

	Camera* m_worldCamera = nullptr;

	Vec3 m_position; // Not initialized like the others because by simply just defining a new object of these classes, they are already initialized in their constructors.
	Vec3 m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	Rgba8 m_color = Rgba8::WHITE;
	PlayerState m_playerState = PlayerState::FREEFLY;
	PlayerState m_desiredPlayerState = PlayerState::FREEFLY;
	ControlState m_controlState = ControlState::KEYBOARD;
	ControlState m_desiredControlState = ControlState::KEYBOARD;

	float m_jumpBuffer = 1.f;
	bool m_sprintToggle = false;
};