#include "Game/Player.hpp"

#include "Game/Game.hpp"
#include "Game/Map.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Math/MathUtils.hpp"

Player::Player(Map* owner, Vec3 const& startingPosition)
	: m_map(owner)
	, m_position(startingPosition)
{

}

void Player::Update()
{
	HandleInputs();
	m_worldCamera->SetPositionAndOrientation(m_position, m_orientation);
}

void Player::Render() const
{

}

void Player::Die()
{
	m_isDead = true;
	m_isGarbage = true;
}

void Player::HandleInputs()
{
	if (m_desiredPlayerState != m_playerState)
	{
		m_playerState = m_desiredPlayerState;
	}

	switch (m_playerState)
	{
		case PlayerState::FIRSTPERSON:	HandleInputs_FirstPerson(); break;
		case PlayerState::FREEFLY:		HandleInputs_FreeFly(); break;
	}
}

void Player::HandleInputs_FreeFly()
{
	if (g_engine->m_input->WasKeyJustPressed('F'))
	{
		SetPlayerState(PlayerState::FIRSTPERSON);
	}

	if (m_desiredControlState != m_controlState)
	{
		m_controlState = m_desiredControlState;
	}


	switch (m_controlState)
	{
		case ControlState::KEYBOARD: HandleInputs_FreeFly_Keyboard(); break;
		case ControlState::CONTROLLER: HandleInputs_FreeFly_Controller(); break;
	}
}

void Player::HandleInputs_FreeFly_Keyboard()
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Keyboard
	if (g_engine->m_input->IsKeyDown('T'))
	{
		m_map->m_game->m_gameClock->SetTimeScale(0.1f);
	}
	else
	{
		m_map->m_game->m_gameClock->SetTimeScale(1.f);
	}

	float currentMoveSpeed = m_map->m_game->m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_map->m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_map->m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	}
	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_position;
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
	m_position += newTranslation;

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_map->m_game->m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_map->m_game->m_gameClock->StepSingleFrame();
	}
}

void Player::HandleInputs_FreeFly_Controller()
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Controller
	XboxController* controller = &g_engine->m_input->m_controllers[0];

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	float currentMoveSpeed = m_map->m_game->m_moveSpeed;
	if (controller->IsButtonDown(XboxButtonID::GAMEPAD_A))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_orientation;
	newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * m_map->m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * m_map->m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);

	newOrientation.m_rollDegrees += controller->GetRightTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	newOrientation.m_rollDegrees -= controller->GetLeftTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;

	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_position;
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

	if (controller->IsButtonDown(XboxButtonID::START))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}
}

void Player::HandleInputs_FirstPerson()
{
	if (g_engine->m_input->WasKeyJustPressed('F'))
	{
		SetPlayerState(PlayerState::FREEFLY);
	}

	if (m_desiredControlState != m_controlState)
	{
		m_controlState = m_desiredControlState;
	}

	switch (m_controlState)
	{
		case ControlState::KEYBOARD: HandleInputs_FirstPerson_Keyboard(); break;
		case ControlState::CONTROLLER: HandleInputs_FirstPerson_Controller(); break;
	}
}

void Player::HandleInputs_FirstPerson_Keyboard()
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Keyboard
	float currentMoveSpeed = m_map->m_game->m_moveSpeed;
	if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
	{
		currentMoveSpeed *= 2.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_orientation;
	newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_map->m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_map->m_game->m_mouseSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);
	if (g_engine->m_input->IsKeyDown('Q'))
	{
		newOrientation.m_rollDegrees -= (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	}
	if (g_engine->m_input->IsKeyDown('E'))
	{
		newOrientation.m_rollDegrees += (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	}
	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_position;
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
	m_position += newTranslation;

	if (g_engine->m_input->IsKeyDown('H'))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_map->m_game->m_gameClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_map->m_game->m_gameClock->StepSingleFrame();
	}
}

void Player::HandleInputs_FirstPerson_Controller()
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Controller
	XboxController* controller = &g_engine->m_input->m_controllers[0];

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	float currentMoveSpeed = m_map->m_game->m_moveSpeed;
	if (controller->IsButtonDown(XboxButtonID::GAMEPAD_A))
	{
		currentMoveSpeed *= 15.f;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Camera Orientation
	EulerAngles newOrientation = m_orientation;
	newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * m_map->m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * m_map->m_game->m_controllerSensitivity;
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -85.f, 85.f);

	newOrientation.m_rollDegrees += controller->GetRightTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;
	newOrientation.m_rollDegrees -= controller->GetLeftTrigger() * (float)s_systemClock->GetDeltaSeconds() * m_map->m_game->m_rollSensitivity;

	newOrientation.m_rollDegrees = GetClamped(newOrientation.m_rollDegrees, -45.f, 45.f);
	m_orientation = newOrientation;

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Movement
	Vec3 currentPosition = m_position;
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

	if (controller->IsButtonDown(XboxButtonID::START))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

Mat44 Player::GetWorldToModelTransform() const
{
	Mat44 ModelToWorld = GetModelToWorldTransform();

	return ModelToWorld.GetOrthonormalInverse();
}

bool Player::IsAlive() const
{
	return !m_isDead;
}

void Player::SetPlayerState(PlayerState const& state)
{
	m_desiredPlayerState = state;
}

void Player::SetControllerState(ControlState const& state)
{
	m_desiredControlState = state;
}
