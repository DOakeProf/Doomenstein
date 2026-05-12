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
	if (m_map->m_isShowingDevourerSpawn)
	{
		return;
	}

	if (m_position.GetLength() > 1000.f && GetActor() != nullptr)
	{
		GetActor()->m_desiredPosition = Vec3(45.5f, 41.5f, 20.f);
	}

	m_hasPlacedOrbThisFrame = false;
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
		actor->Jump();
	}
	m_jumpBuffer += (float)m_map->m_game->m_gameClock->GetDeltaSeconds();

	// Recoil
	float interpolateValue = 1.f * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
	m_recoil.m_yawDegrees = Interpolate(m_recoil.m_yawDegrees, 0.f, interpolateValue);
	m_recoil.m_pitchDegrees = Interpolate(m_recoil.m_pitchDegrees, 0.f, interpolateValue);
	m_recoil.m_rollDegrees = Interpolate(m_recoil.m_rollDegrees, 0.f, interpolateValue);
	m_orientationRecoil = m_orientation + m_recoil;

	if (actor != nullptr)
	{
		// Scope
		float scopeFraction = actor->m_equippedWeapon->m_scopeFraction;
		if (scopeFraction > 0.f)
		{
			m_worldCamera->SetPerspectiveFOV(Interpolate(60.f, actor->m_equippedWeapon->m_definition->m_scopedFOV, SmoothStep3(scopeFraction)));
		}
		else
		{
			m_worldCamera->SetPerspectiveFOV(60.f);
		}

		// Check if near interactable actors
		m_ballNextTo = nullptr;
		m_orbNextTo = nullptr;
		m_spawnPadNextTo = nullptr;
		for (Actor* actorToCheck : m_map->GetActors())
		{
			if (actorToCheck != nullptr && actorToCheck->m_definition->m_name == "Ball")
			{
				Vec3 ballToPlayer = m_position - actorToCheck->m_position;
				if (ballToPlayer.GetLength() < 2.f)
				{
					m_ballNextTo = actorToCheck;
				}
			}
			if (actorToCheck != nullptr && actorToCheck->m_definition->m_name == "OrbPickup")
			{
				Vec3 orbToPlayer = m_position - actorToCheck->m_position;
				if (orbToPlayer.GetLength() < 2.f)
				{
					m_orbNextTo = actorToCheck;
				}
			}
			if (actorToCheck != nullptr && actorToCheck->m_definition->m_name == "SpawnPlatform")
			{
				Vec3 platformToPlayer = m_position - actorToCheck->m_position;
				if (platformToPlayer.GetLength() < 3.5f)
				{
					m_spawnPadNextTo = actorToCheck;
				}
			}
		}

		// Check for if we dropped an orb
		bool isHoldingOrb = (GetActor()->m_equippedWeapon->m_definition->m_name == "OrbPickup");
		if (!isHoldingOrb && m_isHoldingOrb && !m_hasPlacedOrbThisFrame) // We just lost it
		{
			m_map->SpawnActor("OrbPickup", GetActor()->m_position, EulerAngles(), 1.f);
		}
		m_isHoldingOrb = isHoldingOrb;

		if (m_ballInsideOf != nullptr && m_ballInsideOf->m_isDead)
		{
			Vec3 centerOfMap = Vec3(45.5f, 41.5f, 20.f);
			Vec3 actorToCenter = centerOfMap - GetActor()->m_desiredPosition;
			Vec3 newImpulseToCenter = actorToCenter;
			GetActor()->AddImpulse(newImpulseToCenter);
			m_ballInsideOf = nullptr;
		}

		// If in a ball, override all position adjustments from input and move the player to wherever the ball is. This is done again if the DOG has the ball in its mouth but later after the ball has changed desired position.
		if (m_ballInsideOf != nullptr)
		{
			GetActor()->m_desiredPosition = m_ballInsideOf->m_desiredPosition + Vec3(0.f, 0.f, 0.2f);
		}

		if (actor != nullptr && actor->m_isDead)
		{
			float cameraFallTime = (float)actor->m_deathTimer->GetElapsedFraction();
			Vec3 cameraFallDisplacement = Vec3(0.f, 0.f, -actor->m_definition->m_eyeHeight);
			cameraFallDisplacement *= GetClamped(cameraFallTime, 0.f, 0.5f) * 1.5f;
			m_worldCamera->SetPositionAndOrientation(m_position + cameraFallDisplacement, m_orientation);
		}
		else
		{
			m_worldCamera->SetPositionAndOrientation(m_position, m_orientation + m_recoil);
		}

		if (m_orientation.m_rollDegrees != 0.f)
		{
			float maxTurnThisFrame = m_map->m_game->m_rollSensitivity * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
			m_orientation.m_rollDegrees += GetClamped(GetShortestAngularDispDegrees(m_orientation.m_rollDegrees, 0.f), -maxTurnThisFrame, maxTurnThisFrame);
		}
	}

	if (actor == nullptr)
	{
		m_map->SpawnPlayer(this);
	}
	else
	{
		actor->m_equippedWeapon->Update(actor);
	}

	g_engine->m_audio->UpdateListener(m_playerIndex, m_position, m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_orientation.GetUpDir_IFwd_JLeft_KUp());
}

