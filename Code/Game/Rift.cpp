#include "Game/Rift.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Timer.hpp"

#include "Game/App.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/Actor.hpp"

Rift::Rift(Vec3 const& startingPosition, EulerAngles orientation, float height, float width, float sizeScale)
	: Portal(nullptr, startingPosition, orientation, height, width, sizeScale) // Map is nullptr because the rifts are a static variable separate from the maps. The maps should pass in themselves to the proper functions so that the rift knows which map is using it.
{
	m_vertexes.clear();
	AddVertsForQuad3D(m_vertexes, 
						bl + Vec3(0.f, 0.5f, -0.5f), 
						br + Vec3(0.f, -0.5f, -0.5f),
						tr + Vec3(0.f, -0.5f, 0.5f),
						tl + Vec3(0.f, 0.5f, 0.5f));

	m_deathTimer = new Timer(g_app->m_game->m_riftStencilAnim->GetTotalTime(), g_app->m_game->m_gameClock);
}

Rift::~Rift()
{

}

void Rift::Update()
{

	if (m_deathTimer->HasPeriodElapsed())
	{
		m_isGarbage = true;
	}

	if (!m_isDead)
	{
		DebugAddWorldWireSphere(GetPosition(), GetScale(), 0.f, Rgba8::MAGENTA);
		m_actorsNearRift.clear();
		for (Actor* actor : g_app->m_game->m_currentMap->GetActors())
		{
			if (
				actor != nullptr &&
				actor->m_definition->m_cellCount != IntVec2() &&
				DoSpheresOverlap(
					actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_height * 0.5f),
					actor->m_definition->m_height * 0.5f,
					GetPosition(),
					GetScale())
				)
			{
				DebugAddWorldWireSphere(actor->m_position, 0.2f, 0.f, Rgba8::GREEN, Rgba8::GREEN);
				actor->m_riftCollidingWith = this;
				m_actorsNearRift.push_back(actor);
			}
		}
		for (Actor* actor : g_app->m_game->m_currentRiftMap->GetActors())
		{
			if (
				actor != nullptr &&
				actor->m_definition->m_cellCount != IntVec2() &&
				DoSpheresOverlap(
					actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_height * 0.5f),
					actor->m_definition->m_height * 0.5f,
					GetPosition(),
					GetScale())
				)
			{
				DebugAddWorldWireSphere(actor->m_position, 0.2f, 0.f, Rgba8::GREEN, Rgba8::GREEN);
				actor->m_riftCollidingWith = this;
				m_actorsNearRift.push_back(actor);
			}
		}
	}
}

