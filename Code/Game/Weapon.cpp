#include "Game/Weapon.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
//#include "Engine/DebugRender.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Math/Splines.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Portal.hpp"
#include "Game/glTFReader.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"

std::vector<WeaponDefinition*> WeaponDefinition::s_definitions;

void WeaponDefinition::InitializeDefinitions(const char* path)
{
	XmlDocument tileDefsXml;
	[[maybe_unused]] XmlResult result = tileDefsXml.LoadFile(path);
	XmlElement* rootElement = tileDefsXml.RootElement();
	XmlElement* weaponDefElement = rootElement->FirstChildElement();

	while (weaponDefElement)
	{
		WeaponDefinition* newWeaponDef = new WeaponDefinition();
		
		newWeaponDef->m_name =						ParseXmlAttribute(*weaponDefElement, "name", "");
		if (newWeaponDef->m_name == "PortalGun")
		{
			newWeaponDef->m_type = WeaponType::PORTALGUN;
		}
		else
		{
			newWeaponDef->m_type = WeaponType::WEAPON;
		}
		newWeaponDef->m_perk =						ParseXmlAttribute(*weaponDefElement, "perk", "");

		newWeaponDef->m_refireTime =				ParseXmlAttribute(*weaponDefElement, "refireTime", -1.f);
		newWeaponDef->m_maxAmmo =					ParseXmlAttribute(*weaponDefElement, "maxAmmo", -1);
		newWeaponDef->m_reloadTime =				ParseXmlAttribute(*weaponDefElement, "reloadTime", -1.f);
		newWeaponDef->m_recoil =					ParseXmlAttribute(*weaponDefElement, "recoil", -1.f);
		newWeaponDef->m_scopedFOV =					ParseXmlAttribute(*weaponDefElement, "scopedFOV", -1.f);
		newWeaponDef->m_canScope =					ParseXmlAttribute(*weaponDefElement, "canScope", false);

		newWeaponDef->m_rayCount =					ParseXmlAttribute(*weaponDefElement, "rayCount", -1);
		newWeaponDef->m_rayBurst =					ParseXmlAttribute(*weaponDefElement, "rayBurst", -1);
		newWeaponDef->m_rayBurstTime =				ParseXmlAttribute(*weaponDefElement, "rayBurstTime", -1.f);
		newWeaponDef->m_rayCone =					ParseXmlAttribute(*weaponDefElement, "rayCone", -1.f);
		newWeaponDef->m_rayRange =					ParseXmlAttribute(*weaponDefElement, "rayRange", -1.f);
		newWeaponDef->m_rayDamage =					ParseXmlAttribute(*weaponDefElement, "rayDamage", FloatRange());
		newWeaponDef->m_rayImpulse =				ParseXmlAttribute(*weaponDefElement, "rayImpulse", -1.f);
		newWeaponDef->m_precisionMultiplier =		ParseXmlAttribute(*weaponDefElement, "precisionMultiplier", -1.f);

		newWeaponDef->m_projectileCount =			ParseXmlAttribute(*weaponDefElement, "projectileCount", -1);
		newWeaponDef->m_projectileActor =			ParseXmlAttribute(*weaponDefElement, "projectileActor", "");
		newWeaponDef->m_secondaryProjectileActor =	ParseXmlAttribute(*weaponDefElement, "secondaryProjectileActor", "");
		newWeaponDef->m_projectileCone =			ParseXmlAttribute(*weaponDefElement, "projectileCone", 0.f);
		newWeaponDef->m_projectileSpeed =			ParseXmlAttribute(*weaponDefElement, "projectileSpeed", -1.f);

		newWeaponDef->m_meleeCount =				ParseXmlAttribute(*weaponDefElement, "meleeCount", -1);
		newWeaponDef->m_meleeArc =					ParseXmlAttribute(*weaponDefElement, "meleeArc", -1.f);
		newWeaponDef->m_meleeRange =				ParseXmlAttribute(*weaponDefElement, "meleeRange", -1.f);
		newWeaponDef->m_meleeDamage =				ParseXmlAttribute(*weaponDefElement, "meleeDamage", FloatRange());
		newWeaponDef->m_meleeImpulse =				ParseXmlAttribute(*weaponDefElement, "meleeImpulse", -1.f);

		newWeaponDef->m_portalHeight =				ParseXmlAttribute(*weaponDefElement, "portalHeight", -1.f);
		newWeaponDef->m_portalWidth =				ParseXmlAttribute(*weaponDefElement, "portalWidth", -1.f);

		//XmlElement* collisionElement = weaponDefElement->FirstChildElement("Collision");
		//if (collisionElement != nullptr)
		//{
		//	newWeaponDef->m_radius = ParseXmlAttribute(*collisionElement, "radius", -1.f);
		//}
		// 
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// HUD
		XmlElement* HUDElement = weaponDefElement->FirstChildElement("HUD");
		if (HUDElement != nullptr)
		{
			std::string shaderName = ParseXmlAttribute(*HUDElement, "shader", "");
			newWeaponDef->m_shader = g_engine->m_render->CreateOrGetShader(shaderName.c_str(), VertexType::VERTEX_PCUTBN);
			std::string baseTextureName = ParseXmlAttribute(*HUDElement, "baseTexture", "");
			if (baseTextureName != "")
			{
				newWeaponDef->m_baseTexture = g_engine->m_render->CreateOrGetTextureFromFile(baseTextureName.c_str());
			}
			std::string reticleTextureName = ParseXmlAttribute(*HUDElement, "reticleTexture", "");
			newWeaponDef->m_reticleTexture = g_engine->m_render->CreateOrGetTextureFromFile(reticleTextureName.c_str());
			newWeaponDef->m_reticleSize = ParseXmlAttribute(*HUDElement, "reticleSize", Vec2());
			newWeaponDef->m_spriteSize = ParseXmlAttribute(*HUDElement, "spriteSize", IntVec2());
			newWeaponDef->m_spritePivot = ParseXmlAttribute(*HUDElement, "spritePivot", Vec2());

			std::string gltfName = ParseXmlAttribute(*HUDElement, "gltfName", "");
			for (glTF_Asset* curGltfAsset : g_app->m_gltfModels)
			{
				if (gltfName == curGltfAsset->m_name)
				{
					newWeaponDef->m_gltfAssets.push_back(curGltfAsset);
				}
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Animation
		struct Animation
		{
			std::string		m_name;
			Shader* m_shader;
			SpriteSheet* m_spriteSheet;
			IntVec2			m_cellCount;
			float			m_secondsPerFrame;
			int				m_startFrame;
			int				m_endFrame;
		};
		if (HUDElement != nullptr)
		{
			XmlElement* AnimationElement = HUDElement->FirstChildElement("Animation");
			while (AnimationElement)
			{
				WeaponDefinition::Animation newAnimation = WeaponDefinition::Animation();

				newAnimation.m_name = ParseXmlAttribute(*AnimationElement, "name", "");
				std::string shaderName = ParseXmlAttribute(*AnimationElement, "shader", "");
				newAnimation.m_shader = g_engine->m_render->CreateOrGetShader(shaderName.c_str(), VertexType::VERTEX_PCUTBN);
				newAnimation.m_cellCount = ParseXmlAttribute(*AnimationElement, "cellCount", IntVec2());
				std::string spriteSheetName = ParseXmlAttribute(*AnimationElement, "spriteSheet", "");
				newAnimation.m_spriteSheet = new SpriteSheet(g_engine->m_render->CreateOrGetTextureFromFile(spriteSheetName.c_str()), newAnimation.m_cellCount);
				newAnimation.m_secondsPerFrame = ParseXmlAttribute(*AnimationElement, "secondsPerFrame", -1.f);
				newAnimation.m_startFrame = ParseXmlAttribute(*AnimationElement, "startFrame", -1);
				newAnimation.m_endFrame = ParseXmlAttribute(*AnimationElement, "endFrame", -1);

				SpriteAnimDefinition* newAnimDef = new SpriteAnimDefinition(*newAnimation.m_spriteSheet, newAnimation.m_startFrame, newAnimation.m_endFrame, 1.f / newAnimation.m_secondsPerFrame, SpriteAnimPlaybackType::ONCE);
				newAnimation.m_animDef = newAnimDef;

				newWeaponDef->m_animations.push_back(newAnimation);
				AnimationElement = AnimationElement->NextSiblingElement();
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// Sound
		XmlElement* SoundsElement = weaponDefElement->FirstChildElement("Sounds");
		if (SoundsElement != nullptr)
		{
			XmlElement* SoundElement = SoundsElement->FirstChildElement("Sound");
			while (SoundElement)
			{
				WeaponDefinition::Sound* newSound = new Sound();
				newSound->m_soundName = ParseXmlAttribute(*SoundElement, "sound", "");
				std::string soundPath = ParseXmlAttribute(*SoundElement, "name", "");
				newSound->m_sound = g_engine->m_audio->CreateOrGetSound(soundPath.c_str(), true);
				newWeaponDef->m_sounds.push_back(newSound);
				SoundElement = SoundElement->NextSiblingElement();
			}
		}

		s_definitions.push_back(newWeaponDef);
		weaponDefElement = weaponDefElement->NextSiblingElement();
	}
}

void WeaponDefinition::ClearDefinitions()
{
	s_definitions.clear();
}

const WeaponDefinition* WeaponDefinition::GetByName(const std::string& name)
{
	for (int mapIndex = 0; mapIndex < s_definitions.size(); ++mapIndex)
	{
		WeaponDefinition* currentDef = s_definitions[mapIndex];
		if (currentDef->m_name == name)
		{
			return currentDef;
		}
	}
	return nullptr;
}

Weapon::Weapon(Map* map, std::string definition)
	: m_map(map)
{
	m_definition = WeaponDefinition::GetByName(definition);

	m_fireTimer = new Timer(m_definition->m_refireTime, m_map->m_game->m_gameClock);
	m_fireTimer->Start();
	m_alternateFireTimer = new Timer(m_definition->m_refireTime, m_map->m_game->m_gameClock);
	m_alternateFireTimer->Start();
	m_reloadTimer = new Timer(m_definition->m_reloadTime, m_map->m_game->m_gameClock);
	m_burstTimer = new Timer(m_definition->m_rayBurstTime, m_map->m_game->m_gameClock);
	if (m_definition->m_perk == "WhiteNail")
	{
		m_perkTimer = new Timer(1.7f, m_map->m_game->m_gameClock);
	}
	if (m_definition->m_perk == "Broadside")
	{
		m_perkTimer = new Timer(0.5f, m_map->m_game->m_gameClock);
	}

	m_rideTheBullTimer = new Timer(m_rideTheBullTime, m_map->m_game->m_gameClock);

	m_animTimer = new Timer(0.0, m_map->m_game->m_gameClock);
	if (m_definition->m_animations.size() > 0)
	{
		m_defaultAnimation = m_definition->m_animations[0];
		m_defaultAnimation.m_animDef->SetPlaybackType(SpriteAnimPlaybackType::LOOP);
		m_animation = m_defaultAnimation;
		m_animTimer->m_period = m_animation.m_secondsPerFrame;
		m_animTimer->Start();
	}

	m_bullets = m_definition->m_maxAmmo;

	m_scopedTranslation.AppendTranslation3D(Vec3( -0.52f, 0.15f, 0.025f));
	m_scopedWhisperTranslation.AppendTranslation3D(Vec3( -0.67f, 0.15f, 0.f));
}

Weapon::~Weapon()
{
	delete m_fireTimer;
	m_fireTimer = nullptr;
}

void Weapon::Update(Actor* actor)
{
	if (m_animation.m_name != m_defaultAnimation.m_name && m_animTimer->HasPeriodElapsed())
	{
		SetAnimation(m_defaultAnimation);
	}

	if (m_reloadTimer->HasPeriodElapsed())
	{
		StopReload();
		m_bullets = m_definition->m_maxAmmo;
	}

	if (m_definition->m_perk == "WhiteNail")
	{
		if (m_perkTimer->DecrementPeriodIfElapsed())
		{
			m_consecutivePrecisionHits = 0;
			m_perkTimer->Stop();
		}
	}

	if (m_definition->m_perk == "Broadside")
	{
		if (m_perkTimer->DecrementPeriodIfElapsed())
		{
			m_consecutiveHits = 0;
			m_perkTimer->Stop();
		}
	}

	if (m_isScoped)
	{
		m_scopeFraction += (float)m_map->m_game->m_gameClock->GetDeltaSeconds() * 6.f;
		m_scopeFraction = GetClampedZeroToOne(m_scopeFraction);
	}
	else
	{
		m_scopeFraction -= (float)m_map->m_game->m_gameClock->GetDeltaSeconds() * 6.f;
		m_scopeFraction = GetClampedZeroToOne(m_scopeFraction);
	}

	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_DOWNARROW))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(0.f, 0.f, -0.05f));
	}
	if (g_engine->m_input->WasKeyJustPressed(KEYCODE_UPARROW))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(0.f, 0.f, 0.05f));
	}
	if (g_engine->m_input->WasKeyJustPressed('U'))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(0.f, 0.05f, 0.f));
	}
	if (g_engine->m_input->WasKeyJustPressed('I'))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(0.f, -0.05f, 0.f));
	}
	if (g_engine->m_input->WasKeyJustPressed('V'))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(0.05f, 0.f, 0.f));
	}
	if (g_engine->m_input->WasKeyJustPressed('B'))
	{
		m_scopedTranslation.AppendTranslation3D(Vec3(-0.05f, 0.f, 0.f));
	}

	// Recoil
	// This one resets back to normal much more quickly than the player's recoil.
	float interpolateValue = 10.f * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
	m_orientationRecoil.m_yawDegrees = Interpolate(m_orientationRecoil.m_yawDegrees, 0.f, interpolateValue);
	m_orientationRecoil.m_pitchDegrees = Interpolate(m_orientationRecoil.m_pitchDegrees, 0.f, interpolateValue);
	m_orientationRecoil.m_rollDegrees = Interpolate(m_orientationRecoil.m_rollDegrees, 0.f, interpolateValue);

	if (m_definition->m_rayBurst > 0 && m_burstTimer->DecrementPeriodIfElapsed())
	{
		FireBullet(actor);
		--m_burstAmount;
		if (m_burstAmount <= 0)
		{
			m_burstTimer->Stop();
		}
	}

	if (!g_engine->m_input->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		m_rideTheBull = 0;
		m_rideTheBullTimer->Stop();
		m_fireTimer->m_period = m_definition->m_refireTime;
	}

	Vec3 translation = m_scopedTranslation.GetTranslation3D();
	DebugAddMessage(Stringf("ScopedTranslation, %.2f %.2f %.2f", translation.x, translation.y, translation.z), 0.f, Rgba8::BLUE);
}

