#include "Game/Player.hpp"

#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/BitmapFont.hpp"
#include "Engine/VertexUtils.hpp"

Player::Player(Map* owner, Vec3 const& startingPosition)
	: Controller(owner)
	, m_position(startingPosition)
{

}

void Player::Update()
{
	HandleInputs();

	Vec3 cursorBasisPosition = m_position + m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 0.2f;
	Mat44 cursorBasisMatrix = Mat44();
	cursorBasisMatrix.SetTranslation3D(cursorBasisPosition);
	DebugAddBasis(cursorBasisMatrix, 0.f, 0.01f, 0.001f);

	if (m_actorHandle == nullptr)
	{
		return;
	}
	Actor* actor = m_map->GetActorByHandle(*m_actorHandle);
	if (m_jumpBuffer < 0.05f)
	{
		actor->Jump(13.5f);
	}
	m_jumpBuffer += (float)m_map->m_game->m_gameClock->GetDeltaSeconds();

	if (actor != nullptr && actor->m_isDead)
	{
		float cameraFallTime = (float)actor->m_deathTimer->GetElapsedFraction();
		Vec3 cameraFallDisplacement = Vec3(0.f,0.f,-actor->m_definition->m_eyeHeight);
		cameraFallDisplacement *= GetClamped(cameraFallTime, 0.f, 0.5f) * 1.5f;
		m_worldCamera->SetPositionAndOrientation(m_position + cameraFallDisplacement, m_orientation);
	}
	else
	{
		m_worldCamera->SetPositionAndOrientation(m_position, m_orientation);
	}

	if (m_orientation.m_rollDegrees != 0.f)
	{
		float maxTurnThisFrame = m_map->m_game->m_rollSensitivity * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
		m_orientation.m_rollDegrees += GetClamped(GetShortestAngularDispDegrees(m_orientation.m_rollDegrees, 0.f), -maxTurnThisFrame, maxTurnThisFrame);
	}

	if (actor == nullptr)
	{
		m_map->SpawnPlayer(this);
	}
	else
	{
		actor->m_equippedWeapon->Update();
	}
}

void Player::Render()
{
	// HUD
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	GetActor()->m_equippedWeapon->Render();
	Render_HUD_Health();
	Render_Death();
}

void Player::Render_HUD_Health()
{
	Actor* playerActor = GetActor();
	if (playerActor != nullptr)
	{
		std::vector<Vertex> uiHealthVerts;
		std::string uiHealthText;
		if ((float)playerActor->m_health > 0.f)
		{
			uiHealthText = Stringf("HEALTH: %.2f", (float)playerActor->m_health);
		}
		else
		{
			uiHealthText = Stringf("DEAD :(");
		}
		AABB2 SCREEN_AABB2 = AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiHealthVerts, uiHealthText, SCREEN_AABB2, SCREEN_SIZE_Y * 0.03f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.2f));
		g_engine->m_render->BindTexture(&m_map->m_game->m_squirrelFont->GetTexture());
		g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_engine->m_render->DrawVertexList(&uiHealthVerts);
	}
}

void Player::Render_Death()
{
	Actor* playerActor = GetActor();
	if (playerActor != nullptr && playerActor->m_isDead)
	{
		std::vector<Vertex> deathVerts;
		AABB2 SCREEN_AABB2 = AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		AddVertsForAABB2D(deathVerts, SCREEN_AABB2, Rgba8(50,50,50,127));
		g_engine->m_render->BindTexture(nullptr);
		g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_engine->m_render->DrawVertexList(&deathVerts);
	}
}

