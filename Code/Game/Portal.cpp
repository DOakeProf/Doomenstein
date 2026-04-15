#include "Game/Portal.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"

const Vec3 offsetInsidePortalAABB3 = Vec3(0.2f, 0.f, 0.f);
const Vec3 offsetOutsidePortalAABB3 = Vec3(0.00003f, 0.f, 0.f);
const Vec3 offsetZValuePortalAABB3 = Vec3(0.f, 0.f, 0.00001f);
const Vec3 offsetYValuePortalAABB3 = Vec3(0.f, 0.0001f, 0.f);

Portal::Portal(Map* map, Vec3 const& startingPosition, float height, float width)
	: m_map(map)
	, m_position(startingPosition)
{
	m_portalCamera = new Camera();
	m_portalCamera->SetPerspectiveView(SCREEN_ASPECT, 60.f, 0.1f, 100.f);
	m_portalCamera->SetCameraToRenderTransform(Camera::GAME_TO_DIRECTX_CONVENTIONS);
	
	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;
	bl = Vec3(0.f, halfWidth, -halfHeight);
	br = Vec3(0.f, -halfWidth, -halfHeight);
	tr = Vec3(0.f, -halfWidth, halfHeight);
	tl = Vec3(0.f, halfWidth, halfHeight);

	//Vec3 xOffset = Vec3(0.15f, 0.f, 0.f);
	//AddVertsForQuad3D(m_frontVertexes, bl + xOffset, br + xOffset, tr + xOffset, tl + xOffset); // This is for the rendering trick where you render the portal plane further away from the camera.
	//AddVertsForQuad3D(m_backVertexes, bl - xOffset, br - xOffset, tr - xOffset, tl - xOffset);


	AABB3 portalRenderAABB3 = AABB3(
		bl + offsetOutsidePortalAABB3 + offsetZValuePortalAABB3 - offsetYValuePortalAABB3,
		tr - offsetInsidePortalAABB3 - offsetZValuePortalAABB3 + offsetYValuePortalAABB3
	);
	AddVertsForAABB3D(m_vertexes, portalRenderAABB3);
	//AddVertsForQuad3D(m_vertexes, bl - xOffset, br - xOffset, tr - xOffset, tl - xOffset);

	float borderSize = 0.12f;

	AABB3 leftAABB3 = AABB3(bl + Vec3(-borderSize, borderSize, -borderSize), tl + Vec3(borderSize, -borderSize, borderSize));
	AABB3 bottomAABB3 = AABB3(bl + Vec3(-borderSize, -borderSize, -borderSize), br + Vec3(borderSize, borderSize, borderSize));
	AABB3 rightAABB3 = AABB3(br + Vec3(-borderSize, borderSize, -borderSize), tr + Vec3(borderSize, -borderSize, borderSize));
	AABB3 topAABB3 = AABB3(tl + Vec3(-borderSize, -borderSize, -borderSize), tr + Vec3(borderSize, borderSize, borderSize));

	AddVertsForAABB3D(m_borderVertexes, leftAABB3, Rgba8::WHITE);
	AddVertsForAABB3D(m_borderVertexes, bottomAABB3, Rgba8::WHITE);
	AddVertsForAABB3D(m_borderVertexes, rightAABB3, Rgba8::WHITE);
	AddVertsForAABB3D(m_borderVertexes, topAABB3, Rgba8::WHITE);
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

	//Mat44 portalMatrix = GetModelToWorldTransform();
	//DebugAddBasis(portalMatrix, 0.f, 0.5f, 0.1f);
}