void Weapon::Render()
{
	// HUD
	std::vector<Vertex> hudVerts;
	AddVertsForAABB2D(hudVerts, AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y * 0.15f)), Rgba8::WHITE);
	g_engine->m_render->BindTexture(m_definition->m_baseTexture);
	g_engine->m_render->DrawVertexList(&hudVerts);
}

void Weapon::Render_Weapon()
{
	float aspectRatio = (g_engine->m_window->GetClientDimensions().x / g_engine->m_window->GetClientDimensions().y) / (g_engine->m_render->GetCamera()->GetViewport().GetWidth() / g_engine->m_render->GetCamera()->GetViewport().GetHeight());

	// Animation
	std::vector<Vertex> animationVerts;
	Vec2 spriteSize = Vec2((float)m_definition->m_spriteSize.x * aspectRatio, (float)m_definition->m_spriteSize.y);
	Vec2 spritePivot = m_definition->m_spritePivot * spriteSize;
	Vec2 spriteCenter = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.15f);
	AddVertsForAABB2D(animationVerts, AABB2(spriteCenter - spritePivot, spriteCenter + spriteSize - spritePivot), Rgba8::WHITE);
	if (m_definition->m_animations.size() > 0)
	{
		SpriteDef spriteDef = m_animation.m_animDef->GetSpriteDefAtTime((float)m_animTimer->GetElapsedTime());
		AABB2 spriteUVs = spriteDef.m_UVs;

		animationVerts[0].m_uvTexCoords = spriteUVs.m_mins;
		animationVerts[1].m_uvTexCoords = Vec2(spriteUVs.m_maxs.x, spriteUVs.m_mins.y);
		animationVerts[2].m_uvTexCoords = spriteUVs.m_maxs;

		animationVerts[3].m_uvTexCoords = spriteUVs.m_mins;
		animationVerts[4].m_uvTexCoords = spriteUVs.m_maxs;
		animationVerts[5].m_uvTexCoords = Vec2(spriteUVs.m_mins.x, spriteUVs.m_maxs.y);

		g_engine->m_render->BindTexture(m_animation.m_spriteSheet->GetTexture());
		g_engine->m_render->DrawVertexList(&animationVerts);
	}

	// Reticle
	Vec2 reticleHalfSize = m_definition->m_reticleSize * 0.5f;
	reticleHalfSize.x = reticleHalfSize.x * aspectRatio;
	Vec2 screenCenter = Vec2(SCREEN_SIZE_X * 0.5f, SCREEN_SIZE_Y * 0.5f);
	std::vector<Vertex> reticleVerts;
	AddVertsForAABB2D(reticleVerts, AABB2(screenCenter - reticleHalfSize, screenCenter + reticleHalfSize), Rgba8::WHITE);
	g_engine->m_render->BindTexture(m_definition->m_reticleTexture);
	g_engine->m_render->DrawVertexList(&reticleVerts);
}