void Rift::RenderRift(const Map* map)
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Switch to rendering to the target texture instead of back buffer
	g_engine->m_render->ChangeRenderTargetToTextureBuffer();
	g_engine->m_render->ClearTargetTextureBuffer(map->m_game->m_backgroundClearColor);
	g_engine->m_render->ClearTargetTextureDepthBuffer();

	// Render Everything from the player's camera, but from the perspective of if it was in the rift map. 
	g_engine->m_render->BeginCamera(map->m_game->m_currentlyRenderedPlayer->m_worldCamera);

	Vec3 portalNormal = GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
	Vec3 portalLeft = GetOrientation().GetLeftDir_IFwd_JLeft_KUp();
	Vec3 portalUp = GetOrientation().GetUpDir_IFwd_JLeft_KUp();
	Mat44 portalOrientationMatrix = GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Vec4 portalPlane;

	float playerDotPortal = DotProduct3D(portalNormal, map->m_game->m_currentlyRenderedPlayer->m_position - m_position);

	if (playerDotPortal < 0.f)
	{
		portalPlane = Vec4(portalNormal.x, portalNormal.y, portalNormal.z, DotProduct3D(-portalNormal, m_position));
	}
	else
	{
		portalPlane = Vec4(-portalNormal.x, -portalNormal.y,- portalNormal.z, DotProduct3D(portalNormal,m_position));
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Render world
	ClipPlaneConstants clipPlaneConstants = ClipPlaneConstants();
	clipPlaneConstants.gClipPlane[0] = portalPlane;
	clipPlaneConstants.gClipPlane[1] = Vec4(-portalLeft.x, -portalLeft.y, -portalLeft.z, DotProduct3D(portalLeft, m_position + portalOrientationMatrix.TransformPosition3D(m_sizeScale * bl))); // Left clip
	clipPlaneConstants.gClipPlane[2] = Vec4(portalLeft.x, portalLeft.y, portalLeft.z, DotProduct3D(-portalLeft, m_position + portalOrientationMatrix.TransformPosition3D(m_sizeScale * br))); // Right clip
	clipPlaneConstants.gClipPlane[3] = Vec4(-portalUp.x, -portalUp.y, -portalUp.z, DotProduct3D(portalUp, m_position + portalOrientationMatrix.TransformPosition3D(m_sizeScale * tl))); // Up clip
	clipPlaneConstants.gClipPlane[4] = Vec4(portalUp.x, portalUp.y, portalUp.z, DotProduct3D(-portalUp, m_position + portalOrientationMatrix.TransformPosition3D(m_sizeScale * bl))); // Bottom clip
	clipPlaneConstants.amountOfClipPlanes = 1;
	clipPlaneConstants.isEnabled = 1;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, map->m_riftMap->m_clipPlaneCBO);

	map->m_riftMap->Render_World();
	Render_ActorsNearRift(map->m_riftMap);

	clipPlaneConstants.isEnabled = 0;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, map->m_riftMap->m_clipPlaneCBO);
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	g_engine->m_render->ChangeRenderTargetToBackBuffer();
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// IF the near plane intersects the portal, write a quad to the stencil buffer which only appears on one side of the screen.
	// Drawn in world space? calculate the near plane in world and draw it there in world space, so that it can be affected by the portal clipping?
	g_engine->m_render->BindShader(map->m_definition->m_shader);
	clipPlaneConstants.amountOfClipPlanes = 5;
	clipPlaneConstants.isEnabled = 1;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, map->m_riftMap->m_clipPlaneCBO);
	g_engine->m_render->SetModelConstants(Mat44(), Rgba8::WHITE);
	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->ClearStencilBuffer();
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::WRITE_TO_STENCIL);
	Camera* currentCamera = g_engine->m_render->GetCamera();
	float cameraNearPlaneDist = currentCamera->GetPerspectiveNearPlane() + 0.001f;
	float cameraAspect = currentCamera->GetPerspectiveAspect();
	float cameraYFOV = currentCamera->GetPerspectiveFOV();

	Vec3 nearPlaneCenter = currentCamera->GetPosition() + (currentCamera->GetOrientation().GetForwardDir_IFwd_JLeft_KUp() * cameraNearPlaneDist);
	float halfSizeOfNearPlaneY =  cameraNearPlaneDist / CosDegrees(cameraYFOV * 0.5f);
	float halfSizeOfNearPlaneX = halfSizeOfNearPlaneY * cameraAspect;
	Vec3 nearLeft = currentCamera->GetOrientation().GetLeftDir_IFwd_JLeft_KUp() * halfSizeOfNearPlaneX;
	Vec3 nearUp = currentCamera->GetOrientation().GetUpDir_IFwd_JLeft_KUp() * halfSizeOfNearPlaneY;

	Vec3 nearBL = nearPlaneCenter + nearLeft - nearUp;
	Vec3 nearBR = nearPlaneCenter - nearLeft - nearUp;
	Vec3 nearTR = nearPlaneCenter - nearLeft + nearUp;
	Vec3 nearTL = nearPlaneCenter + nearLeft + nearUp;

	std::vector<Vertex> nearPlaneVerts;
	AddVertsForQuad3D(nearPlaneVerts,
		nearBL,
		nearBR,
		nearTR,
		nearTL, 
		Rgba8(0,0,0,100)
	);
	g_engine->m_render->DrawVertexList(&nearPlaneVerts);
	clipPlaneConstants.isEnabled = 0;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, map->m_riftMap->m_clipPlaneCBO);
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Draw portal onto stencil buffer
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);

	g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), Rgba8::WHITE);
	SpriteDef spriteDef;
	if (m_isDead)
	{
		float totalTime = map->m_game->m_riftStencilAnim->GetTotalTime();
		spriteDef = map->m_game->m_riftStencilAnim->GetSpriteDefAtTime(totalTime - (float)m_deathTimer->GetElapsedTime());
	}
	else
	{
		spriteDef = map->m_game->m_riftStencilAnim->GetSpriteDefAtTime((float)m_animTimer->GetElapsedTime());
	}
	AABB2 spriteUVs = spriteDef.m_UVs;
	m_vertexes[0].m_uvTexCoords = spriteUVs.m_mins;
	m_vertexes[1].m_uvTexCoords = Vec2(spriteUVs.m_maxs.x, spriteUVs.m_mins.y);
	m_vertexes[2].m_uvTexCoords = spriteUVs.m_maxs;
	m_vertexes[3].m_uvTexCoords = spriteUVs.m_maxs;
	m_vertexes[4].m_uvTexCoords = Vec2(spriteUVs.m_mins.x, spriteUVs.m_maxs.y);
	m_vertexes[5].m_uvTexCoords = spriteUVs.m_mins;
	g_engine->m_render->BindTexture(map->m_game->m_riftStencilAnim->GetTexture());
	g_engine->m_render->DrawVertexList(&m_vertexes);

	g_engine->m_render->EndCamera(map->m_game->m_currentlyRenderedPlayer->m_worldCamera);
	g_engine->m_render->BeginCamera(map->m_game->m_screenCamera);

	// Draw full screen quad only where stencil is drawn to
	g_engine->m_render->BindShader(map->m_game->m_useTexture1Shader);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::ONLY_DRAW_ON_STENCIL);
	std::vector<Vertex> screenVerts;
	AddVertsForQuad3D(screenVerts,
		Vec3(0.f, SCREEN_SIZE_Y, 0.f),
		Vec3(SCREEN_SIZE_X, SCREEN_SIZE_Y, 0.f),
		Vec3(SCREEN_SIZE_X, 0.f, 0.f),
		Vec3(0.f, 0.f, 0.f)
	);
	g_engine->m_render->DrawVertexList(&screenVerts);

	g_engine->m_render->EndCamera(map->m_game->m_screenCamera);
	g_engine->m_render->BeginCamera(map->m_game->m_currentlyRenderedPlayer->m_worldCamera);
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// Set rendering modes and shaders back to default
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
}