void Player::Render()
{
	// HUD
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	GetActor()->m_equippedWeapon->Render();
	if (!GetActor()->m_isDead)
	{
		GetActor()->m_equippedWeapon->Render_Weapon();
	}
	Render_HUD();
	Render_Death();
}

void Player::Render_HUD()
{
	Actor* playerActor = GetActor();
	if (playerActor != nullptr)
	{
		std::vector<Vertex> uiVerts;
		std::string uiHealthText;
		if ((float)playerActor->m_health > 0.f)
		{
			uiHealthText = Stringf("%i", playerActor->m_health);
		}
		else
		{
			uiHealthText = Stringf("0");
		}
		AABB2 SCREEN_AABB2 = AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, uiHealthText, SCREEN_AABB2, SCREEN_SIZE_Y * 0.06f, Rgba8::WHITE, 1.f, Vec2(0.285f, 0.06f));
		m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, Stringf("%i", m_playerKills), SCREEN_AABB2, SCREEN_SIZE_Y * 0.06f, Rgba8::WHITE, 1.f, Vec2(0.05f, 0.06f));
		m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, Stringf("%i", m_playerDeaths), SCREEN_AABB2, SCREEN_SIZE_Y * 0.06f, Rgba8::WHITE, 1.f, Vec2(0.95f, 0.06f));
		if (GetActor()->m_equippedWeapon->m_definition->m_maxAmmo != -1)
		{
			m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, Stringf("%i/%i", GetActor()->m_equippedWeapon->m_bullets, GetActor()->m_equippedWeapon->m_definition->m_maxAmmo), SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.15f, 0.06f));
		}
		if (m_ballNextTo != nullptr && m_ballInsideOf == nullptr)
		{
			if (m_controlState == ControlState::KEYBOARD)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press E to enter Ball", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
			else if (m_controlState == ControlState::CONTROLLER)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press Y to enter Ball", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
		}
		else if (m_orbNextTo != nullptr && GetActor()->m_equippedWeapon->m_definition->m_name != "OrbPickup")
		{
			if (m_controlState == ControlState::KEYBOARD)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press E to pick up Orb", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
			else if (m_controlState == ControlState::CONTROLLER)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press Y to pick up Orb", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
		}
		else if (m_spawnPadNextTo != nullptr && GetActor()->m_equippedWeapon->m_definition->m_name != "OrbPickup" && m_ballInsideOf != nullptr)
		{
			m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Missing Orb", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8(200, 200, 200), 1.f, Vec2(0.5f, 0.4f));
		}
		else if (m_spawnPadNextTo != nullptr && GetActor()->m_equippedWeapon->m_definition->m_name == "OrbPickup")
		{
			if (m_controlState == ControlState::KEYBOARD)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press E to deposit Orb", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
			else if (m_controlState == ControlState::CONTROLLER)
			{
				m_map->m_game->m_squirrelFont->AddVertsForTextInBox2D(uiVerts, "Press Y to deposit Orb", SCREEN_AABB2, SCREEN_SIZE_Y * 0.04f, Rgba8::WHITE, 1.f, Vec2(0.5f, 0.4f));
			}
		}
		g_engine->m_render->BindTexture(&m_map->m_game->m_squirrelFont->GetTexture());
		g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_engine->m_render->DrawVertexList(&uiVerts);
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

	HandleInputs_Debug();

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
		float scopeFraction = 1.f;
		if (GetActor() != nullptr && GetActor()->m_equippedWeapon->m_isScoped)
		{
			scopeFraction = GetActor()->m_equippedWeapon->m_definition->m_scopedFOV / GetActor()->m_definition->m_cameraFOV;
		}
		newOrientation.m_yawDegrees += g_engine->m_input->GetCursorClientDelta().x * m_map->m_game->m_mouseSensitivity * scopeFraction;
		newOrientation.m_pitchDegrees -= g_engine->m_input->GetCursorClientDelta().y * m_map->m_game->m_mouseSensitivity * scopeFraction;
		newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
		m_orientation = newOrientation;

		HandleAACapture();
		actor->m_orientation = m_orientationRecoil;

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
		else
		{
			actor->m_equippedWeapon->StopScope();
		}
		if (g_engine->m_input->WasKeyJustPressed('1') && actor->m_weapons.size() > 0)
		{
			actor->m_equippedWeapon->StopReload();
			actor->m_equippedWeapon = actor->m_weapons[0];
		}
		if (g_engine->m_input->WasKeyJustPressed('2') && actor->m_weapons.size() > 1)
		{
			actor->m_equippedWeapon->StopReload();
			actor->m_equippedWeapon = actor->m_weapons[1];
		}
		if (g_engine->m_input->WasKeyJustPressed('3') && actor->m_weapons.size() > 2)
		{
			actor->m_equippedWeapon->StopReload();
			actor->m_equippedWeapon = actor->m_weapons[2];
		}
		if (g_engine->m_input->WasKeyJustPressed('4') && actor->m_weapons.size() > 3)
		{
			actor->m_equippedWeapon->StopReload();
			actor->m_equippedWeapon = actor->m_weapons[3];
		}
		if (g_engine->m_input->WasKeyJustPressed('5') && actor->m_weapons.size() > 4)
		{
			actor->m_equippedWeapon->StopReload();
			actor->m_equippedWeapon = actor->m_weapons[4];
		}
		if (g_engine->m_input->WasKeyJustPressed(KEYCODE_LEFTARROW))
		{
			actor->m_equippedWeapon->StopReload();
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
			actor->m_equippedWeapon->StopReload();
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
		if (g_engine->m_input->WasKeyJustPressed('R'))
		{
			actor->m_equippedWeapon->StartReload(actor);
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Interacting with E
		if (g_engine->m_input->WasKeyJustPressed('E'))
		{
			if (m_ballInsideOf != nullptr) // If in ball, exit it
			{
				GetActor()->m_position = m_ballInsideOf->m_position + Vec3(1.f, 0.f, 0.f);
				m_ballInsideOf = nullptr;
			}
			else if (m_ballNextTo != nullptr) // If not in ball but near one, enter it
			{
				m_ballInsideOf = m_ballNextTo;
			}
			else if (m_orbNextTo != nullptr && GetActor()->m_equippedWeapon->m_definition->m_name != "OrbPickup") // If next to orb, pick it up
			{
				GetActor()->m_equippedWeapon = new Weapon(m_map, "OrbPickup");
				m_orbNextTo->Die();
			}
			else if (GetActor()->m_equippedWeapon->m_definition->m_name == "OrbPickup" && m_spawnPadNextTo != nullptr) // If next to a spawn pad and holding orb, put the orb in it and spawn a ball
			{
				GetActor()->m_equippedWeapon = GetActor()->m_weapons[0];
				m_hasPlacedOrbThisFrame = true;
				m_map->SpawnActor("Ball", m_spawnPadNextTo->m_position, EulerAngles(), 1.f);
			}
			else if (GetActor()->m_equippedWeapon->m_definition->m_name == "OrbPickup") // If holding an orb, drop it
			{
				GetActor()->m_equippedWeapon = GetActor()->m_weapons[0];
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
		float scopeFraction = 1.f;
		if (GetActor() != nullptr && GetActor()->m_equippedWeapon->m_isScoped)
		{
			scopeFraction = GetActor()->m_equippedWeapon->m_definition->m_scopedFOV / GetActor()->m_definition->m_cameraFOV;
		}
		newOrientation.m_yawDegrees -= controller->GetRightStick().GetPosition().x * actor->m_definition->m_turnSpeed * deltaSeconds * scopeFraction;
		newOrientation.m_pitchDegrees -= controller->GetRightStick().GetPosition().y * actor->m_definition->m_turnSpeed * deltaSeconds * scopeFraction;
		newOrientation.m_pitchDegrees = GetClamped(newOrientation.m_pitchDegrees, -89.f, 89.f);
		m_orientation = newOrientation;

		HandleAACapture();
		actor->m_orientation = m_orientationRecoil;

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
		else
		{
			actor->m_equippedWeapon->StopScope();
		}
		if (controller->WasButtonJustPressed(XboxButtonID::DPAD_LEFT) || controller->WasButtonJustPressed(XboxButtonID::DPAD_DOWN) || controller->WasButtonJustPressed(XboxButtonID::LEFT_SHOULDER))
		{
			actor->m_equippedWeapon->StopReload();
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
		if (controller->WasButtonJustPressed(XboxButtonID::DPAD_RIGHT) || controller->WasButtonJustPressed(XboxButtonID::DPAD_UP) || controller->WasButtonJustPressed(XboxButtonID::RIGHT_SHOULDER))
		{
			actor->m_equippedWeapon->StopReload();
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
		if (controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_X))
		{
			actor->m_equippedWeapon->StartReload(actor);
		}

		if (m_ballInsideOf != nullptr && controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_Y))
		{
			GetActor()->m_position = m_ballInsideOf->m_position + Vec3(1.f, 0.f, 0.f);
			m_ballInsideOf = nullptr;
		}
		else if (m_ballNextTo != nullptr && controller->WasButtonJustPressed(XboxButtonID::GAMEPAD_Y))
		{
			m_ballInsideOf = m_ballNextTo;
		}

		m_position = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);
	}

	if (controller->WasButtonJustPressed(XboxButtonID::START))
	{
		m_map->m_game->m_gameClock->TogglePause();
	}
}

void Player::HandleInputs_Debug()
{
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F2))
	{
		m_map->m_sunDirection.x -= 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_map->m_sunDirection.x, m_map->m_sunDirection.y, m_map->m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F3))
	{
		m_map->m_sunDirection.x += 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_map->m_sunDirection.x, m_map->m_sunDirection.y, m_map->m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F4))
	{
		m_map->m_sunDirection.y -= 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_map->m_sunDirection.x, m_map->m_sunDirection.y, m_map->m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F5))
	{
		m_map->m_sunDirection.y += 1;
		std::string message = Stringf("Sun Direction: %.2f, %.2f, %.2f", m_map->m_sunDirection.x, m_map->m_sunDirection.y, m_map->m_sunDirection.z);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F6))
	{
		m_map->m_sunIntensity -= 0.05f;
		m_map->m_sunIntensity = GetClamped(m_map->m_sunIntensity, 0.f, 1.f);
		std::string message = Stringf("Sun Intensity: %.2f", m_map->m_sunIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F7))
	{
		m_map->m_sunIntensity += 0.05f;
		m_map->m_sunIntensity = GetClamped(m_map->m_sunIntensity, 0.f, 1.f);
		std::string message = Stringf("Sun Intensity: %.2f", m_map->m_sunIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F8))
	{
		m_map->m_ambientIntensity -= 0.05f;
		m_map->m_ambientIntensity = GetClamped(m_map->m_ambientIntensity, 0.f, 1.f);
		std::string message = Stringf("Ambient Intensity: %.2f", m_map->m_ambientIntensity);
		DebugAddMessage(message, 2.f);
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_F9))
	{
		m_map->m_ambientIntensity += 0.05f;
		m_map->m_ambientIntensity = GetClamped(m_map->m_ambientIntensity, 0.f, 1.f);
		std::string message = Stringf("Ambient Intensity: %.2f", m_map->m_ambientIntensity);
		DebugAddMessage(message, 2.f);
	}

	//if (g_engine->m_input->WasKeyJustPressed('R'))
	//{
	//	m_map->m_game->RefreshRifts();
	//}

	if (g_engine->m_input->WasKeyJustPressed('N') && m_map->m_game->m_players.size() < 2)
	{
		Actor* newActorToPossess = nullptr;
		int startingActorIndex = m_map->m_game->m_players[0]->m_actorHandle->GetIndex();
		for (int actorIndex = startingActorIndex + 1; actorIndex < m_map->m_actors.size(); ++actorIndex)
		{
			Actor* actor = m_map->m_actors[actorIndex];
			if (actor != nullptr && actor->m_definition->m_canBePossessed)
			{
				newActorToPossess = actor;
				break;
			}
		}
		if (newActorToPossess == nullptr) // If we reached the end of the list of actors, cycle back to the beginning.
		{
			for (int actorIndex = 0; actorIndex < m_map->m_actors.size(); ++actorIndex)
			{
				Actor* actor = m_map->m_actors[actorIndex];
				if (actor != nullptr && actor->m_definition->m_canBePossessed)
				{
					newActorToPossess = actor;
					break;
				}
			}
		}
		if (newActorToPossess != nullptr)
		{
			m_map->m_game->m_players[0]->Depossess();
			m_map->m_game->m_players[0]->Possess(newActorToPossess->m_handle);
		}
	}

	//if (g_engine->m_input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	//{
	//	RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 10.f, nullptr);
	//}
	//if (g_engine->m_input->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	//{
	//	RaycastAll(m_player->m_position, m_player->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), 0.25f, nullptr);
	//}
}