void Weapon::Render_GLTF()
{
	if (m_definition->m_gltfAssets.size() > 0)
	{
		Player* currentPlayer = m_map->m_game->m_currentlyRenderedPlayer;
		Mat44 modelMatrix = Mat44();
		modelMatrix.AppendTranslation3D(currentPlayer->m_position);

		if (currentPlayer->m_ballInsideOf != nullptr)
		{
			//EulerAngles orientationInBallSpace = m_orientation + m_ballInsideOf->m_orientation;
			modelMatrix.Append((currentPlayer->m_ballInsideOf->m_orientation + currentPlayer->m_orientationRecoil).GetAsMatrix_IFwd_JLeft_KUp());
		}
		else
		{
			modelMatrix.Append((currentPlayer->m_orientationRecoil).GetAsMatrix_IFwd_JLeft_KUp());
		}
		modelMatrix.Append(m_orientationRecoil.GetAsMatrix_IFwd_JLeft_KUp());
		modelMatrix.AppendTranslation3D(Vec3(0.5f, -0.15f, -0.15f));

		if (m_scopeFraction > 0.f)
		{
			Mat44 interpolatedScopeMatrix;
			if (m_definition->m_name == "BlackSpindle")
			{
				interpolatedScopeMatrix = Interpolate(Mat44(), m_scopedWhisperTranslation, SmoothStep3(m_scopeFraction));
			}
			else
			{
				interpolatedScopeMatrix = Interpolate(Mat44(), m_scopedTranslation, SmoothStep3(m_scopeFraction));
			}
			modelMatrix.Append(interpolatedScopeMatrix);
		}

		modelMatrix.Append(Camera::GLTF_TO_GAME_CONVENTIONS);

		g_engine->m_render->SetModelConstants(modelMatrix, Rgba8::WHITE);

		for (glTF_Asset* asset : m_definition->m_gltfAssets)
		{
			asset->Test_RenderModel();
		}
	}
}

