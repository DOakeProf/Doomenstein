#pragma once
//#include "Game/Entity.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

struct Camera;
class Map;

class Portal
{
public:
	Portal(Map* map, Vec3 const& startingPosition, EulerAngles const& orientation, float height, float width);
	~Portal();

	virtual void Update();
	void RenderOutline() const;
	virtual void RenderPortal();

	void MoveCamera();

	Mat44 GetModelToWorldTransform() const;
	Mat44 GetModelToWorldTransform_OrientationFlipped() const;
	Mat44 GetWorldToModelTransform() const;

	AABB3 GetPortalAsAABB3() const;

	void AssignPortal(Portal* otherPortal);
	Camera* GetCamera() const;
	Portal* GetOtherPortal() const;
	EulerAngles GetOrientation() const;
	Vec3 GetPosition() const;

	RaycastResult3D RaycastAgainst(Vec3 const& start, Vec3 const& direction, float length);
	Vec3 TransformPointIntoOtherPortalSpace(Vec3& point);
	EulerAngles TransformOrientationIntoOtherPortalSpace(EulerAngles& orientation);

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