void Rift::RenderOutline(const Map* map)
{
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), Rgba8::WHITE);
	SpriteDef spriteDef;
	if (m_isDead)
	{
		float totalTime = map->m_game->m_riftStencilAnim->GetTotalTime();
		spriteDef = map->m_game->m_riftAnim->GetSpriteDefAtTime(totalTime - (float)m_deathTimer->GetElapsedTime());
	}
	else
	{
		spriteDef = map->m_game->m_riftAnim->GetSpriteDefAtTime((float)m_animTimer->GetElapsedTime());
	}
	AABB2 spriteUVs = spriteDef.m_UVs;
	m_vertexes[0].m_uvTexCoords = spriteUVs.m_mins;
	m_vertexes[1].m_uvTexCoords = Vec2(spriteUVs.m_maxs.x, spriteUVs.m_mins.y);
	m_vertexes[2].m_uvTexCoords = spriteUVs.m_maxs;
	m_vertexes[3].m_uvTexCoords = spriteUVs.m_maxs;
	m_vertexes[4].m_uvTexCoords = Vec2(spriteUVs.m_mins.x, spriteUVs.m_maxs.y);
	m_vertexes[5].m_uvTexCoords = spriteUVs.m_mins;
	g_engine->m_render->BindTexture(map->m_game->m_riftAnim->GetTexture());
	g_engine->m_render->DrawVertexList(&m_vertexes);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
}

void Rift::Render_ActorsNearRift(const Map* map)
{
	Vec3 portalNormal = GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
	Vec3 portalLeft = GetOrientation().GetLeftDir_IFwd_JLeft_KUp();
	Vec3 portalUp = GetOrientation().GetUpDir_IFwd_JLeft_KUp();
	Mat44 portalOrientationMatrix = GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Vec4 portalPlane;

	ClipPlaneConstants clipPlaneConstants = ClipPlaneConstants();
	clipPlaneConstants.amountOfClipPlanes = 1;
	clipPlaneConstants.isEnabled = 1;

	for (Actor* actor : m_actorsNearRift)
	{
		if (actor != nullptr && actor->m_map != map)
		{
			float actorDotPortal = DotProduct3D(portalNormal, actor->m_position - GetPosition());
			
			float playerDotPortal = DotProduct3D(portalNormal, map->m_game->m_currentlyRenderedPlayer->m_position - GetPosition());
			if (map->m_isRenderingRift && playerDotPortal * actorDotPortal < 0.f) // Skip rendering this one if the player is on the same side as it.
			{
				continue;
			}

			if (actorDotPortal < 0.f)
			{
				portalPlane = Vec4(portalNormal.x, portalNormal.y, portalNormal.z, DotProduct3D(-portalNormal, m_position));
			}
			else
			{
				portalPlane = Vec4(-portalNormal.x, -portalNormal.y, -portalNormal.z, DotProduct3D(portalNormal, m_position));
			}
			clipPlaneConstants.gClipPlane[0] = portalPlane;
			g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, map->m_riftMap->m_clipPlaneCBO);

			actor->Render();
		}
	}
}

void Rift::Die()
{
	m_isDead = true;
	m_deathTimer->Start();
}