void Weapon::Fire(Actor* actor)
{
	// Ride The Bull perk
	if (m_definition->m_perk == "RideTheBull" && !m_isReloading && m_bullets > 0)
	{
		if (m_rideTheBullTimer->IsStopped())
		{
			m_rideTheBullTimer->Start();
		}
		else if (m_rideTheBull < m_maxRideTheBull && m_rideTheBullTimer->DecrementPeriodIfElapsed())
		{
			++m_rideTheBull;
			m_fireTimer->m_period = m_definition->m_refireTime / (1.f + (0.1f * m_rideTheBull));
		}
	}
	else
	{
		m_rideTheBull = 0;
		m_rideTheBullTimer->Stop();
		m_fireTimer->m_period = m_definition->m_refireTime;
	}

	if (!m_fireTimer->DecrementPeriodIfElapsed())
	{
		return;
	}
	else if (m_definition->m_maxAmmo != -1 && m_bullets <= 0)
	{
		StartReload(actor);
		return;
	}
	else
	{
		if (m_definition->m_rayBurst > 0)
		{
			m_burstAmount = m_definition->m_rayBurst - 1;
			m_burstTimer->Start();
		}
		FireBullet(actor);
	}
}

void Weapon::FireBullet(Actor* actor)
{
	if (m_definition->m_maxAmmo != -1 && m_bullets < 0)
	{
		return;
	}
	StopReload();
	--m_bullets;
	m_fireTimer->Start();
	actor->SetAnimGroup("Attack");
	SetAnimation("Attack");
	SoundPlaybackID playbackID = PlaySoundOnActor("Fire", actor);
	if (playbackID != -1)
	{
		g_engine->m_audio->SetSoundPlaybackSpeed(playbackID, 1.f + (0.02f * m_rideTheBull));
		actor->AddSoundPlaybackID(playbackID);
	}

	// Add recoil
	if (m_definition->m_recoil != -1.f && actor->m_controller != nullptr && actor->m_controller->IsPlayer())
	{
		float recoil = m_definition->m_recoil;
		// If zoomed in, lower recoil
		if (m_scopeFraction > 0.5f)
		{
			recoil /= 1.5f;
		}
		float recoilDirRotation = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(-30.f, 30.f);
		Vec2 recoilDir = Vec2(recoil, 0.f);
		recoilDir = recoilDir.GetRotatedByDegrees(recoilDirRotation);
		((Player*)actor->m_controller)->m_recoil.m_pitchDegrees -= recoilDir.x * (1.f + (0.1f * m_rideTheBull));
		((Player*)actor->m_controller)->m_recoil.m_yawDegrees -= recoilDir.y * (1.f + (0.1f * m_rideTheBull));
		m_orientationRecoil.m_pitchDegrees -= recoilDir.x * (1.f + (0.1f * m_rideTheBull) * 0.5f);
		m_orientationRecoil.m_yawDegrees -= recoilDir.y * (1.f + (0.1f * m_rideTheBull) * 0.5f);
	}

	if (m_definition->m_perk == "Broadside")
	{
		++m_consecutiveHits;
		m_perkTimer->Start();
	}

	switch (m_definition->m_type)
	{
	case WeaponType::WEAPON:	Fire_Weapon(actor);		break;
	case WeaponType::PORTALGUN: Fire_PortalGun(actor);	break;
	}
}