void Portal::RenderOutline() const
{
	// Draw Outline
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::READ_WRITE_LESS_EQUAL);
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_render->BindTexture(nullptr);

	g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), Rgba8::WHITE);
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
	g_engine->m_render->ClearTargetTextureBuffer(m_map->m_game->m_backgroundClearColor);
	g_engine->m_render->ClearTargetTextureDepthBuffer();

	// Render Everything from the other portal's camera
	g_engine->m_render->EndCamera(m_map->m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_portalCamera);

	Vec3 portalNormal = m_otherPortal->GetOrientation().GetForwardDir_IFwd_JLeft_KUp();
	Vec4 portalPlane;
	if (m_isFlipped)
	{
		portalPlane = Vec4(portalNormal.x, portalNormal.y, portalNormal.z, DotProduct3D(-portalNormal, m_otherPortal->GetPosition()));
	}
	else
	{
		portalPlane = Vec4(-portalNormal.x, -portalNormal.y, -portalNormal.z, DotProduct3D(portalNormal, m_otherPortal->GetPosition()));
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Render world
	ClipPlaneConstants clipPlaneConstants = ClipPlaneConstants();
	clipPlaneConstants.gClipPlane = portalPlane;
	clipPlaneConstants.isEnabled = 1;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, m_map->m_clipPlaneCBO);

	m_map->Render_World();

	clipPlaneConstants.isEnabled = 0;
	g_engine->m_render->SetConstantBufferData(k_clipPlaneConstantsSlot, clipPlaneConstants, m_map->m_clipPlaneCBO);
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	g_engine->m_render->EndCamera(m_portalCamera);
	g_engine->m_render->BeginCamera(m_map->m_player->m_worldCamera);

	// Draw portal onto stencil buffer
	g_engine->m_render->BindTexture(nullptr);
	g_engine->m_render->ClearStencilBuffer();
	g_engine->m_render->ChangeRenderTargetToBackBuffer();
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::WRITE_TO_STENCIL);
	g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_engine->m_render->SetBlendMode(BlendMode::ALPHA);

	if (m_isFlipped)
	{
		g_engine->m_render->SetModelConstants(GetModelToWorldTransform_OrientationFlipped(), Rgba8::WHITE);
	}
	else
	{
		g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), Rgba8::WHITE);
	}
	g_engine->m_render->DrawVertexList(&m_vertexes);

	g_engine->m_render->EndCamera(m_map->m_player->m_worldCamera);
	g_engine->m_render->BeginCamera(m_map->m_game->m_screenCamera);

	// Draw full screen quad only where stencil is drawn to
	g_engine->m_render->BindShader(m_map->m_game->m_useTexture1Shader);
	g_engine->m_render->SetDepthStencilMode(DepthStencilMode::ONLY_DRAW_ON_STENCIL);
	std::vector<Vertex> screenVerts;
	AddVertsForQuad3D(screenVerts,
					  Vec3(0.f, SCREEN_SIZE_Y, 0.f),
					  Vec3(SCREEN_SIZE_X, SCREEN_SIZE_Y, 0.f),
					  Vec3(SCREEN_SIZE_X, 0.f, 0.f),
					  Vec3(0.f, 0.f, 0.f)
	);
	g_engine->m_render->DrawVertexList(&screenVerts);

	g_engine->m_render->EndCamera(m_map->m_game->m_screenCamera);
	g_engine->m_render->BeginCamera(m_map->m_player->m_worldCamera);
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
	Vec3 playerPos = m_map->m_player->m_worldCamera->GetPosition();
	EulerAngles playerOrientation = m_map->m_player->m_worldCamera->GetOrientation();

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
	Vec3 playerToSelfPortal = m_position - m_map->m_player->m_position;
	float agreementBetweenCameraFwdAndCameraToPortal = DotProduct3D(playerToSelfPortal.GetNormalized(), playerOrientation.GetForwardDir_IFwd_JLeft_KUp());
	//m_portalCamera->ChangeNearPlane(lengthOfCameraToPortal * agreementBetweenCameraFwdAndCameraToPortal); // TODO: Make this another plane clip in the pixel shader.

	//std::string portal1CameraRotation = Stringf("Portal 1 Camera Rotation: %.4f, %.4f, %.4f", m_portals[1]->m_portalCamera->GetOrientation().m_yawDegrees, m_portals[1]->m_portalCamera->GetOrientation().m_pitchDegrees, m_portals[1]->m_portalCamera->GetOrientation().m_rollDegrees);
	//DebugAddMessage(portal1CameraRotation, 0.f, Rgba8::CYAN);
}

Mat44 Portal::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

Mat44 Portal::GetModelToWorldTransform_OrientationFlipped() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	EulerAngles flippedOrientation = m_orientation;
	Vec3 portalNormal = m_orientation.GetForwardDir_IFwd_JLeft_KUp();
	if (portalNormal.z != 0.f) // If portal is horizontal
	{
		flippedOrientation.m_pitchDegrees = m_orientation.m_pitchDegrees + 180.f;
	}
	else // If portal is vertical
	{
		flippedOrientation.m_yawDegrees = m_orientation.m_yawDegrees + 180.f;
	}
	Mat44 orientationMatrix = flippedOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