void Player::HandleInputs()
{
	if (m_desiredPlayerState != m_playerState)
	{
		m_playerState = m_desiredPlayerState;
	}

	Actor* actor = GetActor();

	if (actor != nullptr && GetActor()->m_isDead)
	{
		return;
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
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
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
	newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
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

	if (controller->WasButtonJustPressed(XboxButtonID::START))
	{
		m_map->m_game->m_gameClock->TogglePause();
	}
}

void Player::HandleInputs_FirstPerson()
{
	//&& m_map->m_game->m_players.size() < 2
	if (g_engine->m_input->WasKeyJustPressed('F') || m_actorHandle == nullptr || m_map->GetActorByHandle(*m_actorHandle) == nullptr)
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

	// Just do both for now
	//HandleInputs_FirstPerson_Keyboard();
	//HandleInputs_FirstPerson_Controller();
}

void Player::HandleInputs_FirstPerson_Keyboard()
{
	if (m_actorHandle != nullptr && m_map->GetActorByHandle(*m_actorHandle) != nullptr)
	{
		Actor* actor = m_map->GetActorByHandle(*m_actorHandle);

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Camera Orientation
		EulerAngles newOrientation = m_orientation;
		newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_map->m_game->m_mouseSensitivity;
		newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_map->m_game->m_mouseSensitivity;
		newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
		m_orientation = newOrientation;

		actor->m_orientation = m_orientation;

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Movement
		float currentMoveSpeed = actor->m_definition->m_walkSpeed;
		if (g_engine->m_input->IsKeyDown(KEYCODE_SHIFT))
		{
			currentMoveSpeed = actor->m_definition->m_runSpeed;
		}

		Vec3 currentPosition = actor->m_position;
		Vec3 newAddedPosition;
		if (g_engine->m_input->IsKeyDown('A'))
		{
			newAddedPosition.y += 1.f;
		}
		if (g_engine->m_input->IsKeyDown('D'))
		{
			newAddedPosition.y -= 1.f;
		}
		if (g_engine->m_input->IsKeyDown('W'))
		{
			newAddedPosition.x += 1.f;
		}
		if (g_engine->m_input->IsKeyDown('S'))
		{
			newAddedPosition.x -= 1.f;
		}
		if (newAddedPosition != Vec3())
		{
			Mat44 orientationMatrix = newOrientation.GetAsMatrix_IFwd_JLeft_KUp();
			orientationMatrix.AppendTranslation3D(newAddedPosition);
			Vec3 directionToMoveIn = orientationMatrix.GetTranslation3D();
			directionToMoveIn.z = 0;
			directionToMoveIn = directionToMoveIn.GetNormalized();
			actor->MoveInDirection(directionToMoveIn, currentMoveSpeed);
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Gameplay
		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_SPACE))
		{
			m_jumpBuffer = 0.f;
		}
		if (g_engine->m_input->WasKeyJustReleased(KEYCODE_SPACE) && actor->m_isJumping && actor->m_velocity.z > 0.f)
		{
			actor->CancelJump();
		}
		if (g_engine->m_input->IsKeyDown(KEYCODE_LEFT_MOUSE))
		{
			actor->Attack();
		}
		if (g_engine->m_input->IsKeyDown(KEYCODE_RIGHT_MOUSE))
		{
			actor->SecondaryAttack();
		}
		if (g_engine->m_input->WasKeyJustPressed('1') && actor->m_weapons.size() > 0)
		{
			actor->m_equippedWeapon = actor->m_weapons[0];
		}
		if (g_engine->m_input->WasKeyJustPressed('2') && actor->m_weapons.size() > 1)
		{
			actor->m_equippedWeapon = actor->m_weapons[1];
		}
		if (g_engine->m_input->WasKeyJustPressed('3') && actor->m_weapons.size() > 2)
		{
			actor->m_equippedWeapon = actor->m_weapons[2];
		}
		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_LEFTARROW))
		{
			int weaponIndex = actor->GetEquippedWeaponIndex();
			int newWeaponIndex = --weaponIndex;
			if (newWeaponIndex < 0)
			{
				actor->m_equippedWeapon = actor->m_weapons[actor->m_weapons.size() - 1];
			}
			else
			{
				actor->m_equippedWeapon = actor->m_weapons[newWeaponIndex];
			}
		}
		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_RIGHTARROW))
		{
			int weaponIndex = actor->GetEquippedWeaponIndex();
			int newWeaponIndex = ++weaponIndex;
			if (newWeaponIndex >= actor->m_weapons.size())
			{
				actor->m_equippedWeapon = actor->m_weapons[0];
			}
			else
			{
				actor->m_equippedWeapon = actor->m_weapons[newWeaponIndex];
			}
		}

		m_position = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);
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

	if (m_actorHandle != nullptr && m_map->GetActorByHandle(*m_actorHandle) != nullptr)
	{
		Actor* actor = m_map->GetActorByHandle(*m_actorHandle);
		float deltaSeconds = (float)m_map->m_game->m_gameClock->GetDeltaSeconds();

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Camera Orientation
		EulerAngles newOrientation = m_orientation;
		newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * actor->m_definition->m_turnSpeed * deltaSeconds;
		newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * actor->m_definition->m_turnSpeed * deltaSeconds;m_map->m_game->m_controllerSensitivity;
		newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
		m_orientation = newOrientation;

		actor->m_orientation = m_orientation;

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Movement
		Vec3 currentPosition = actor->m_position;
		Vec2 leftStickPos = controller->GetLeftStick().GetPosition();
		Vec3 newAddedPosition = Vec3(leftStickPos.x, leftStickPos.y, 0.f);

		float currentMoveSpeed = actor->m_definition->m_walkSpeed;
		if (controller->WasButtonJustPressed(XboxButtonID::LEFT_THUMB))
		{
			m_sprintToggle = !m_sprintToggle;
		}
		if (leftStickPos == Vec2())
		{
			m_sprintToggle = false;
		}
		if (m_sprintToggle)
		{
			currentMoveSpeed = actor->m_definition->m_runSpeed;
		}

		if (newAddedPosition != Vec3())
		{
			newAddedPosition = newAddedPosition.GetRotatedAboutZDegrees(m_orientation.m_yawDegrees - 90.f); // Minus 90 to get pos Y as our forward instead of X.
			actor->MoveInDirection(newAddedPosition, currentMoveSpeed);
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Gameplay
		if (controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_A))
		{
			m_jumpBuffer = 0.f;
		}
		if (controller->WasButtonJustReleased(XboxButtonID::GAMEPAD_A) && actor->m_isJumping && actor->m_velocity.z > 0.f)
		{
			actor->CancelJump();
		}
		if (controller->GetRightTrigger() != 0.f)
		{
			actor->Attack();
		}
		if (controller->GetLeftTrigger() != 0.f)
		{
			actor->SecondaryAttack();
		}
		if (controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_X) && actor->m_weapons.size() > 0)
		{
			actor->m_equippedWeapon = actor->m_weapons[0];
		}
		if (controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_Y) && actor->m_weapons.size() > 1)
		{
			actor->m_equippedWeapon = actor->m_weapons[1];
		}
		if (controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_B) && actor->m_weapons.size() > 2)
		{
			actor->m_equippedWeapon = actor->m_weapons[2];
		}
		if (controller->WasButtonJustPressed(XboxButtonID::DPAD_LEFT) || controller->WasButtonJustPressed(XboxButtonID::DPAD_DOWN))
		{
			int weaponIndex = actor->GetEquippedWeaponIndex();
			int newWeaponIndex = --weaponIndex;
			if (newWeaponIndex < 0)
			{
				actor->m_equippedWeapon = actor->m_weapons[actor->m_weapons.size() - 1];
			}
			else
			{
				actor->m_equippedWeapon = actor->m_weapons[newWeaponIndex];
			}
		}
		if (controller->WasButtonJustPressed(XboxButtonID::DPAD_RIGHT) || controller->WasButtonJustPressed(XboxButtonID::DPAD_UP))
		{
			int weaponIndex = actor->GetEquippedWeaponIndex();
			int newWeaponIndex = ++weaponIndex;
			if (newWeaponIndex >= actor->m_weapons.size())
			{
				actor->m_equippedWeapon = actor->m_weapons[0];
			}
			else
			{
				actor->m_equippedWeapon = actor->m_weapons[newWeaponIndex];
			}
		}

		m_position = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);
	}

	if (controller->WasButtonJustPressed(XboxButtonID::START))
	{
		m_map->m_game->m_gameClock->TogglePause();
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

void Player::SetPlayerState(PlayerState const& state)
{
	m_desiredPlayerState = state;
}

void Player::SetControllerState(ControlState const& state)
{
	m_desiredControlState = state;
}

void Player::SetViewport(bool isMultiplayer, int playerIndex)
{
	if (isMultiplayer)
	{
		float halfScreenHeight = (float)g_engine->m_window->GetClientDimensions().y * 0.5f; // TODO: Move the viewport logic elsewhere.
		AABB2 playerViewport = AABB2(0.f, (halfScreenHeight * playerIndex), (float)g_engine->m_window->GetClientDimensions().x, halfScreenHeight + (halfScreenHeight * playerIndex));
		m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
		m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);
		m_worldCamera->SetViewport(playerViewport);
		m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_screenCamera->SetViewport(playerViewport);
		m_viewport = playerViewport;
	}
	else
	{
		AABB2 playerViewport = AABB2(0.f, 0.f, (float)g_engine->m_window->GetClientDimensions().x, (float)g_engine->m_window->GetClientDimensions().y);
		m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
		m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);
		m_worldCamera->SetViewport(playerViewport);
		m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_screenCamera->SetViewport(playerViewport);
		m_viewport = playerViewport;
	}
}

bool Player::IsPlayer() const
{
	return true;
}