void Weapon::AlternateFire(Actor* actor)
{
	if (!m_alternateFireTimer->DecrementPeriodIfElapsed())
	{
		return;
	}
	else
	{
		m_alternateFireTimer->Start();
		actor->SetAnimGroup("Attack");
		SetAnimation("Attack");
		PlaySoundOnActor("Fire", actor);
	}
	switch (m_definition->m_type)
	{
		case WeaponType::WEAPON:	AlternateFire_Weapon(actor);		break;
		case WeaponType::PORTALGUN: AlternateFire_PortalGun(actor);		break;
	}
}

void Weapon::AlternateFire_Weapon([[maybe_unused]] Actor* actor)
{

}

void Weapon::AlternateFire_PortalGun(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, m_definition->m_rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RoundDownToInt(RandomDamage), actor->m_handle);
				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse);
			}
			else if (result.m_didImpact)
			{
				if (m_rightPortal != nullptr)
				{
					actor->m_map->RemovePortal(m_rightPortal);
					m_rightPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001f;
				m_rightPortal = new Portal(actor->m_map, portalPosition, EulerAngles(), m_definition->m_portalHeight, m_definition->m_portalWidth);
				m_rightPortal->m_isFlipped = true;

				Vec3 inverseImpactNormal = result.m_impactNormal * -1.f; // Flip the right portal so that it is directing towards the surface its on. This will make the link between both portals shoot things that are entering away from the walls instead of towards them.
				if (inverseImpactNormal.z == 0.f) // If the impact normal is horizontal
				{
					Mat44 portalOrientation = Mat44(inverseImpactNormal, CrossProduct3D(Vec3(0.f, 0.f, 1.f), inverseImpactNormal), Vec3(0.f, 0.f, 1.f), Vec3());
					m_rightPortal->SetOrientation(EulerAngles(portalOrientation));
				}
				else // If the impact normal is vertical
				{
					//Vec3 iBasis = inverseImpactNormal;
					//Vec3 jBasis = CrossProduct3D(inverseImpactNormal, actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp());
					//Vec3 kBasis = CrossProduct3D(jBasis, inverseImpactNormal);

					//EulerAngles rightPortalOrientation = EulerAngles();
					//rightPortalOrientation.m_yawDegrees = Atan2Degrees(iBasis.y, iBasis.x);
					//rightPortalOrientation.m_pitchDegrees = AsinDegrees(-iBasis.z);

					//if (iBasis.z == -1.f)
					//{
					//	kBasis *= -1.f;
					//}

					//rightPortalOrientation.m_rollDegrees = Atan2Degrees(kBasis.y, kBasis.x);

					EulerAngles rightPortalOrientation = actor->m_orientation;

					if (result.m_impactNormal.z == 1.f)
					{
						rightPortalOrientation.m_pitchDegrees = 90.f;
					}
					else
					{
						rightPortalOrientation.m_pitchDegrees = -90.f;
					}

					m_rightPortal->SetOrientation(rightPortalOrientation);
				}

				m_rightPortal->AssignPortal(m_leftPortal);
				if (m_leftPortal != nullptr)
				{
					m_leftPortal->AssignPortal(m_rightPortal);
				}

				actor->m_map->AddPortal(m_rightPortal);
			}
		}
	}
}

