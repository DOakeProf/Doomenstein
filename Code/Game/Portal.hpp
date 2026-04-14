#pragma once
//#include "Game/Entity.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/AABB3.hpp"

struct Camera;
class Map;

class Portal
{
public:
	Portal(Map* map, Vec3 const& startingPosition, float height, float width);
	~Portal();

	void Update();
	void RenderOutline() const;
	void RenderPortal() const;

	void Update_MoveCamera();

	Mat44 GetModelToWorldTransform() const;
	Mat44 GetModelToWorldTransform_OrientationFlipped() const;
	Mat44 GetWorldToModelTransform() const;

	AABB3 GetPortalAsAABB3() const;

	void AssignPortal(Portal* otherPortal);
	Camera* GetCamera() const;
	Portal* GetOtherPortal() const;
	EulerAngles GetOrientation() const;
	Vec3 GetPosition() const;

	void SetOrientation(EulerAngles const& newOrientation);

	Vec3 bl;
	Vec3 br;
	Vec3 tr;
	Vec3 tl;

	bool m_isPlayerOnFrontSide = false;
	bool m_isFlipped = false; // For checking which side of the portal should be clipped in the portal's view.
	Camera* m_portalCamera = nullptr;

private:
	Portal* m_otherPortal = nullptr;

	Map* m_map = nullptr;
	Vec3 m_position;
	EulerAngles m_orientation;
	
	std::vector<Vertex> m_vertexes;
	std::vector<Vertex> m_borderVertexes;
};