void Player::HandleAACorrection()
{
	if (m_playerState != PlayerState::FIRSTPERSON) // We don't want aim assist to take over if not in first person.
	{
		return;
	}

	Actor* actor = GetActor();
	if (actor != nullptr && m_isAimAssistActive)
	{
		Vec3 selfToAAPointNormalized = (m_aimAssistCapturedPos - m_position).GetNormalized();
		Mat44 newOrientationMatrix = Mat44(selfToAAPointNormalized, Vec3(1.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f), Vec3());
		newOrientationMatrix.Orthonormalize_XFwd_YLeft_ZUp();
		EulerAngles newOrientation = EulerAngles(newOrientationMatrix);
		newOrientation.m_rollDegrees = 0.f;
		if (m_orientation.m_yawDegrees != newOrientation.m_yawDegrees)
		{
			float AAFraction = 0.f;
			switch (m_controlState)
			{
				case ControlState::CONTROLLER: AAFraction = m_map->m_game->m_controllerAA; break;
				case ControlState::KEYBOARD: AAFraction = m_map->m_game->m_mouseAA; break;
			}
			m_orientation = Interpolate(m_orientation, newOrientation, AAFraction);
		}
	}
}

void Player::HandleAACapture()
{
	Actor* actor = GetActor();
	m_isAimAssistActive = false;
	if (actor != nullptr)
	{
		RaycastResultDoomenstein raycastResult = m_map->RaycastWorldActors(
			m_position,
			m_orientation.GetForwardDir_IFwd_JLeft_KUp(),
			actor->m_equippedWeapon->m_definition->m_rayRange,
			actor
		);
		if (raycastResult.m_didImpact && raycastResult.m_impactDist > 2.f)
		{
			m_isAimAssistActive = true;
			m_aimAssistCapturedPos = raycastResult.m_impactPos;
		}
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
		m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.01f, 450.f);
		m_worldCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);
		m_worldCamera->SetViewport(playerViewport);
		m_screenCamera->SetOrthoView(Vec2(0, 0), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_screenCamera->SetViewport(playerViewport);
		m_viewport = playerViewport;
	}
	else
	{
		AABB2 playerViewport = AABB2(0.f, 0.f, (float)g_engine->m_window->GetClientDimensions().x, (float)g_engine->m_window->GetClientDimensions().y);
		m_worldCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.01f, 450.f);
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