void Weapon::SetAnimation(std::string animationName)
{
	if (m_animation.m_name == animationName)
	{
		return;
	}
	for (WeaponDefinition::Animation curAnimation : m_definition->m_animations)
	{
		if (curAnimation.m_name == animationName)
		{
			m_animation = curAnimation;
			m_animTimer->m_period = m_animation.m_secondsPerFrame * (m_animation.m_endFrame - m_animation.m_startFrame);
			m_animTimer->Start();
			return;
		}
	}
}

void Weapon::SetAnimation(WeaponDefinition::Animation animation)
{
	m_animation = animation;
	m_animTimer->m_period = m_animation.m_secondsPerFrame * (m_animation.m_endFrame - m_animation.m_startFrame);
	m_animTimer->Start();
}

void Weapon::StartReload(Actor* actor)
{
	if (!m_isReloading && m_bullets < m_definition->m_maxAmmo)
	{
		m_reloadSound = PlaySoundOnActor("Reload", actor);
		m_reloadTimer->Start();
		m_isReloading = true;
	}
}

void Weapon::StopReload()
{
	if (m_reloadSound != -1)
	{
		g_engine->m_audio->StopSound(m_reloadSound);
	}
	m_isReloading = false;
	m_reloadTimer->Stop();
}

void Weapon::startScope()
{
	m_isScoped = true;
}

void Weapon::StopScope()
{
	m_isScoped = false;
}

void Weapon::PushImpactPointToFitSurface(RaycastResultDoomenstein& result)
{
	// Push the portal to fit onto whatever surface its on.
	if (result.m_impactNormal.z == 0.f) // Is a vertical (wall) portal
	{
		int tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * 0.01f);
		const Tile* tileInFrontOfPortal = m_map->GetTile(tileIndex);
		float halfPortalHeight = m_definition->m_portalHeight * 0.5f;
		float halfPortalWidth = m_definition->m_portalWidth * 0.5f;

		// Has a floor and portal is over it
		if (tileInFrontOfPortal->m_tileDefinition->m_floorSpriteCoords != IntVec2(-1, -1) &&
			result.m_impactPos.z - halfPortalHeight < tileInFrontOfPortal->m_bounds.m_mins.z)
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_mins.z + halfPortalHeight;
		}
		// Has a Ceiling and portal is over it
		if (tileInFrontOfPortal->m_tileDefinition->m_ceilingSpriteCoords != IntVec2(-1, -1) &&
			result.m_impactPos.z + halfPortalHeight > tileInFrontOfPortal->m_bounds.m_maxs.z)
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_maxs.z - halfPortalHeight;
		}

		Vec3 rotatedImpactNormal = result.m_impactNormal.GetRotatedAboutZDegrees(90.f);
		Vec3 rotatedWidthVec = rotatedImpactNormal * halfPortalWidth;
		tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * -0.01f + Vec3(0.f, 0.f, 1.f));
		const Tile* tileAbovePortal = m_map->GetTile(tileIndex);
		tileIndex = m_map->GetTileIndexFromWorldPosition(result.m_impactPos + result.m_impactNormal * -0.01f - Vec3(0.f, 0.f, 1.f));
		const Tile* tileBelowPortal = m_map->GetTile(tileIndex);

		Vec3 rightTilePos = (result.m_impactPos + result.m_impactNormal * -0.01f + rotatedImpactNormal);
		Vec3 leftTilePos = (result.m_impactPos + result.m_impactNormal * -0.01f - rotatedImpactNormal);
		tileIndex = m_map->GetTileIndexFromWorldPosition(rightTilePos);
		const Tile* tileRightOfPortal = m_map->GetTile(tileIndex);
		tileIndex = m_map->GetTileIndexFromWorldPosition(leftTilePos);
		const Tile* tileLeftOfPortal = m_map->GetTile(tileIndex);

		// Portal hanging off top
		if (tileAbovePortal == nullptr ||
			(tileAbovePortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1, -1) &&
				result.m_impactPos.z + halfPortalHeight > tileAbovePortal->m_bounds.m_mins.z))
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_maxs.z - halfPortalHeight;
		}
		// Portal hanging off bottom
		if (tileBelowPortal == nullptr ||
			(tileBelowPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1, -1) &&
				result.m_impactPos.z + halfPortalHeight < tileBelowPortal->m_bounds.m_maxs.z))
		{
			result.m_impactPos.z = tileInFrontOfPortal->m_bounds.m_mins.z + halfPortalHeight;
		}
		// Portal hanging off right side
		if (tileRightOfPortal == nullptr ||
			(tileRightOfPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1, -1) &&
				IsPointInsideAABB3D((result.m_impactPos + result.m_impactNormal * -0.01f + rotatedWidthVec), tileRightOfPortal->m_bounds)))
		{
			if (result.m_impactNormal.x == 0) // Portal is along x axis
			{
				if (fmod(result.m_impactPos.x, 1.f) > 0.5f)
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) + halfPortalWidth;
				}
			}
			else // Portal is along y axis
			{
				if (fmod(result.m_impactPos.y, 1.f) > 0.5f)
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) + halfPortalWidth;
				}
			}
		}
		// Portal hanging off left side
		if (tileLeftOfPortal == nullptr ||
			(tileLeftOfPortal->m_tileDefinition->m_wallSpriteCoords == IntVec2(-1, -1) &&
				IsPointInsideAABB3D((result.m_impactPos + result.m_impactNormal * -0.01f - rotatedWidthVec), tileLeftOfPortal->m_bounds)))
		{
			if (result.m_impactNormal.x == 0) // Portal is along x axis
			{
				if (fmod(result.m_impactPos.x, 1.f) > 0.5f)
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.x = (float)RoundDownToInt(result.m_impactPos.x) + halfPortalWidth;
				}
			}
			else // Portal is along y axis
			{
				if (fmod(result.m_impactPos.y, 1.f) > 0.5f)
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) - halfPortalWidth + 1.f;
				}
				else
				{
					result.m_impactPos.y = (float)RoundDownToInt(result.m_impactPos.y) + halfPortalWidth;
				}
			}
		}
	}
	else // Is a horizontal (floor/ceiling) portal
	{

	}
}

