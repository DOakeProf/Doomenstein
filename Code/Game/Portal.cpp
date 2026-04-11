#include "Game/Portal.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/GameCommon.hpp"

Portal::Portal(Game* owner, Vec3 const& startingPosition)
	: Entity(owner, startingPosition)
{
	m_portalCamera = new Camera();
	m_portalCamera->SetPerspectiveView(SCREEN_ASPECT, m_game->m_perspectiveFOV, 0.1f, 100.f);
	m_portalCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);

	bl = Vec3(0.f, 0.5f, -1.f);
	br = Vec3(0.f, -0.5f, -1.f);
	tr = Vec3(0.f, -0.5f, 1.f);
	tl = Vec3(0.f, 0.5f, 1.f);

	Vec3 xOffset = Vec3(0.15f, 0.f, 0.f);

	AddVertsForQuad3D(m_frontVertexes, bl + xOffset, br + xOffset, tr + xOffset, tl + xOffset);
	AddVertsForQuad3D(m_backVertexes, bl - xOffset, br - xOffset, tr - xOffset, tl - xOffset);

	float borderSize = 0.12f;

	AABB3 leftAABB3 = AABB3(bl + Vec3(-borderSize, borderSize, -borderSize), tl + Vec3(borderSize, -borderSize, borderSize));
	AABB3 bottomAABB3 = AABB3(bl + Vec3(-borderSize, -borderSize, -borderSize), br + Vec3(borderSize, borderSize, borderSize));
	AABB3 rightAABB3 = AABB3(br + Vec3(-borderSize, borderSize, -borderSize), tr + Vec3(borderSize, -borderSize, borderSize));
	AABB3 topAABB3 = AABB3(tl + Vec3(-borderSize, -borderSize, -borderSize), tr + Vec3(borderSize, borderSize, borderSize));

	AddVertsForAABB3D(m_borderVertexes, leftAABB3, m_color);
	AddVertsForAABB3D(m_borderVertexes, bottomAABB3, m_color);
	AddVertsForAABB3D(m_borderVertexes, rightAABB3, m_color);
	AddVertsForAABB3D(m_borderVertexes, topAABB3, m_color);
}

Portal::~Portal()
{

}

void Portal::Update()
{
	if (m_otherPortal == nullptr)
	{
		return;
	}

	Update_MoveCamera();
}

void Portal::Render() const
{
	// Draw Outline
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_render->BindTexture(nullptr);

	g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
	g_engine->m_render->DrawVertexList(&m_borderVertexes);
}

void Portal::RenderPortal() const
{
	if (m_otherPortal == nullptr)
	{
		return;
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Switch to rendering to the target texture instead of back buffer
	g_engine->m_render->ChangeRenderTargetToTextureBuffer();
	g_engine->m_render->ClearTargetTextureBuffer(m_game->m_backgroundClearColor);
	g_engine->m_render->ClearTargetTextureDepthBuffer();

	// Render Everything from the other portal's camera
	g_engine->m_render->EndCamera(m_game->m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_portalCamera);

	m_game->RenderAllEntities();

	g_engine->m_render->EndCamera(m_portalCamera);
	g_engine->m_render->BeginCamera(m_game->m_player->m_worldCamera);

	// Draw portal onto stencil buffer
	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->ClearStencilBuffer();
	g_engine->m_render->ChangeRenderTargetToBackBuffer();
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::WRITE_TO_STENCIL);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
	if (m_isPlayerOnFrontSide)
	{
		g_engine->m_render->DrawVertexList(&m_backVertexes);
	}
	else
	{
		g_engine->m_render->DrawVertexList(&m_frontVertexes);
	}

	g_engine->m_render->EndCamera(m_game->m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_game->m_player->m_screenCamera);

	// Draw full screen quad only where stencil is drawn to
	g_engine->m_render->BindShader(m_game->m_useTexture1Shader);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::ONLY_DRAW_ON_STENCIL);
	std::vector<Vertex> screenVerts;
	AddVertsForQuad3D(screenVerts,
					  Vec3(0.f, SCREEN_SIZE_Y, 0.f),
					  Vec3(SCREEN_SIZE_X, SCREEN_SIZE_Y, 0.f),
					  Vec3(SCREEN_SIZE_X, 0.f, 0.f),
					  Vec3(0.f, 0.f, 0.f)
	);
	g_engine->m_render->DrawVertexList(&screenVerts);

	g_engine->m_render->EndCamera(m_game->m_player->m_screenCamera);
	g_engine->m_render->BeginCamera(m_game->m_player->m_worldCamera);
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// Set rendering modes and shaders back to default
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
}

void Portal::Update_MoveCamera()
{
	// Get starting world values
	Vec3 otherPortalPos = m_otherPortal->m_position;
	Vec3 playerPos = m_game->m_player->m_worldCamera->GetPosition();
	EulerAngles playerOrientation = m_game->m_player->m_worldCamera->GetOrientation();

	// Convert player position from world space to self portal space
	Mat44 selfMatrixWorldToModel = GetWorldToModelTransform();
	Vec3 playerPosInSelfPortalSpace = selfMatrixWorldToModel.TransformPosition3D(playerPos);

	// Convert the player position back to world space with reference to other portal, instead of the self portal
	Mat44 otherPortalMatrixModelToWorld = m_otherPortal->GetModelToWorldTransform();
	Vec3 newCameraPosition = otherPortalMatrixModelToWorld.TransformPosition3D(playerPosInSelfPortalSpace);

	// Calculate the proper orientation in the other portal space
	Mat44 selfMatrixWorldToModelCopy = selfMatrixWorldToModel;
	selfMatrixWorldToModelCopy.Append(playerOrientation.GetAsMatrix_IFwd_JLeft_KUp()); // Transform the player's orientation into self model space by appending
	Mat44 newOrientationMatrix = otherPortalMatrixModelToWorld;
	newOrientationMatrix.Append(selfMatrixWorldToModelCopy); // Transform player's orientation from self model space back to world space with reference to the other portal's space.
	EulerAngles playerOrientationInOtherPortalSpace = EulerAngles(newOrientationMatrix);

	float lengthOfCameraToPortal = playerPosInSelfPortalSpace.GetLength();

	m_portalCamera->SetPositionAndOrientation(newCameraPosition, playerOrientationInOtherPortalSpace);

	// Calculate what the near plane should be at based on the player's position to the portal and the player's camera orientation.
	Vec3 playerToSelfPortal = m_position - m_game->m_player->m_position;
	float agreementBetweenCameraFwdAndCameraToPortal = DotProduct3D(playerToSelfPortal.GetNormalized(), playerOrientation.GetForwardDir_IFwd_JLeft_KUp());
	m_portalCamera->ChangeNearPlane(lengthOfCameraToPortal * agreementBetweenCameraFwdAndCameraToPortal);

	//std::string portal1CameraRotation = Stringf("Portal 1 Camera Rotation: %.4f, %.4f, %.4f", m_portals[1]->m_portalCamera->GetOrientation().m_yawDegrees, m_portals[1]->m_portalCamera->GetOrientation().m_pitchDegrees, m_portals[1]->m_portalCamera->GetOrientation().m_rollDegrees);
	//DebugAddMessage(portal1CameraRotation, 0.f, Rgba8::CYAN);
}

void Portal::AssignPortal(Portal* otherPortal)
{
	m_otherPortal = otherPortal;
}

Camera* Portal::GetCamera()
{
	return m_portalCamera;
}

Portal* Portal::GetOtherPortal()
{
	return m_otherPortal;
}