Mat44 Portal::GetWorldToModelTransform() const
{
	Mat44 worldToModel = GetModelToWorldTransform();
	worldToModel = worldToModel.GetOrthonormalInverse();
	return worldToModel;
}

AABB3 Portal::GetPortalAsAABB3() const
{
	Mat44 portalToWorld;
	Vec3 mins;
	Vec3 maxs;
	if (m_isFlipped)
	{
		portalToWorld = GetModelToWorldTransform();
	}
	else
	{
		portalToWorld = GetModelToWorldTransform_OrientationFlipped();
	}

	mins = bl + offsetZValuePortalAABB3 - offsetYValuePortalAABB3 - Vec3(0.0001f, 0.f, 0.f); // To prevent z fighting by clipping pixels the portal is almost on.
	maxs = tr + offsetInsidePortalAABB3 - offsetZValuePortalAABB3 + offsetYValuePortalAABB3;

	mins = portalToWorld.TransformPosition3D(mins);
	maxs = portalToWorld.TransformPosition3D(maxs);

	if (mins.x > maxs.x)
	{
		float xStorage = mins.x;
		mins.x = maxs.x;
		maxs.x = xStorage;
	}
	if (mins.y > maxs.y)
	{
		float yStorage = mins.y;
		mins.y = maxs.y;
		maxs.y = yStorage;
	}
	if (mins.z > maxs.z)
	{
		float zStorage = mins.z;
		mins.z = maxs.z;
		maxs.z = zStorage;
	}

	return AABB3(mins, maxs);
}

void Portal::AssignPortal(Portal* otherPortal)
{
	m_otherPortal = otherPortal;
}

Camera* Portal::GetCamera() const
{
	return m_portalCamera;
}

Portal* Portal::GetOtherPortal() const
{
	return m_otherPortal;
}

EulerAngles Portal::GetOrientation() const
{
	return m_orientation;
}

Vec3 Portal::GetPosition() const
{
	return m_position;
}

RaycastResult3D Portal::RaycastAgainst(Vec3 const& start, Vec3 const& direction, float length)
{
	Mat44 portalTransform = GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Vec3 bottomLeft = portalTransform.TransformPosition3D(bl);
	Vec3 bottomRight = portalTransform.TransformPosition3D(br);
	Vec3 topRight = portalTransform.TransformPosition3D(tr);
	Vec3 topLeft = portalTransform.TransformPosition3D(tl);
	RaycastResult3D result = RaycastVSQuad3D(start, direction, length,
		bottomLeft + GetPosition(),
		bottomRight + GetPosition(),
		topRight + GetPosition(),
		topLeft + GetPosition()
	);
	return result;
}

Vec3 Portal::TransformPointIntoOtherPortalSpace(Vec3& point)
{
	if (m_otherPortal == nullptr)
	{
		return point;
	}

	Mat44 portalTransform = GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	Mat44 otherPortalTransform = GetOtherPortal()->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();

	// Calculate the proper position in the other portal space and set the actors desired position to that.
	Vec3 portalToPoint = point - m_position;
	portalTransform.Transpose();
	portalToPoint = portalTransform.TransformPosition3D(portalToPoint);
	portalToPoint = otherPortalTransform.TransformPosition3D(portalToPoint);
	point = GetOtherPortal()->GetPosition() + portalToPoint;

	return point;
}

EulerAngles Portal::TransformOrientationIntoOtherPortalSpace(EulerAngles& orientation)
{
	if (m_otherPortal == nullptr)
	{
		return orientation;
	}

	EulerAngles newOrientation = orientation;
	Mat44 selfMatrixWorldToModel = GetWorldToModelTransform();
	Mat44 otherPortalMatrixModelToWorld = GetOtherPortal()->GetModelToWorldTransform();
	selfMatrixWorldToModel.Append(newOrientation.GetAsMatrix_IFwd_JLeft_KUp()); // Transform the actor's orientation into self model space by appending
	Mat44 newOrientationMatrix = otherPortalMatrixModelToWorld;
	newOrientationMatrix.Append(selfMatrixWorldToModel); // Transform actor's orientation from self model space back to world space with reference to the other portal's space.
	EulerAngles orientationInOtherPortalSpace = EulerAngles(newOrientationMatrix);
	orientation = orientationInOtherPortalSpace;

	return orientation;
}

void Portal::SetOrientation(EulerAngles const& newOrientation)
{
	m_orientation = newOrientation;
}