SoundPlaybackID Weapon::PlaySoundOnActor(std::string soundName, Actor* actor)
{
	for (WeaponDefinition::Sound* sound : m_definition->m_sounds)
	{
		if (sound->m_soundName == soundName)
		{
			SoundPlaybackID playbackID = g_engine->m_audio->StartSoundAt(sound->m_sound, actor->m_position, false);
			actor->AddSoundPlaybackID(playbackID);
			return playbackID;
		}
	}
	return (SoundPlaybackID)-1;
}

void Weapon::Fire_Weapon(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		float rayRange = m_definition->m_rayRange;
		float rayCone = m_definition->m_rayCone;
		if (m_scopeFraction > 0.5f)
		{
			rayRange *= 1.3f;
			rayCone /= 1.8f;
		}
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), rayCone * (1.f + 0.25f * (float)m_consecutiveHits));
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				if (result.m_actor->m_controller != nullptr && result.m_actor->m_controller->IsPlayer() && ((Player*)result.m_actor->m_controller)->m_godMode) // This is here to prevent players from being pushed around in god mode.
				{
					return;
				}
				// Was it a precision hit?
				float precisionMultiplier = 1.f;
				RaycastResult3D precisionResult = result.m_actor->RaycastVsPrecision(initialFirePosition, randomDirection, rayRange);
				if (precisionResult.m_didImpact)
				{
					precisionMultiplier = m_definition->m_precisionMultiplier;
					if (m_definition->m_perk == "WhiteNail")
					{
						++m_consecutivePrecisionHits;
						if (m_consecutivePrecisionHits >= 3)
						{
							m_bullets = m_definition->m_maxAmmo;
							m_consecutivePrecisionHits = 0;
						}
						m_perkTimer->Start();
					}
				}

				float damageFalloffMultiplier = SmoothStop6(1.f - (result.m_impactDist / rayRange));
				float RandomDamage = damageFalloffMultiplier * // Damage falloff
									 precisionMultiplier * // Precision damage
									 (1.f + 0.4f * (float)m_consecutiveHits) * // Broadside perk damage increase
									 result.m_actor->m_definition->m_armorMultiplier * // Actor armor multiplier
									 actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RoundDownToInt(RandomDamage), actor->m_handle);

				if (result.m_actor->m_health <= 0 )
				{
					if (m_definition->m_perk == "RideTheBull")
					{
						m_bullets = m_definition->m_maxAmmo;
					}
					if (m_definition->m_perk == "Redemption")
					{
						actor->Heal(10);
					}
				}

				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse / result.m_actor->m_definition->m_radius);
				result.m_actor->m_map->SpawnActor("BloodSplatter", result.m_impactPos, EulerAngles());

				// Damage Number
				Actor* damageNumber = result.m_actor->m_map->SpawnActor("DamageNumber", result.m_impactPos + ( -randomDirection * 0.1f), EulerAngles(), RangeMap(RandomDamage, 10.f, 100.f, 1.f, 4.f));
				if (precisionResult.m_didImpact)
				{
					damageNumber->m_color = Rgba8(255, 255, 50, 255);
				}
				else
				{
					damageNumber->m_color = Rgba8(150, 150, 150, 255);
				}
				Vec3 damageNumberToActor = (actor->m_position - result.m_impactPos).GetNormalized();
				Mat44 damageNumberToActorMatrix = Mat44(damageNumberToActor, Vec3(1.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f), Vec3());
				damageNumberToActorMatrix.Orthonormalize_XFwd_YLeft_ZUp();
				EulerAngles damageNumberToActorOrientation = EulerAngles(damageNumberToActorMatrix);
				damageNumberToActorOrientation.m_yawDegrees += 90.f;
				damageNumberToActorOrientation.m_pitchDegrees += actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(0.f, 360.f);
				damageNumber->AddImpulse(actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(1.f, 2.f) * damageNumberToActorOrientation.GetForwardDir_IFwd_JLeft_KUp());
				damageNumber->m_valueToDisplay = RandomDamage;
			}
			else if (result.m_didImpact)
			{
				if (result.m_riftMap != nullptr)
				{
					result.m_riftMap->SpawnActor("BulletHit", result.m_impactPos, EulerAngles());
				}	
				else
				{
					actor->m_map->SpawnActor("BulletHit", result.m_impactPos, EulerAngles());
				}
			}
		}
	}
	if (!m_definition->m_projectileActor.empty())
	{
		for (int projectileIndex = 0; projectileIndex < m_definition->m_projectileCount; ++projectileIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_projectileCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.1f);

			SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
			Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
			projectile->m_owner = actor;
			projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
		}
	}
	if (m_definition->m_meleeCount != -1)
	{
		std::vector<Actor*> actors = actor->m_map->GetActors();
		for (int actorIndex = 0; actorIndex < actors.size(); ++actorIndex)
		{
			Actor* currentActor = actors[actorIndex];
			bool isActorSameFaction = actor->m_map->AreActorsSameFaction(actor, currentActor);

			if (currentActor != nullptr && currentActor != actor && !isActorSameFaction)
			{
				Vec3 distFromSelfToOther = currentActor->m_position - actor->m_position;
				float angleBetweenSelfToOtherAndForward = ConvertRadiansToDegrees(GetAngleDegreesBetweenVectors3D(distFromSelfToOther.GetNormalized(), currentActor->m_orientation.GetForwardDir_IFwd_JLeft_KUp()));
				if (distFromSelfToOther.GetLength() < m_definition->m_meleeRange &&
					angleBetweenSelfToOtherAndForward < m_definition->m_meleeArc)
				{
					if (currentActor->m_controller != nullptr && currentActor->m_controller->IsPlayer() && ((Player*)currentActor->m_controller)->m_godMode) // This is here to prevent players from being pushed around in god mode.
					{
						return;
					}
					float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_meleeDamage.m_min, m_definition->m_meleeDamage.m_max);
					currentActor->Damage(RoundDownToInt(RandomDamage), actor->m_handle);
					currentActor->AddImpulse(distFromSelfToOther.GetNormalized() * m_definition->m_meleeImpulse);
				}
			}
		}
	}
}

