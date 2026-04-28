#include "Game/Weapon.hpp"
#include "Engine/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
//#include "Engine/DebugRender.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/VertexUtils.hpp"

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

		newWeaponDef->m_refireTime =				ParseXmlAttribute(*weaponDefElement, "refireTime", -1.f);

		newWeaponDef->m_rayCount =					ParseXmlAttribute(*weaponDefElement, "rayCount", -1);
		newWeaponDef->m_rayCone =					ParseXmlAttribute(*weaponDefElement, "rayCone", -1.f);
		newWeaponDef->m_rayRange =					ParseXmlAttribute(*weaponDefElement, "rayRange", -1.f);
		newWeaponDef->m_rayDamage =					ParseXmlAttribute(*weaponDefElement, "rayDamage", FloatRange());
		newWeaponDef->m_rayImpulse =				ParseXmlAttribute(*weaponDefElement, "rayImpulse", -1.f);

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
			newWeaponDef->m_soundName = ParseXmlAttribute(*SoundElement, "sound", "");
			std::string soundPath = ParseXmlAttribute(*SoundElement, "name", "");
			newWeaponDef->m_sound = g_engine->m_audio->CreateOrGetSound(soundPath.c_str(), true);
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

	m_animTimer = new Timer(0.0, m_map->m_game->m_gameClock);
	if (m_definition->m_animations.size() > 0)
	{
		m_defaultAnimation = m_definition->m_animations[0];
		m_defaultAnimation.m_animDef->SetPlaybackType(SpriteAnimPlaybackType::LOOP);
		m_animation = m_defaultAnimation;
		m_animTimer->m_period = m_animation.m_secondsPerFrame;
		m_animTimer->Start();
	}
}

Weapon::~Weapon()
{
	delete m_fireTimer;
	m_fireTimer = nullptr;
}

void Weapon::Update()
{
	if (m_animation.m_name != m_defaultAnimation.m_name && m_animTimer->HasPeriodElapsed())
	{
		SetAnimation(m_defaultAnimation);
	}
}

void Weapon::Render()
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

	// HUD
	std::vector<Vertex> hudVerts;
	AddVertsForAABB2D(hudVerts, AABB2(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y * 0.15f)), Rgba8::WHITE);
	g_engine->m_render->BindTexture(m_definition->m_baseTexture);
	g_engine->m_render->DrawVertexList(&hudVerts);

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
		Player* currentPlayer = m_map->m_currentlyRenderedPlayer;
		Mat44 modelMatrix = Mat44();
		modelMatrix.AppendTranslation3D(currentPlayer->m_position);

		modelMatrix.Append(currentPlayer->m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
		modelMatrix.AppendTranslation3D(Vec3(0.5f, -0.15f, -0.15f));

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
	if (!m_fireTimer->DecrementPeriodIfElapsed())
	{
		return;
	}
	else
	{
		m_fireTimer->Start();
		actor->SetAnimGroup("Attack");
		SetAnimation("Attack");
		SoundPlaybackID playbackID = g_engine->m_audio->StartSoundAt(m_definition->m_sound, actor->m_position, false);
		actor->AddSoundPlaybackID(playbackID);
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
		SoundPlaybackID playbackID = g_engine->m_audio->StartSoundAt(m_definition->m_sound, actor->m_position, false);
		actor->AddSoundPlaybackID(playbackID);
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
					m_map->RemovePortal(m_rightPortal);
					m_rightPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001f;
				m_rightPortal = new Portal(m_map, portalPosition, EulerAngles(), m_definition->m_portalHeight, m_definition->m_portalWidth);
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

				m_map->AddPortal(m_rightPortal);
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

void Weapon::Fire_Weapon(Actor* actor)
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
				m_map->SpawnActor("BloodSplatter", result.m_impactPos, EulerAngles());
			}
			else
			{
				m_map->SpawnActor("BulletHit", result.m_impactPos, EulerAngles());
			}
		}
	}
	if (!m_definition->m_projectileActor.empty())
	{
		Vec3 randomDirection = actor->m_map->m_game->m_randomNumberGenerator->RollRandomDirectionInCone(actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp(), m_definition->m_projectileCone);
		Vec3 initialFirePosition = actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight - 0.1f);

		SpawnInfo spawnInfo = SpawnInfo(m_definition->m_projectileActor, initialFirePosition, actor->m_orientation);
		Actor* projectile = actor->m_map->SpawnActor(spawnInfo);
		projectile->m_owner = actor;
		projectile->AddImpulse(randomDirection * m_definition->m_projectileSpeed);
	}
	if (m_definition->m_meleeCount != -1)
	{
		std::vector<Actor*> actors = m_map->GetActors();
		for (int actorIndex = 0; actorIndex < actors.size(); ++actorIndex)
		{
			Actor* currentActor = actors[actorIndex];
			bool isActorSameFaction = m_map->AreActorsSameFaction(actor, currentActor);

			if (currentActor != nullptr && currentActor != actor && !isActorSameFaction)
			{
				Vec3 distFromSelfToOther = currentActor->m_position - actor->m_position;
				float angleBetweenSelfToOtherAndForward = ConvertRadiansToDegrees(GetAngleDegreesBetweenVectors3D(distFromSelfToOther.GetNormalized(), currentActor->m_orientation.GetForwardDir_IFwd_JLeft_KUp()));
				if (distFromSelfToOther.GetLength() < m_definition->m_meleeRange &&
					angleBetweenSelfToOtherAndForward < m_definition->m_meleeArc)
				{
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
					m_map->RemovePortal(m_leftPortal);
					m_leftPortal = nullptr;
				}
				//Vec3 portalPosition = actor->m_position + actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp() * 1.f;

				PushImpactPointToFitSurface(result);

				Vec3 portalPosition = result.m_impactPos + result.m_impactNormal * 0.0001f;
				m_leftPortal = new Portal(m_map, portalPosition, EulerAngles(), m_definition->m_portalHeight, m_definition->m_portalWidth);
				
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

				m_map->AddPortal(m_leftPortal);
			}
		}
	}
}