void Weapon::Fire_PortalGun(Actor* actor)
{
	if (m_definition->m_rayCount != -1)
	{
		for (int rayIndex = 0; rayIndex < m_definition->m_rayCount; ++rayIndex)
		{
			Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_rayCone);
			Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight);

			RaycastResultDoomenstein result = actor->m_map->RaycastAll(initialFirePosition, randomDirection, m_definition->m_rayRange, actor);
			if (result.m_didImpact && result.m_actor != nullptr)
			{
				float RandomDamage = actor->m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				result.m_actor->Damage(RoundDownToInt(RandomDamage), actor->m_handle);
				result.m_actor->AddImpulse(randomDirection * m_definition->m_rayImpulse);
			}
			else if (result.m_didImpact)
			{
				if (m_leftPortal != nullptr)
				{
					actor->m_map->RemovePortal(m_leftPortal);
					m_leftPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001f;
				m_leftPortal = new Portal(actor->m_map, portalPosition, EulerAngles(), m_definition->m_portalHeight, m_definition->m_portalWidth);
				
				if (result.m_impactNormal.z == 0.f) // If the impact normal is horizontal
				{
					Mat44 portalOrientation = Mat44(result.m_impactNormal, CrossProduct3D(Vec3(0.f,0.f,1.f), result.m_impactNormal), Vec3(0.f,0.f,1.f), Vec3());
					m_leftPortal->SetOrientation(EulerAngles(portalOrientation));
				}
				else // If the impact normal is vertical
				{
					//Vec3 iBasis = result.m_impactNormal;
					//Vec3 jBasis = CrossProduct3D(iBasis, actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp());
					//Vec3 kBasis = CrossProduct3D(jBasis, iBasis);

					//EulerAngles leftPortalOrientation = EulerAngles();
					//leftPortalOrientation.m_yawDegrees = Atan2Degrees(iBasis.y, iBasis.x);
					//leftPortalOrientation.m_pitchDegrees = AsinDegrees(-iBasis.z);

					//if (iBasis.z == 1.f)
					//{
					//	kBasis *= -1.f;
					//}

					//leftPortalOrientation.m_rollDegrees = Atan2Degrees(kBasis.y, kBasis.x);

					EulerAngles leftPortalOrientation = actor->m_orientation;

					if (result.m_impactNormal.z == 1.f)
					{
						leftPortalOrientation.m_pitchDegrees = -90.f;
					}
					else
					{
						leftPortalOrientation.m_pitchDegrees = 90.f;
					}

					m_leftPortal->SetOrientation(leftPortalOrientation);
				}

				m_leftPortal->AssignPortal(m_rightPortal);
				if (m_rightPortal != nullptr)
				{
					m_rightPortal->AssignPortal(m_leftPortal);
				}

				actor->m_map->AddPortal(m_leftPortal);
			}
		}
	}
}
